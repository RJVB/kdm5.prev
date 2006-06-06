/*
    kde-system-monitor, the KDE System Monitor

	Copyright (c) 1999 - 2001 Chris Schlaeger <cs@kde.org>
	Copyright (c) 2006 John Tapsell <john.tapsell@kdemail.net>

    This program is free software; you can redistribute it and/or
    modify it under the terms version 2 of of the GNU General Public
    License as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#include <assert.h>
#include <QTimer>

#include <QDomElement>
#include <QVBoxLayout>
#include <QList>
#include <QHBoxLayout>
#include <QResizeEvent>
#include <QSortFilterProxyModel>
#include <QHeaderView>
#include <QAction>
#include <QMenu>
#include <QTime>

#include <kapplication.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kdialogbase.h>

#include <ksgrd/SensorManager.h>

#include "ProcessController.moc"
#include "ProcessController.h"
#include "SignalIDs.h"

#include <QCheckBox>
#include <QComboBox>
#include <QLayout>

#include <kapplication.h>
#include <kpushbutton.h>

ProcessController::ProcessController(QWidget* parent, const QString &title)
	: KSGRD::SensorDisplay(parent, title, false/*isApplet.  Can't be applet, so false*/), mModel(parent), mFilterModel(parent)
{

	mReadyForPs = false;
	mUi.setupUi(this);
	mFilterModel.setSourceModel(&mModel);
	mUi.treeView->setModel(&mFilterModel);
	mColumnContextMenu = new QMenu(mUi.treeView->header());
	connect(mColumnContextMenu, SIGNAL(triggered(QAction*)), this, SLOT(showOrHideColumn(QAction *)));
	
	mUi.treeView->header()->setContextMenuPolicy(Qt::CustomContextMenu);
	
	mUi.treeView->header()->setClickable(true);
	mUi.treeView->header()->setSortIndicatorShown(true);
	
	connect(mUi.btnRefresh, SIGNAL(clicked()), this, SLOT(updateList()));
	connect(mUi.btnKillProcess, SIGNAL(clicked()), this, SLOT(killProcess()));
	connect(mUi.txtFilter, SIGNAL(textChanged(const QString &)), &mFilterModel, SLOT(setFilterRegExp(const QString &)));
	connect(mUi.cmbFilter, SIGNAL(currentIndexChanged(int)), &mFilterModel, SLOT(setFilter(int)));
	connect(mUi.treeView, SIGNAL(expanded(const QModelIndex &)), this, SLOT(expandAllChildren(const QModelIndex &)));
	connect(mUi.treeView, SIGNAL(collapsed(const QModelIndex &)), this, SLOT(resizeFirstColumn()));
	connect(&mModel, SIGNAL(rowsInserted(const QModelIndex &, int, int)), this, SLOT(rowsInserted(const QModelIndex &, int, int)));
	connect(mUi.treeView->selectionModel(), SIGNAL(currentRowChanged(const QModelIndex & , const QModelIndex & )), this, SLOT(currentRowChanged(const QModelIndex &)));
	connect(mUi.treeView->header(), SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showContextMenu(const QPoint&)));
	connect(mUi.chkShowTotals, SIGNAL(stateChanged(int)), &mModel, SLOT(setShowTotals(int)));
	
	setPlotterWidget(this);
	setMinimumSize(sizeHint());
}
void ProcessController::currentRowChanged(const QModelIndex &current)
{
	mUi.btnKillProcess->setEnabled(current.isValid() && mKillSupported);
}
void ProcessController::showContextMenu(const QPoint &point){
	mColumnContextMenu->clear();
	
	{
		int index = mUi.treeView->header()->logicalIndexAt(point);
		if(index >= 0) {
			//selected a column.  Give the option to hide it
			QAction *action = new QAction(mColumnContextMenu);
			action->setData(-index-1); //We set data to be negative (and minus 1) to hide a column, and positive to show a column
			action->setText(i18n("Hide column '%1'", mFilterModel.headerData(index, Qt::Horizontal, Qt::DisplayRole).toString()));
			mColumnContextMenu->addAction(action);
			if(mUi.treeView->header()->sectionsHidden()) {
				mColumnContextMenu->addSeparator();
			}
		}
	}

	if(mUi.treeView->header()->sectionsHidden()) {
		int num_headings = mFilterModel.columnCount();
		for(int i = 0; i < num_headings; ++i) {
			if(mUi.treeView->header()->isSectionHidden(i)) {
				QAction *action = new QAction(mColumnContextMenu);
				action->setText(i18n("Show column '%1'", mFilterModel.headerData(i, Qt::Horizontal, Qt::DisplayRole).toString()));
				action->setData(i); //We set data to be negative (and minus 1)to hide a column, and positive to show a column
				mColumnContextMenu->addAction(action);
			}
		}
	}
	mColumnContextMenu->exec(mUi.treeView->header()->mapToGlobal(point));	
}

