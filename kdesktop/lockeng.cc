//===========================================================================
//
// This file is part of the KDE project
//
// Copyright (c) 1999 Martin R. Jones <mjones@kde.org>
//


#include <config.h>


#include "lockeng.h"
#include "lockeng.moc"
#include "kdesktopsettings.h"
#include "kscreensaveradaptor.h"

#include <kstandarddirs.h>
#include <kapplication.h>
#include <kservicegroup.h>
#include <kdebug.h>
#include <klocale.h>
#include <QFile>
#include <QX11Info>
#include <assert.h>
#include <stdlib.h>

#include "xautolock_c.h"
extern xautolock_corner_t xautolock_corners[ 4 ];

const char* SaverEngine::screenSaverDBusObjectPath = "/ScreenSaver";

//===========================================================================
//
// Screen saver engine. Doesn't handle the actual screensaver window,
// starting screensaver hacks, or password entry. That's done by
// a newly started process.
//
SaverEngine::SaverEngine()
    : QWidget(),
      mBlankOnly(false)
{

    (void) new KScreenSaverAdaptor( this );
    QDBus::sessionBus().registerObject( screenSaverDBusObjectPath, this );

    // Save X screensaver parameters
    XGetScreenSaver(QX11Info::display(), &mXTimeout, &mXInterval,
                    &mXBlanking, &mXExposures);

    mState = Waiting;
    mXAutoLock = 0;
    mEnabled = false;

    connect(&mLockProcess, SIGNAL(processExited(KProcess *)),
                        SLOT(lockProcessExited()));

    configure();
}

//---------------------------------------------------------------------------
//
// Destructor - usual cleanups.
//
SaverEngine::~SaverEngine()
{
    mLockProcess.detach(); // don't kill it if we crash
    delete mXAutoLock;

    // Restore X screensaver parameters
    XSetScreenSaver(QX11Info::display(), mXTimeout, mXInterval, mXBlanking,
                    mXExposures);
}

//---------------------------------------------------------------------------

// This should be called only using DBus.
void SaverEngine::lock()
{
    bool ok = true;
    if (mState == Waiting)
    {
        ok = startLockProcess( ForceLock );
    }
// It takes a while for kdesktop_lock to start and lock the screen.
// Therefore delay the DBus call until it tells kdesktop that the locking is in effect.
// This is done only for --forcelock .
    if( ok && mState != Saving )
    {
#ifdef __GNUC__
#warning port dcop transactions to dbus
#endif
#if 0
        DCOPClientTransaction* trans = kapp->dcopClient()->beginTransaction();
        mLockTransactions.append( trans );
#endif
    }
}

void SaverEngine::processLockTransactions()
{
#ifdef __GNUC__
#warning port dcop transactions to dbus
#endif
#if 0
    for( QVector< DCOPClientTransaction* >::ConstIterator it = mLockTransactions.begin();
         it != mLockTransactions.end();
         ++it )
    {
        DCOPCString replyType = "void";
        QByteArray arr;
        kapp->dcopClient()->endTransaction( *it, replyType, arr );
    }
    mLockTransactions.clear();
#endif
}

void SaverEngine::saverLockReady()
{
    if( mState != Preparing )
    {
	kDebug( 1204 ) << "Got unexpected saverReady()" << endl;
    }
    kDebug( 1204 ) << "Saver Lock Ready" << endl;
    processLockTransactions();
}

//---------------------------------------------------------------------------
void SaverEngine::save()
{
    if (mState == Waiting)
    {
        startLockProcess( DefaultLock );
    }
}

//---------------------------------------------------------------------------
void SaverEngine::quit()
{
    if (mState == Saving || mState == Preparing)
    {
        stopLockProcess();
    }
}

//---------------------------------------------------------------------------
bool SaverEngine::isEnabled()
{
  return mEnabled;
}

//---------------------------------------------------------------------------
bool SaverEngine::enable( bool e )
{
    if ( e == mEnabled )
	return true;

    // If we aren't in a suitable state, we will not reconfigure.
    if (mState != Waiting)
        return false;

    mEnabled = e;

    if (mEnabled)
    {
	if ( !mXAutoLock ) {
	    mXAutoLock = new XAutoLock();
	    connect(mXAutoLock, SIGNAL(timeout()), SLOT(idleTimeout()));
	}
        mXAutoLock->setTimeout(mTimeout);
        mXAutoLock->setDPMS(mDPMS);
	//mXAutoLock->changeCornerLockStatus( mLockCornerTopLeft, mLockCornerTopRight, mLockCornerBottomLeft, mLockCornerBottomRight);

        // We'll handle blanking
        XSetScreenSaver(QX11Info::display(), mTimeout + 10, mXInterval, mXBlanking, mXExposures);

        mXAutoLock->start();

        kDebug(1204) << "Saver Engine started, timeout: " << mTimeout << endl;
    }
    else
    {
	if (mXAutoLock)
	{
	    delete mXAutoLock;
	    mXAutoLock = 0;
	}

	XSetScreenSaver(QX11Info::display(), 0, mXInterval, mXBlanking, mXExposures);
        kDebug(1204) << "Saver Engine disabled" << endl;
    }

    return true;
}

