/*
 * This file was generated by dbusidl2cpp version 0.5
 * when processing input file org.kde.kdesktop.Background.xml
 *
 * dbusidl2cpp is Copyright (C) 2006 Trolltech AS. All rights reserved.
 *
 * This is an auto-generated file.
 */

#include "kbackgroundadaptor.h"
#include <QtCore/QMetaObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>

/*
 * Implementation of adaptor class BackgroundAdaptor
 */

BackgroundAdaptor::BackgroundAdaptor(QObject *parent)
   : QDBusAbstractAdaptor(parent)
{
    // constructor
    setAutoRelaySignals(true);
}

BackgroundAdaptor::~BackgroundAdaptor()
{
    // destructor
}

void BackgroundAdaptor::changeWallpaper()
{
    // handle method call org.kde.kdesktop.Background.changeWallpaper
    QMetaObject::invokeMethod(parent(), "changeWallpaper");

    // Alternative:
    //static_cast<YourObjectType *>(parent())->changeWallpaper();
}

void BackgroundAdaptor::configure()
{
    // handle method call org.kde.kdesktop.Background.configure
    QMetaObject::invokeMethod(parent(), "configure");

    // Alternative:
    //static_cast<YourObjectType *>(parent())->configure();
}

QString BackgroundAdaptor::currentWallpaper(int desk)
{
    // handle method call org.kde.kdesktop.Background.currentWallpaper
    QString out0;
    QMetaObject::invokeMethod(parent(), "currentWallpaper", Q_RETURN_ARG(QString, out0), Q_ARG(int, desk));

    // Alternative:
    //out0 = static_cast<YourObjectType *>(parent())->currentWallpaper(desk);
    return out0;
}

bool BackgroundAdaptor::isCommon()
{
    // handle method call org.kde.kdesktop.Background.isCommon
    bool out0;
    QMetaObject::invokeMethod(parent(), "isCommon", Q_RETURN_ARG(bool, out0));

    // Alternative:
    //out0 = static_cast<YourObjectType *>(parent())->isCommon();
    return out0;
}

bool BackgroundAdaptor::isExport()
{
    // handle method call org.kde.kdesktop.Background.isExport
    bool out0;
    QMetaObject::invokeMethod(parent(), "isExport", Q_RETURN_ARG(bool, out0));

    // Alternative:
    //out0 = static_cast<YourObjectType *>(parent())->isExport();
    return out0;
}

void BackgroundAdaptor::setBackgroundEnabled(bool enable)
{
    // handle method call org.kde.kdesktop.Background.setBackgroundEnabled
    QMetaObject::invokeMethod(parent(), "setBackgroundEnabled", Q_ARG(bool, enable));

    // Alternative:
    //static_cast<YourObjectType *>(parent())->setBackgroundEnabled(enable);
}

void BackgroundAdaptor::setCache(int bLinit, int size)
{
    // handle method call org.kde.kdesktop.Background.setCache
    QMetaObject::invokeMethod(parent(), "setCache", Q_ARG(int, bLinit), Q_ARG(int, size));

    // Alternative:
    //static_cast<YourObjectType *>(parent())->setCache(bLinit, size);
}

void BackgroundAdaptor::setCommon(int common)
{
    // handle method call org.kde.kdesktop.Background.setCommon
    QMetaObject::invokeMethod(parent(), "setCommon", Q_ARG(int, common));

    // Alternative:
    //static_cast<YourObjectType *>(parent())->setCommon(common);
}

void BackgroundAdaptor::setExport(int xport)
{
    // handle method call org.kde.kdesktop.Background.setExport
    QMetaObject::invokeMethod(parent(), "setExport", Q_ARG(int, xport));

    // Alternative:
    //static_cast<YourObjectType *>(parent())->setExport(xport);
}

void BackgroundAdaptor::setWallpaper(const QString &wallpaper, int mode)
{
    // handle method call org.kde.kdesktop.Background.setWallpaper
    QMetaObject::invokeMethod(parent(), "setWallpaper", Q_ARG(QString, wallpaper), Q_ARG(int, mode));

    // Alternative:
    //static_cast<YourObjectType *>(parent())->setWallpaper(wallpaper, mode);
}

void BackgroundAdaptor::setWallpaper(int desk, const QString &wallpaper, int mode)
{
    // handle method call org.kde.kdesktop.Background.setWallpaper
    QMetaObject::invokeMethod(parent(), "setWallpaper", Q_ARG(int, desk), Q_ARG(QString, wallpaper), Q_ARG(int, mode));

    // Alternative:
    //static_cast<YourObjectType *>(parent())->setWallpaper(desk, wallpaper, mode);
}

QStringList BackgroundAdaptor::wallpaperFiles(int desk)
{
    // handle method call org.kde.kdesktop.Background.wallpaperFiles
    QStringList out0;
    QMetaObject::invokeMethod(parent(), "wallpaperFiles", Q_RETURN_ARG(QStringList, out0), Q_ARG(int, desk));

    // Alternative:
    //out0 = static_cast<YourObjectType *>(parent())->wallpaperFiles(desk);
    return out0;
}

QStringList BackgroundAdaptor::wallpaperList(int desk)
{
    // handle method call org.kde.kdesktop.Background.wallpaperList
    QStringList out0;
    QMetaObject::invokeMethod(parent(), "wallpaperList", Q_RETURN_ARG(QStringList, out0), Q_ARG(int, desk));

    // Alternative:
    //out0 = static_cast<YourObjectType *>(parent())->wallpaperList(desk);
    return out0;
}


#include "kbackgroundadaptor.moc"