void ProcessController::showOrHideColumn(QAction *action)
{
	int index = action->data().toInt();
	//We set data to be negative to hide a column, and positive to show a column
	if(index < 0)
		mUi.treeView->hideColumn(-1-index);
	else {
		mUi.treeView->showColumn(index);
		mUi.treeView->resizeColumnToContents(index);
		mUi.treeView->resizeColumnToContents(mFilterModel.columnCount());
	}
		
}
void ProcessController::expandAllChildren(const QModelIndex &parent) 
{
	QModelIndex sourceParent = mFilterModel.mapToSource(parent);
	for(int i = 0; i < mModel.rowCount(sourceParent); i++) 
		mUi.treeView->expand(mFilterModel.mapFromSource(mModel.index(i,0, sourceParent)));
	QTimer::singleShot(0,this,SLOT(resizeFirstColumn())); //compress events, as this can end up being called for each child
}

void ProcessController::resizeFirstColumn()
{
	mUi.treeView->resizeColumnToContents(0);
}
void ProcessController::rowsInserted( const QModelIndex & parent, int , int )
{
	if(!parent.isValid()) { //init has been added - expand it!
		//We can't expand straight away - it's only just been added.  We need to wait till all its children have been added first
		//So we just fire off a singleshot.  This will call expandInit() in the event loop which is after all the children have been added
		QTimer::singleShot(0, this, SLOT(expandInit()));
	}
}
void ProcessController::expandInit()
{
	mUi.treeView->expand(mFilterModel.mapFromSource(mModel.index(0,0, QModelIndex())));
	QTimer::singleShot(0,this,SLOT(resizeFirstColumn())); //compress events, as this can end up being called for each child
}

void ProcessController::resizeEvent(QResizeEvent* ev)
{
	QWidget::resizeEvent(ev);
}

bool ProcessController::addSensor(const QString& hostName,
				  const QString& sensorName,
				  const QString& sensorType,
				  const QString& title)
{
	if (sensorType != "table")
		return false;

	registerSensor(new KSGRD::SensorProperties(hostName, sensorName, sensorType, title));
	/* This just triggers the first communication. The full set of
	 * requests are send whenever the sensor reconnects (detected in
	 * sensorError(). */

	mModel.setIsLocalhost(sensors().at(0)->isLocalhost()); //by telling our model that this is localhost, it can provide more information about the data it has
	
	sensors().at(0)->setIsOk(true); //Assume it is okay from the start
	setSensorOk(sensors().at(0)->isOk());
	
	kDebug() << "Sending ps? in addsensor " << QTime::currentTime().toString("hh:mm:ss.zzz") << endl;
	sendRequest(hostName, "ps?", Ps_Info_Command);
	kDebug() << "Sending ps in addsensor " << QTime::currentTime().toString("hh:mm:ss.zzz") << endl;
	sendRequest(hostName, "ps", Ps_Command);
	sendRequest(hostName, "test kill", Kill_Supported_Command);
	sendRequest(hostName, "test xres", XRes_Supported_Command);

	if (title.isEmpty())
		setTitle(i18n("%1: Running Processes", hostName));
	else
		setTitle(title);

	return true;
}

void
ProcessController::updateList()
{
	if(!mReadyForPs) {
		return;
	}
	sendRequest(sensors().at(0)->hostName(), "ps", Ps_Command);
	if(mXResSupported)
		sendRequest(sensors().at(0)->hostName(), "xres", XRes_Command);
}

