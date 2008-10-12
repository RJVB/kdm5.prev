/*  This file is part of the KDE project
    Copyright (C) 2007 Will Stephenson <wstephenson@kde.org>
    Copyright (C) 2007 Daniel Gollub <dgollub@suse.de>
    Copyright (C) 2008 Tom Patzig <tpatzig@suse.de>


    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

*/

#ifndef BLUEZ_BLUETOOTHINTERFACE_H
#define BLUEZ_BLUETOOTHINTERFACE_H

#include <kdemacros.h>
#include <QtDBus>
#include <QVariant>
#include <QDBusVariant>
#include <QDBusObjectPath>
#include <QMetaType>

#include <solid/control/ifaces/bluetoothinterface.h>

class BluezBluetoothInterfacePrivate;


class KDE_EXPORT BluezBluetoothInterface : public Solid::Control::Ifaces::BluetoothInterface
{
    Q_OBJECT
    Q_INTERFACES(Solid::Control::Ifaces::BluetoothInterface)
    
public:
    BluezBluetoothInterface(const QString  & objectPath);
    virtual ~BluezBluetoothInterface();
    QString ubi() const;
/*
    QString address() const;
    QString version() const;
    QString revision() const;
    QString manufacturer() const;
    QString company() const;
    Solid::Control::BluetoothInterface::Mode mode() const;
    int discoverableTimeout() const;
    bool isDiscoverable() const;
    QStringList listConnections() const;
    QString majorClass() const;
    QStringList listAvailableMinorClasses() const;
    QString minorClass() const;
    QStringList serviceClasses() const;
    QString name() const;
    QStringList listBondings() const;
    bool isPeriodicDiscoveryActive() const;
    bool isPeriodicDiscoveryNameResolvingActive() const;
    QStringList listRemoteDevices() const;
    QStringList listRecentRemoteDevices(const QDateTime &) const;
    QString getRemoteName(const QString &);
    bool isTrusted(const QString &);

    QObject *createBluetoothRemoteDevice(const QString &);
*/    
    QString createDevice(const QString &) const;
    QString createPairedDevice(const QString &,const QString &,const QString &) const;
    QString findDevice(const QString &) const;
    QMap< QString, QVariant > getProperties() const;
    QStringList listDevices() const;


public Q_SLOTS:

    void cancelDeviceCreation(const QString &) const;
    void registerAgent(const QString &,const QString &) const;
    void releaseSession() const;
    void removeDevice(const QString &) const;
    void requestSession() const;
    void setProperty(const QString &, const QVariant &) const;
    void startDiscovery() const;
    void stopDiscovery() const;
    void unregisterAgent(const QString &) const;

/*
    void setMode(const Solid::Control::BluetoothInterface::Mode);
    void setDiscoverableTimeout(int);
    void setMinorClass(const QString &);
    void setName(const QString &);
    void discoverDevices();
    void discoverDevicesWithoutNameResolving();
    void cancelDiscovery();
    void startPeriodicDiscovery();
    void stopPeriodicDiscovery();
    void setPeriodicDiscoveryNameResolving(bool);
    void setTrusted(const QString &);
    void removeTrust(const QString &);

    void slotModeChanged(const Solid::Control::BluetoothInterface::Mode mode);
    void slotDiscoverableTimeoutChanged(int timeout);
    void slotMinorClassChanged(const QString &minor);
    void slotNameChanged(const QString &name);
    void slotDiscoveryStarted();
    void slotDiscoveryCompleted();
    void slotRemoteDeviceFound(const QString &ubi, uint deviceClass, short rssi);
    void slotRemoteDeviceDisappeared(const QString &ubi);
    void slotRemoteNameUpdated(const QString &, const QString &);
    void slotRemoteDeviceConnected(const QString&);
    void slotRemoteDeviceDisconnected(const QString&);
    void slotTrustAdded(const QString&);
    void slotTrustRemoved(const QString&);
    void slotBondingCreated(const QString&);
    void slotBondingRemoved(const QString&);
*/    
    void slotDeviceCreated(const QDBusObjectPath &);
    void slotDeviceDisappeared(const QString &);
    void slotDeviceFound(const QString &, const QMap< QString, QVariant > &);
    void slotDeviceRemoved(const QDBusObjectPath &);
    void slotPropertyChanged(const QString &,const QVariant &);

private:
    BluezBluetoothInterfacePrivate * d;

    QStringList listReply(const QString &method) const;
    QString stringReply(const QString &method, const QString &param = "") const;
    bool boolReply(const QString &method, const QString &param = "") const;
    QDBusObjectPath objectReply(const QString &method, const QString &param = "" ) const;
};

#endif
