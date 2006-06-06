/*
    KSysGuard, the KDE System Guard

	Copyright (c) 1999, 2000 Chris Schlaeger <cs@kde.org>
	Copyright (c) 2006 John Tapsell <john.tapsell@kdemail.net>

    This program is free software; you can redistribute it and/or
    modify it under the terms of version 2 of the GNU General Public
    License as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#ifndef _ProcessController_h_
#define _ProcessController_h_

#include <QWidget>
#include <QAbstractItemModel>

//Added by qt3to4:
#include <QTimerEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QResizeEvent>
#include <QList>


#include <kapplication.h>

#include <SensorDisplay.h>

#include "ui_ProcessWidgetUI.h"
#include "ProcessModel.h"

#include "ProcessFilter.h"

class QAction;

extern KApplication* Kapp;

/**
 * This widget implements a process list page. Besides the process
 * list which is implemented as a ProcessList, it contains two
 * comboxes and two buttons.  The combo boxes are used to set the
 * update rate and the process filter.  The buttons are used to force
 * an immediate update and to kill a process.
 */
class ProcessController : public KSGRD::SensorDisplay
{
	Q_OBJECT

public:
	ProcessController(QWidget* parent, const QString& title);
	virtual ~ProcessController() { }

	/* Functions for SensorDisplay*/
	
	void resizeEvent(QResizeEvent*);

	bool restoreSettings(QDomElement& element);

	bool saveSettings(QDomDocument& doc, QDomElement& element, bool save = true);

	void refreshList(void)
	{
		updateList();
	}

	virtual void timerEvent(QTimerEvent*)
	{
		updateList();
	}

	virtual bool addSensor(const QString&, const QString&, const QString&, const QString&);

	virtual void answerReceived(int id, const QStringList& answer);

	virtual void sensorError(int, bool err);

	void configureSettings() { }

	virtual bool hasSettingsDialog() const
	{
		return (false);
	}

public slots:

	void expandAllChildren(const QModelIndex &parent);
	void currentRowChanged(const QModelIndex &current);
	void resizeFirstColumn();
		
	void killProcess();
	void killProcess(int pid, int sig);

	void reniceProcess(int pid, int niceValue);

	void updateList();
private slots:
	void rowsInserted( const QModelIndex & parent, int start, int end );
	void expandInit();
	void showContextMenu(const QPoint &point);
	void showOrHideColumn(QAction *);
signals:
	void setFilterMode(int);

private:

	enum { Ps_Info_Command = 1, Ps_Command, Kill_Command, Kill_Supported_Command, Renice_Command, XRes_Info_Command, XRes_Command, XRes_Supported_Command };
	bool mKillSupported;
	/** Is the XRes extension supported where the ksysguardd daemon is? (And does the daemon support it) */
	bool mXResSupported;
	
	/** The column context menu when you right click on a column.*/
	QMenu *mColumnContextMenu;
	
	/** We do not want to send out a "ps" command before we have the results from "ps?" so this is set to false until we get a result from ps?
	 */
	bool mReadyForPs;
	
	QStringList mHeader;
	QStringList mColType;
	QList<QStringList> mData;

	ProcessModel mModel;
	ProcessFilter mFilterModel;
	Ui::ProcessWidget mUi;
};

#endif
