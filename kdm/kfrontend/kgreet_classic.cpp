    /*

    Conversation widget for kdm greeter

    Copyright (C) 1997, 1998, 2000 Steffen Hansen <hansen@kde.org>
    Copyright (C) 2000-2003 Oswald Buddenhagen <ossi@kde.org>


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
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

    */

#include "kgreet_classic.h"

#include <klocale.h>
#include <kpassdlg.h>
#include <kuser.h>

#include <qlayout.h>
#include <qlabel.h>

KClassicGreeter::KClassicGreeter(
	KGreeterPluginHandler *_handler, QWidget *parent, QWidget *predecessor,
	const QString &_fixedEntity, Function _func, Context _ctx ) :
    QObject(),
    KGreeterPlugin( _handler ),
    fixedUser( _fixedEntity ),
    func( _func ),
    ctx( _ctx ),
    running( false ),
    suspended( false )
{
    QGridLayout *grid = new QGridLayout( 0, 0, 10 );
    layoutItem = grid;
    QWidget *pred = predecessor;
    int line = 0;
    int echoMode = handler->gplugGetConf( "EchoMode", QVariant( -1 ) ).toInt();

    loginLabel = passwdLabel = passwd1Label = passwd2Label = 0;
    loginEdit = 0;
    passwdEdit = passwd1Edit = passwd2Edit = 0;
    if (ctx == ExUnlock || ctx == ExChangeTok)
	fixedUser = KUser().loginName();
    if (func != ChAuthTok) {
	if (fixedUser.isEmpty()) {
	    loginEdit = new QLineEdit( parent );
	    loginLabel = new QLabel( loginEdit, i18n("&Username:"), parent );
	    connect( loginEdit, SIGNAL(lostFocus()), SLOT(slotLoginLostFocus()) );
	    if (pred) {
		parent->setTabOrder( pred, loginEdit );
		pred = loginEdit;
	    }
	    grid->addWidget( loginLabel, line, 0 );
	    grid->addWidget( loginEdit, line++, 1 );
	} else if (ctx != Login && ctx != Shutdown) {
	    loginLabel = new QLabel( i18n("Username:"), parent );
	    grid->addWidget( loginLabel, line, 0 );
	    grid->addWidget( new QLabel( fixedUser, parent ), line++, 1 );
	}
	if (echoMode == -1)
	    passwdEdit = new KPasswordEdit( parent, 0 );
	else
	    passwdEdit = new KPasswordEdit( (KPasswordEdit::EchoModes)echoMode, parent, 0 );
	passwdLabel = new QLabel( passwdEdit,
	    func == Authenticate ? i18n("&Password:") : i18n("Current &password:"), parent );
	if (pred) {
	    parent->setTabOrder( pred, passwdEdit );
	    pred = passwdEdit;
	}
	grid->addWidget( passwdLabel, line, 0 );
	grid->addWidget( passwdEdit, line++, 1 );
    }
    if (func != Authenticate) {
	if (echoMode == -1) {
	    passwd1Edit = new KPasswordEdit( (KPasswordEdit::EchoModes)echoMode, parent, 0 );
	    passwd2Edit = new KPasswordEdit( (KPasswordEdit::EchoModes)echoMode, parent, 0 );
	} else {
	    passwd1Edit = new KPasswordEdit( parent, 0 );
	    passwd2Edit = new KPasswordEdit( parent, 0 );
	}
	passwd1Label = new QLabel( passwd1Edit, i18n("&New password:"), parent );
	passwd2Label = new QLabel( passwd2Edit, i18n("Con&firm password:"), parent );
	if (pred) {
	    parent->setTabOrder( pred, passwd1Edit );
	    parent->setTabOrder( passwd1Edit, passwd2Edit );
	}
	grid->addWidget( passwd1Label, line, 0 );
	grid->addWidget( passwd1Edit, line++, 1 );
	grid->addWidget( passwd2Label, line, 0 );
	grid->addWidget( passwd2Edit, line, 1 );
    }

    QLayoutIterator it = static_cast<QLayout *>(layoutItem)->iterator();
    for (QLayoutItem *itm = it.current(); itm; itm = ++it)
	 itm->widget()->show();
}

// virtual
KClassicGreeter::~KClassicGreeter()
{
    abort();
    QLayoutIterator it = static_cast<QLayout *>(layoutItem)->iterator();
    for (QLayoutItem *itm = it.current(); itm; itm = ++it)
	 delete itm->widget();
    delete layoutItem;
}

void // virtual 
KClassicGreeter::presetEntity( const QString &entity, int field )
{
    loginEdit->setText( entity );
    if (field)
	passwdEdit->setFocus();
    else {
	loginEdit->selectAll();
	loginEdit->setFocus();
    }
    curUser = entity;
    handler->gplugSetUser( entity );
}

QString // virtual 
KClassicGreeter::getEntity() const
{
    return loginEdit->text();
}

void // virtual
KClassicGreeter::setUser( const QString &user )
{
    // assert (fixedUser.isEmpty());
    curUser = user;
    loginEdit->setText( user );
}

void // virtual
KClassicGreeter::setEnabled( bool enable )
{
    // assert (!passwd1Label);
    if (loginLabel)
	loginLabel->setEnabled( enable );
    passwdLabel->setEnabled( enable );
    setActive( enable );
}