void ProcessController::killProcess(int pid, int sig)
{
	sendRequest(sensors().at(0)->hostName(),
				QString("kill %1 %2" ).arg(pid).arg(sig), Kill_Command);

	if ( !timerOn() )
	    // give kde-system-monitor-daemon time to update its proccess list
	    QTimer::singleShot(3000, this, SLOT(updateList()));
	else
	    updateList();
}

void
ProcessController::killProcess()
{
	QModelIndexList selectedIndexes = mUi.treeView->selectionModel()->selectedIndexes();
	QStringList selectedAsStrings;
	QList< long long> selectedPids;
	for (int i = 0; i < selectedIndexes.size(); ++i) {
		if(selectedIndexes.at(i).column() == 0) { //I can't work out how to get selectedIndexes() to only return 1 index per row
			Process *process = reinterpret_cast<Process *> (mFilterModel.mapToSource(selectedIndexes.at(i)).internalPointer());
			selectedPids << process->pid;
			selectedAsStrings << mModel.getStringForProcess(process);
		}
	}
	
	if (selectedAsStrings.isEmpty())
	{
		KMessageBox::sorry(this, i18n("You need to select a process first."));
		return;
	}
	else
	{
		QString  msg = i18np("Do you want to kill the selected process?",
				"Do you want to kill the %n selected processes?",
				selectedAsStrings.count());

		KDialogBase *dlg = new KDialogBase (  i18n ("Kill Process"),
						      KDialogBase::Yes | KDialogBase::Cancel,
						      KDialogBase::Yes, KDialogBase::Cancel, this->parentWidget(),
						      "killconfirmation",
			       			      true, true, KGuiItem(i18n("Kill")));

		bool dontAgain = false;

		int res = KMessageBox::createKMessageBox(dlg, QMessageBox::Question,
			                                 msg, selectedAsStrings,
							 i18n("Do not ask again"), &dontAgain,
							 KMessageBox::Notify);

		if (res != KDialogBase::Yes)
		{
			return;
		}
	}

	Q_ASSERT(selectedPids.size() == selectedAsStrings.size());
        for (int i = 0; i < selectedPids.size(); ++i) {
            sendRequest(sensors().at(0)->hostName(), QString("kill %1 %2" ).arg(selectedPids.at( i ))
                       .arg(MENU_ID_SIGKILL), Kill_Command);
        }
	if ( !timerOn())
		// give kde-system-monitor-daemon time to update its proccess list
		QTimer::singleShot(3000, this, SLOT(updateList()));
	else
		updateList();
}

void
ProcessController::reniceProcess(int pid, int niceValue)
{
	sendRequest(sensors().at(0)->hostName(),
				QString("setpriority %1 %2" ).arg(pid).arg(niceValue), Renice_Command);
	sendRequest(sensors().at(0)->hostName(), "ps", Ps_Command);  //update the display afterwards
}

