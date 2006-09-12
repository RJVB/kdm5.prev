/*

chooser widget for KDM

Copyright (C) 2002-2003 Oswald Buddenhagen <ossi@kde.org>
based on the chooser (C) 1999 by Harald Hoyer <Harald.Hoyer@RedHat.de>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#include "kchooser.h"
#include "kconsole.h"
#include "kdmconfig.h"
#include "kdm_greet.h"

#include <klocale.h>

#include <QLayout>
#include <QLabel>
#include <QPushButton>
#include <QSocketNotifier>
#include <QLineEdit>
//Added by qt3to4:
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QBoxLayout>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QHeaderView>

#include <stdlib.h> // for free()

class ChooserListViewItem : public QTreeWidgetItem {
  public:
	ChooserListViewItem( QTreeWidget *parent, int _id, const QString &nam, const QString &sts )
		: QTreeWidgetItem( parent, QStringList() << nam << sts ) { id = _id; };

	int id;
};


ChooserDlg::ChooserDlg()
	: inherited()
{
	completeMenu( LOGIN_REMOTE_ONLY, ex_greet, i18n("&Local Login"), Qt::ALT+Qt::Key_L );

	QBoxLayout *vbox = new QVBoxLayout( this );

	QLabel *title = new QLabel( i18n("XDMCP Host Menu"), this );
	title->setAlignment( Qt::AlignCenter );
	vbox->addWidget( title );

	host_view = new QTreeWidget( this );
	host_view->setRootIsDecorated( false );
	host_view->setUniformRowHeights( true );
	host_view->setEditTriggers( QAbstractItemView::NoEditTriggers );
	host_view->setColumnCount( 2 );
	host_view->setHeaderLabels( QStringList() << i18n("Hostname") << i18n("Status") );
	host_view->setColumnWidth( 0, fontMetrics().width( "login.crap.net" ) );
	host_view->setMinimumWidth( fontMetrics().width( "login.crap.com Display not authorized to connect this server" ) );
	host_view->setAllColumnsShowFocus( true );
	vbox->addWidget( host_view );

	iline = new QLineEdit( this );
	iline->setEnabled( TRUE );
	QLabel *itxt = new QLabel( i18n("Hos&t:"), this );
	itxt->setBuddy( iline );
	QPushButton *addButton = new QPushButton( i18n("A&dd"), this );
	connect( addButton, SIGNAL(clicked()), SLOT(addHostname()) );
	QBoxLayout *hibox = new QHBoxLayout();
	vbox->addLayout( hibox );
	hibox->setParent( vbox );
	hibox->addWidget( itxt );
	hibox->addWidget( iline );
	hibox->addWidget( addButton );

	// Buttons
	QPushButton *acceptButton = new QPushButton( i18n("&Accept"), this );
	acceptButton->setDefault( true );
	QPushButton *pingButton = new QPushButton( i18n("&Refresh"), this );

	QBoxLayout *hbox = new QHBoxLayout();
	vbox->addLayout( hbox );
	hbox->setParent( vbox );
	hbox->setSpacing( 20 );
	hbox->addWidget( acceptButton );
	hbox->addWidget( pingButton );
	hbox->addStretch( 1 );

	if (optMenu) {
		QPushButton *menuButton = new QPushButton( i18n("&Menu"), this );
		menuButton->setMenu( optMenu );
		hbox->addWidget( menuButton );
		hbox->addStretch( 1 );
	}

//	QPushButton *helpButton = new QPushButton( i18n("&Help"), this );
//	hbox->addWidget( helpButton );

#ifdef WITH_KDM_XCONSOLE
	if (consoleView)
		vbox->addWidget( consoleView );
#endif

	sn = new QSocketNotifier( rfd, QSocketNotifier::Read, this );
	connect( sn, SIGNAL(activated( int )), SLOT(slotReadPipe()) );

	connect( pingButton, SIGNAL(clicked()), SLOT(pingHosts()) );
	connect( acceptButton, SIGNAL(clicked()), SLOT(accept()) );
//	connect( helpButton, SIGNAL(clicked()), SLOT(slotHelp()) );
	connect( host_view, SIGNAL(itemActivated( QTreeWidgetItem *, int )), SLOT(accept()) );

	adjustGeometry();
}

/*
void ChooserDlg::slotHelp()
{
	KMessageBox::information( 0,
	                          i18n("Choose a host, you want to work on,\n"
	                               "in the list or add one.\n\n"
	                               "After this box, you must press cancel\n"
	                               "in the Host Menu to enter a host. :("));
	iline->setFocus();
}
*/

void ChooserDlg::addHostname()
{
	if (!iline->text().isEmpty()) {
		GSendInt( G_Ch_RegisterHost );
		GSendStr( iline->text().toLatin1() );
		iline->clear();
	}
}

void ChooserDlg::pingHosts()
{
	GSendInt( G_Ch_Refresh );
}

void ChooserDlg::accept()
{
	if (focusWidget() == iline) {
		if (!iline->text().isEmpty()) {
			GSendInt( G_Ch_DirectChoice );
			GSendStr( iline->text().toLatin1() );
			iline->clear();
		}
		return;
	} else /*if (focusWidget() == host_view)*/ {
		QTreeWidgetItem *item = host_view->currentItem();
		if (item) {
			GSendInt( G_Ready );
			GSendInt( ((ChooserListViewItem *)item)->id );
			::exit( EX_NORMAL );
		}
	}
}

void ChooserDlg::reject()
{
}

QString ChooserDlg::recvStr()
{
	char *arr = GRecvStr();
	if (arr) {
		QString str = QString::fromLatin1( arr );
		free( arr );
		return str;
	} else
		return i18n("<unknown>");
}

ChooserListViewItem *ChooserDlg::findItem( int id )
{
	for (int i = 0, rc = host_view->model()->rowCount(); i < rc; i++) {
		ChooserListViewItem *itm = static_cast<ChooserListViewItem *>(
			host_view->topLevelItem( i ));
		if (itm->id == id)
			return itm;
	}
	return 0;
}

void ChooserDlg::slotReadPipe()
{
	int id;
	QString nam, sts;

	int cmd = GRecvInt();
	switch (cmd) {
	case G_Ch_AddHost:
	case G_Ch_ChangeHost:
		id = GRecvInt();
		nam = recvStr();
		sts = recvStr();
		GRecvInt(); /* swallow willing for now */
		if (cmd == G_Ch_AddHost)
			new ChooserListViewItem( host_view, id, nam, sts );
		else {
			QTreeWidgetItem *itm = findItem( id );
			itm->setText( 0, nam );
			itm->setText( 1, sts );
		}
		break;
	case G_Ch_RemoveHost:
		delete findItem( GRecvInt() );
		break;
	case G_Ch_BadHost:
		KFMsgBox::box( this, QMessageBox::Warning, i18n("Unknown host %1", recvStr() ) );
		break;
	case G_Ch_Exit:
		done( ex_exit );
		break;
	default: /* XXX huuh ...? */
		break;
	}
}

#include "kchooser.moc"