void // private
KClassicGreeter::returnData()
{
    switch (exp++) {
    case 0:
	handler->gplugReturnText(
	    (loginEdit ? loginEdit->text() : fixedUser).local8Bit(),
	    KGreeterPluginHandler::IsUser );
	break;
    case 1:
	handler->gplugReturnText( passwdEdit->password(),
				  KGreeterPluginHandler::IsPassword );
	break;
    case 2:
	handler->gplugReturnText( passwd1Edit->password(), 0 );
	break;
    default: // case 3:
	handler->gplugReturnText( passwd2Edit->password(),
				  KGreeterPluginHandler::IsPassword );
	break;
    }
}

void // virtual
KClassicGreeter::textPrompt( const char *, bool, bool nonBlocking )
{
    if (has >= exp || nonBlocking)
	returnData();
}

void // virtual
KClassicGreeter::binaryPrompt( const char *, bool )
{
    // this simply cannot happen ... :}
}

void // virtual
KClassicGreeter::start()
{
    if (passwdEdit && passwdEdit->isEnabled()) {
	if (loginEdit) {
	    if (!loginEdit->hasFocus() && !passwdEdit->hasFocus()) {
		if (!loginEdit->text().isEmpty())
		    passwdEdit->setFocus();
		else
		    loginEdit->setFocus();
	    }
	} else
	    passwdEdit->setFocus();
	authTok = false;
	if (func == Authenticate || ctx == ChangeTok || ctx == ExChangeTok)
	    exp = -1;
	else
	    exp = 0;
    } else {
	if (running) { // what a hack ... PAM sucks.
	    passwd1Edit->erase();
	    passwd2Edit->erase();
	}
	passwd1Edit->setFocus();
	authTok = true;
	exp = 2;
    }
    has = -1;
    running = true;
}

void // virtual
KClassicGreeter::suspend()
{
    // assert( running && !cont );
    abort();
    suspended = true;
}

void // virtual
KClassicGreeter::resume()
{
    // assert( suspended );
    suspended = false;
}

void // virtual
KClassicGreeter::next()
{
    // assert( running );
    if (loginEdit && loginEdit->hasFocus()) {
	passwdEdit->setFocus(); // will cancel running login if necessary
	has = 0;
    } else if (passwdEdit && passwdEdit->hasFocus()) {
	if (passwd1Edit)
	    passwd1Edit->setFocus();
	has = 1;
    } else if (passwd1Edit) {
	if (passwd1Edit->hasFocus()) {
	    passwd2Edit->setFocus();
	    has = 1; // sic!
	} else
	    has = 3;
    } else
	has = 1;
    if (exp < 0) {
	exp = authTok ? 2 : 0;
	handler->gplugStart( "classic" );
    } else if (has >= exp)
	returnData();
}

void
KClassicGreeter::abort()
{
    // assert( running );
    if (exp >= 0) {
	exp = -1;
	handler->gplugReturnText( 0, 0 );
    }
}

void // virtual
KClassicGreeter::succeeded()
{
    // assert( running );
    if (!authTok) {
	setActive( false );
	if (passwd1Edit) {
	    authTok = true;
	    return;
	}
    } else
	setActive2( false );
    exp = -1;
    running = false;
}

void // virtual
KClassicGreeter::failed()
{
    // assert( running );
    setActive( false );
    setActive2( false );
    exp = -1;
    running = false;
}

void // virtual
KClassicGreeter::revive()
{
    // assert( !running );
    if (authTok) {
	passwd1Edit->erase();
	passwd2Edit->erase();
    } else {
	passwdEdit->erase();
	setActive( true );
    }
    setActive2( true );
}

void // virtual
KClassicGreeter::clear()
{
    // in fixedUser mode this is only called after errors, not during auth.
    // won't be called during password change.
    abort();
    if (loginEdit)
	loginEdit->clear();
    passwdEdit->erase();
    if (running && !suspended) {
	if (loginEdit)
	    loginEdit->setFocus();
	else
	    passwdEdit->setFocus();
    }
}


// private

void
KClassicGreeter::setActive( bool enable )
{
    if (loginEdit)
	loginEdit->setEnabled( enable );
    if (passwdEdit)
	passwdEdit->setEnabled( enable );
}

void
KClassicGreeter::setActive2( bool enable )
{
    if (passwd1Edit) {
	passwd1Edit->setEnabled( enable );
	passwd2Edit->setEnabled( enable );
    }
}

void
KClassicGreeter::slotLoginLostFocus()
{
    if (exp > 0) {
	if (curUser == loginEdit->text())
	    return;
	exp = -1;
	handler->gplugReturnText( 0, 0 );
    }
    curUser = loginEdit->text();
    handler->gplugSetUser( curUser );
}


// factory

static KGreeterPlugin *
create_kclassicgreeter(
    KGreeterPluginHandler *handler, QWidget *parent, QWidget *predecessor,
    const QString &fixedEntity,
    KGreeterPlugin::Function func,
    KGreeterPlugin::Context ctx )
{
    return new KClassicGreeter( handler, parent, predecessor, fixedEntity, func, ctx );
}

kgreeterplugin_info kgreeterplugin_info = {
    I18N_NOOP("Username + password (classic)"), kgreeterplugin_info::Local, 0, 0, create_kclassicgreeter
};

#include "kgreet_classic.moc"
