/*
 * This file was generated by dbusidl2cpp version 0.5
 * when processing input file org.kde.kdesktop.KScreensaver.xml
 *
 * dbusidl2cpp is Copyright (C) 2006 Trolltech AS. All rights reserved.
 *
 * This is an auto-generated file.
 */

#include "kscreensaveradaptor.h"
#include <QtCore/QMetaObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>

/*
 * Implementation of adaptor class KScreensaverAdaptor
 */

KScreensaverAdaptor::KScreensaverAdaptor(QObject *parent)
   : QDBusAbstractAdaptor(parent)
{
    // constructor
    setAutoRelaySignals(true);
}

KScreensaverAdaptor::~KScreensaverAdaptor()
{
    // destructor
}

void KScreensaverAdaptor::configure()
{
    // handle method call org.kde.kdesktop.KScreensaver.configure
    QMetaObject::invokeMethod(parent(), "configure");

    // Alternative:
    //static_cast<YourObjectType *>(parent())->configure();
}

bool KScreensaverAdaptor::enable(bool e)
{
    // handle method call org.kde.kdesktop.KScreensaver.enable
    bool out0;
    QMetaObject::invokeMethod(parent(), "enable", Q_RETURN_ARG(bool, out0), Q_ARG(bool, e));

    // Alternative:
    //out0 = static_cast<YourObjectType *>(parent())->enable(e);
    return out0;
}

bool KScreensaverAdaptor::isBlanked()
{
    // handle method call org.kde.kdesktop.KScreensaver.isBlanked
    bool out0;
    QMetaObject::invokeMethod(parent(), "isBlanked", Q_RETURN_ARG(bool, out0));

    // Alternative:
    //out0 = static_cast<YourObjectType *>(parent())->isBlanked();
    return out0;
}

bool KScreensaverAdaptor::isEnabled()
{
    // handle method call org.kde.kdesktop.KScreensaver.isEnabled
    bool out0;
    QMetaObject::invokeMethod(parent(), "isEnabled", Q_RETURN_ARG(bool, out0));

    // Alternative:
    //out0 = static_cast<YourObjectType *>(parent())->isEnabled();
    return out0;
}

void KScreensaverAdaptor::lock()
{
    // handle method call org.kde.kdesktop.KScreensaver.lock
    QMetaObject::invokeMethod(parent(), "lock");

    // Alternative:
    //static_cast<YourObjectType *>(parent())->lock();
}

void KScreensaverAdaptor::quit()
{
    // handle method call org.kde.kdesktop.KScreensaver.quit
    QMetaObject::invokeMethod(parent(), "quit");

    // Alternative:
    //static_cast<YourObjectType *>(parent())->quit();
}

void KScreensaverAdaptor::save()
{
    // handle method call org.kde.kdesktop.KScreensaver.save
    QMetaObject::invokeMethod(parent(), "save");

    // Alternative:
    //static_cast<YourObjectType *>(parent())->save();
}

void KScreensaverAdaptor::saverLockReady()
{
    // handle method call org.kde.kdesktop.KScreensaver.saverLockReady
    QMetaObject::invokeMethod(parent(), "saverLockReady");

    // Alternative:
    //static_cast<YourObjectType *>(parent())->saverLockReady();
}

void KScreensaverAdaptor::setBlankOnly(bool blankOnly)
{
    // handle method call org.kde.kdesktop.KScreensaver.setBlankOnly
    QMetaObject::invokeMethod(parent(), "setBlankOnly", Q_ARG(bool, blankOnly));

    // Alternative:
    //static_cast<YourObjectType *>(parent())->setBlankOnly(blankOnly);
}


#include "kscreensaveradaptor.moc"
