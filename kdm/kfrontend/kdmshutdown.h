    /*

    Shutdown dialog

    Copyright (C) 1997, 1998 Steffen Hansen <hansen@kde.org>
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


#ifndef KDMSHUTDOWN_H
#define KDMSHUTDOWN_H

#include "kgverify.h"

#include <qradiobutton.h>

class LiloInfo;
class QLabel;
class KPushButton;
class QButtonGroup;
class QGroupBox;
class QComboBox;
class QCheckBox;
class QLineEdit;

enum { Authed = QDialog::Accepted + 1, Schedule };

class KDMShutdownBase : public FDialog, public KGVerifyHandler {
    Q_OBJECT
    typedef FDialog inherited;

public:
    KDMShutdownBase( int _uid, QWidget *_parent );
    virtual ~KDMShutdownBase();

protected slots:
    virtual void accept();

protected:
    virtual void accepted();

protected:
    void updateNeedRoot();
    void complete( QWidget *prevWidget );

    QVBoxLayout		*box;
    bool		mayNuke, doesNuke, mayOk, maySched;

private slots:
    void slotSched();
    void slotActivatePlugMenu();

private:
    KPushButton		*okButton, *cancelButton;
    QLabel		*rootlab;
    KGVerify		*verify;
    int 		needRoot, uid;

    static int		curPlugin;
    static PluginList	pluginList;

public: // from KGVerifyHandler
    virtual void verifyPluginChanged( int id );
    virtual void verifyOk();
    virtual void verifyFailed();
    virtual void verifyRetry();
    virtual void verifySetUser( const QString &user );
};


#if defined(__linux__) && (defined(__i386__) || defined(__amd64__))
class LiloHandler {
public:
    LiloHandler();
    ~LiloHandler();
    void setupTargets( QWidget *parent );
    void applyTarget();

    LiloInfo		*liloInfo;
    QComboBox		*targets;
    int			defaultLiloTarget, oldLiloTarget;
};
#endif

class KDMShutdown : public KDMShutdownBase
#if defined(__linux__) && (defined(__i386__) || defined(__amd64__))
		  , public LiloHandler
#endif
{
    Q_OBJECT
    typedef KDMShutdownBase inherited;

public:
    KDMShutdown( int _uid, QWidget *_parent = 0 );
    static void scheduleShutdown( QWidget *_parent = 0 );

protected slots:
    virtual void accept();

protected:
    virtual void accepted();

private slots:
    void slotTargetChanged();
    void slotWhenChanged();

private:
    QButtonGroup	*howGroup;
    QGroupBox		*schedGroup;
    QRadioButton	*restart_rb;
    QLineEdit		*le_start, *le_timeout;
    QCheckBox		*cb_force;
    int 		sch_st, sch_to;

};

class KDMRadioButton : public QRadioButton
{
    Q_OBJECT
    typedef QRadioButton inherited;

public:
    KDMRadioButton( const QString &label, QWidget *parent );

private:
    virtual void mouseDoubleClickEvent( QMouseEvent * );

signals:
    void doubleClicked();

};

class KDMSlimShutdown : public FDialog
#if defined(__linux__) && (defined(__i386__) || defined(__amd64__))
		      , public LiloHandler
#endif
{
    Q_OBJECT
    typedef FDialog inherited;

public:
    KDMSlimShutdown( QWidget *_parent = 0 );
    static void externShutdown( int type, int uid );

private slots:
    void slotHalt();
    void slotReboot();
    void slotSched();

private:
    bool checkShutdown();

};

class KDMConfShutdown : public KDMShutdownBase {
    Q_OBJECT
    typedef KDMShutdownBase inherited;

public:
    KDMConfShutdown( int _uid, struct dpySpec *sess, QWidget *_parent = 0 );
};

class KDMCancelShutdown : public KDMShutdownBase {
    Q_OBJECT
    typedef KDMShutdownBase inherited;

public:
    KDMCancelShutdown(
	int how, int start, int timeout, int force, int uid,
	QWidget *_parent );
};

#endif /* KDMSHUTDOWN_H */