//---------------------------------------------------------------------------
bool SaverEngine::isBlanked()
{
  return (mState != Waiting);
}

//---------------------------------------------------------------------------
//
// Read and apply configuration.
//
void SaverEngine::configure()
{
    // If we aren't in a suitable state, we will not reconfigure.
    if (mState != Waiting)
        return;

    // create a new config obj to ensure we read the latest options
    KDesktopSettings::self()->readConfig();

    bool e  = KDesktopSettings::screenSaverEnabled();
    mTimeout = KDesktopSettings::timeout();
    mDPMS = KDesktopSettings::dpmsDependent();

    mEnabled = !e;   // force the enable()

    int action;
    action = KDesktopSettings::actionTopLeft();
    xautolock_corners[0] = applyManualSettings(action);
    action = KDesktopSettings::actionTopRight();
    xautolock_corners[1] = applyManualSettings(action);
    action = KDesktopSettings::actionBottomLeft();
    xautolock_corners[2] = applyManualSettings(action);
    action = KDesktopSettings::actionBottomRight();
    xautolock_corners[3] = applyManualSettings(action);

    enable( e );
}

//---------------------------------------------------------------------------
//
//  Set a variable to indicate only using the blanker and not the saver.
//
void SaverEngine::setBlankOnly( bool blankOnly )
{
	mBlankOnly = blankOnly;
	// FIXME: if running, stop  and restart?  What about security
	// implications of this?
}

//---------------------------------------------------------------------------
//
// Start the screen saver.
//
bool SaverEngine::startLockProcess( LockType lock_type )
{
    if (mState != Waiting)
        return true;

    kDebug(1204) << "SaverEngine: starting saver" << endl;
    emit screenSaverStarted(); // DBus signal

    if (mLockProcess.isRunning())
    {
        stopLockProcess();
    }
    mLockProcess.clearArguments();
    QString path = KStandardDirs::findExe( "kdesktop_lock" );
    if( path.isEmpty())
    {
	kDebug( 1204 ) << "Can't find kdesktop_lock!" << endl;
	return false;
    }
    mLockProcess << path;
    switch( lock_type )
    {
	case ForceLock:
    	    mLockProcess << QString( "--forcelock" );
	  break;
	case DontLock:
	    mLockProcess << QString( "--dontlock" );
	  break;
	default:
	  break;
    }
    if (mBlankOnly)
	    mLockProcess << QString( "--blank" );

    if (mLockProcess.start() == false )
    {
	kDebug( 1204 ) << "Failed to start kdesktop_lock!" << endl;
	return false;
    }

    mState = Preparing;
    if (mXAutoLock)
    {
        mXAutoLock->stop();
    }
    return true;
}

//---------------------------------------------------------------------------
//
// Stop the screen saver.
//
void SaverEngine::stopLockProcess()
{
    if (mState == Waiting)
    {
        kWarning(1204) << "SaverEngine::stopSaver() saver not active" << endl;
        return;
    }
    kDebug(1204) << "SaverEngine: stopping lock" << endl;
    emit screenSaverStopped(); // DBus signal

    mLockProcess.kill();

    if (mXAutoLock)
    {
        mXAutoLock->start();
    }
    processLockTransactions();
    mState = Waiting;
    XSetScreenSaver(QX11Info::display(), mTimeout + 10, mXInterval, mXBlanking, mXExposures);
}

void SaverEngine::lockProcessExited()
{
    kDebug(1204) << "SaverEngine: lock exited" << endl;
    if( mState == Waiting )
	return;
    emit screenSaverStopped(); // DBus signal
    if (mXAutoLock)
    {
        mXAutoLock->start();
    }
    processLockTransactions();
    mState = Waiting;
    XSetScreenSaver(QX11Info::display(), mTimeout + 10, mXInterval, mXBlanking, mXExposures);
}

//---------------------------------------------------------------------------
//
// XAutoLock has detected the required idle time.
//
void SaverEngine::idleTimeout()
{
    // disable X screensaver
    XSetScreenSaver(QX11Info::display(), 0, mXInterval, mXBlanking, mXExposures);
    startLockProcess( DefaultLock );
}

xautolock_corner_t SaverEngine::applyManualSettings(int action)
{
	if (action == 0)
	{
		kDebug() << "no lock" << endl;
		return ca_nothing;
	}
	else
	if (action == 1)
	{
		kDebug() << "lock screen" << endl;
		return ca_forceLock;
	}
	else
	if (action == 2)
	{
		kDebug() << "prevent lock" << endl;
		return ca_dontLock;
	}
	else
	{
		kDebug() << "no lock nothing" << endl;
		return ca_nothing;
	}
}