void
ProcessController::answerReceived(int id, const QStringList& answer)
{
	/* We received something, so the sensor is probably ok. */
	sensorError(id, false);
	switch (id)
	{
	case Ps_Info_Command:
	{

		kDebug() << "PS_INFO_COMMAND: Received ps info at " << QTime::currentTime().toString("hh:mm:ss.zzz") << endl;
		/* We have received the answer to a ps? command that contains
		 * the information about the table headers. */
		if (answer.count() != 2)
		{
			kDebug (1215) << "ProcessController::answerReceived(Ps_Info_Command)"
				  "wrong number of lines [" <<  answer << "]" << endl;
			sensorError(id, true);
			return;
		}
		QString line = answer.at(1);
		QByteArray coltype;
		for(int i = 0; i < line.size(); i++)  //coltype is in the form "d\tf\tS\t" etc, so split into a list of char
			if(line[i] != '\t')
				coltype += line[i].toLatin1();
		QStringList header = answer.at(0).split('\t');
		if(coltype.count() != header.count()) {
			kDebug(1215) << "ProcessControll::answerReceived.  Invalid data from a client - column type and data don't match in number.  Discarding" << endl;
			sensorError(id, true);
			return;
		}
		if(!mModel.setHeader(header, coltype)) {
			sensorError(id,true);
			return;
		}

		mFilterModel.setFilterKeyColumn(0);
		kDebug() << "PS_INFO_COMMAND: We are now ready for ps.  " << QTime::currentTime().toString("hh:mm:ss.zzz") << endl;
		mReadyForPs = true;
//		sendRequest(sensors().at(0)->hostName(), "ps", Ps_Command);

		break;
	}
	case Ps_Command:
	{
		kDebug() << "We received ps data." << QTime::currentTime().toString("hh:mm:ss.zzz") << endl;
		/* We have received the answer to a ps command that contains a
		 * list of processes with various additional information. */
		if(!mReadyForPs) {
			kDebug(1215) << "Process data arrived before we were ready for it. hmm" << endl;
			break;
		}
		QList<QStringList> data;
		QStringListIterator i(answer);
		while(i.hasNext()) {
			QString row = i.next();
			if(!row.trimmed().isEmpty())
				data << row.split('\t');
		}
		if(data.isEmpty()) {
			kDebug(1215) << "No data in the answer from 'ps'" << endl;
			break;
		}
		if(!mModel.setData(data)) {
			sensorError(id, true);
			return;
		}

		kDebug() << "We finished with ps data." << QTime::currentTime().toString("hh:mm:ss.zzz") << endl;
		break;
	}
	case Kill_Command:
	{
		// result of kill operation
		if(answer.count() != 2) {
			kDebug(1215) << "Invalid answer from a kill request" << endl;
			break;
		}
		switch (answer[0].toInt())
		{
		case 0:	// successful kill operation
			break;
		case 1:	// unknown error
			KSGRD::SensorMgr->notify(
				i18n("Error while attempting to kill process %1.",
				 answer[1]));
			break;
		case 2:
			KSGRD::SensorMgr->notify(
				i18n("Insufficient permissions to kill "
							 "process %1.", answer[1]));
			break;
		case 3:
			KSGRD::SensorMgr->notify(
				i18n("Process %1 has already disappeared.",
				 answer[1]));
			break;
		case 4:
			KSGRD::SensorMgr->notify(i18n("Invalid Signal."));
			break;
		}
		break;
	}
	case Kill_Supported_Command:
		kDebug() << "We received kill supported data." << QTime::currentTime().toString("hh:mm:ss.zzz") << endl;
		if(!answer.isEmpty())
			mKillSupported = (answer[0].toInt() == 1);
		break;
	case Renice_Command:
	{
		// result of renice operation
		if(answer.count() != 2) {
			kDebug(1215) << "Invalid answer from a renice request" << endl;
			break;
		}
		switch (answer[0].toInt())
		{
		case 0:	// successful renice operation
			break;
		case 1:	// unknown error
			KSGRD::SensorMgr->notify(
				i18n("Error while attempting to renice process %1.",
				 answer[1]));
			break;
		case 2:
			KSGRD::SensorMgr->notify(
				i18n("Insufficient permissions to renice "
							 "process %1.", answer[1]));
			break;
		case 3:
			KSGRD::SensorMgr->notify(
				i18n("Process %1 has already disappeared.",
				 answer[1]));
			break;
		case 4:
			KSGRD::SensorMgr->notify(i18n("Invalid argument."));
			break;
		}
		break;
	}
	case XRes_Info_Command:
	{
		kDebug() << "XRES INFO" << endl;
		if (answer.count() != 2)
		{
			kDebug (1215) << "ProcessController::answerReceived(XRes_Info_Command)"
				  "wrong number of lines [" <<  answer << "]" << endl;
			sensorError(id, true);
			return;
		}
		QString line = answer.at(1);
		QByteArray coltype;
		for(int i = 0; i < line.size(); i++)  //coltype is in the form "d\tf\tS\t" etc, so split into a list of char
			if(line[i] != '\t')
				coltype += line[i].toLatin1();//FIXME: the answer should really be a QByteArray, not a string
		QStringList header = answer.at(0).split('\t');
		if(coltype.size() != header.count()) {
			kDebug(1215) << "ProcessControll::answerReceived.  Invalid data from a client - column type and data don't match in number.  Discarding" << endl;
			sensorError(id, true);
			return;
		}
		if(!mModel.setXResHeader(header, coltype)) {
			sensorError(id,true);
			return;
		}

		break;
	}
	case XRes_Command:
	{
		/* We have received the answer to a xres command that contains a
		 * list of processes that we should already know about, with various additional information. */
		QStringListIterator i(answer);
		while(i.hasNext()) {
		  mModel.setXResData(i.next().split('\t'));
		}
		break;
	}
	case XRes_Supported_Command:
	{
		kDebug() << "We received xres supported. " << QTime::currentTime().toString("hh:mm:ss.zzz") << endl;
		if(answer.isEmpty())
		  mXResSupported = false;
		else {
		  mXResSupported = (answer[0].toInt() == 1);
		  if(mXResSupported) 
			sendRequest(sensors().at(0)->hostName(), "xres?", XRes_Info_Command);
		}
		break;
	}

	}
}

void
ProcessController::sensorError(int, bool err)
{
	if (err == sensors().at(0)->isOk())
	{
		if (!err)
		{
			/* Whenever the communication with the sensor has been
			 * (re-)established we need to requests the full set of
			 * properties again, since the back-end might be a new
			 * one. */

			mReadyForPs = false;
			kDebug() << "Sending ps1? " << QTime::currentTime().toString("hh:mm:ss.zzz") << endl;
			sendRequest(sensors().at(0)->hostName(), "test kill", Kill_Supported_Command);
			sendRequest(sensors().at(0)->hostName(), "test xres", XRes_Supported_Command);
			kDebug() << "Sending ps? command" << endl;
			kDebug() << "Sending ps2? " << QTime::currentTime().toString("hh:mm:ss.zzz") << endl;
			sendRequest(sensors().at(0)->hostName(), "ps?", Ps_Info_Command);
		} else {
			kDebug() << "SensorError called with an error" << endl;
		}
		/* This happens only when the sensorOk status needs to be changed. */
		sensors().at(0)->setIsOk( !err );
	}
	setSensorOk(sensors().at(0)->isOk());
}

bool
ProcessController::restoreSettings(QDomElement& element)
{
	kDebug() << "RESTORING SETTINGS " << endl;
	bool result = addSensor(element.attribute("hostName"),
				element.attribute("sensorName"),
				(element.attribute("sensorType").isEmpty() ? "table" : element.attribute("sensorType")),
				QString());
//	mUi.chkTreeView->setChecked(element.attribute("tree").toInt());
//	setTreeView(element.attribute("tree").toInt());

	uint filter = element.attribute("filter", "0").toUInt();
	mUi.cmbFilter->setCurrentIndex(filter);

	uint col = element.attribute("sortColumn", "1").toUInt(); //Default to sorting the user column
	bool inc = element.attribute("incrOrder", "0").toUInt();  //Default to descending order
	mFilterModel.sort(col,(inc)?Qt::AscendingOrder:Qt::DescendingOrder);

	bool showTotals = element.attribute("showTotals", "1").toUInt();
	mUi.chkShowTotals->setCheckState( (showTotals)?Qt::Checked:Qt::Unchecked );
	
	SensorDisplay::restoreSettings(element);
	setModified(false);

	return (result);
}

bool
ProcessController::saveSettings(QDomDocument& doc, QDomElement& element, bool save)
{
	kDebug() << "SAVING SETTINGS " << endl;
	element.setAttribute("hostName", sensors().at(0)->hostName());
	element.setAttribute("sensorName", sensors().at(0)->name());
	element.setAttribute("sensorType", sensors().at(0)->type());
//	element.setAttribute("tree", (uint) mUi.chkTreeView->isChecked());
	element.setAttribute("showTotals", (uint) (mUi.chkShowTotals->checkState() == Qt::Unchecked));
	
	element.setAttribute("filter", mUi.cmbFilter->currentIndex());
	element.setAttribute("sortColumn", mUi.treeView->header()->sortIndicatorSection());
	element.setAttribute("incrOrder", (uint) (mUi.treeView->header()->sortIndicatorOrder() == Qt::AscendingOrder));

	SensorDisplay::saveSettings(doc, element);

	if (save)
		setModified(false);

	return (true);
}

