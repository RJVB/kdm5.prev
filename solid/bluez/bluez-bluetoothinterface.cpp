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

#include "bluez-bluetoothinterface.h"

#include <solid/control/bluetoothinterface.h>

#include "bluez-bluetoothremotedevice.h"
#include <KDebug>



class BluezBluetoothInterfacePrivate
{
public:
    BluezBluetoothInterfacePrivate(const QString  & objPath)
            : iface("org.bluez",
                    objPath,
                    "org.bluez.Adapter",
                    QDBusConnection::systemBus()),
            objectPath(objPath)
    { }
    QDBusInterface iface;
    QString objectPath;

    QMap<QString, BluezBluetoothRemoteDevice *> devices;
};



BluezBluetoothInterface::BluezBluetoothInterface(const QString  & objectPath)
        : BluetoothInterface(0), d(new BluezBluetoothInterfacePrivate(objectPath))
{

#define connectInterfaceToThis(signal, slot) \
    d->iface.connection().connect("org.bluez", \
                                   objectPath, \
                                   "org.bluez.Adapter", \
                                   signal, this, SLOT(slot));
/*  
    connectInterfaceToThis("ModeChanged", slotModeChanged(const QString &));
    connectInterfaceToThis("DiscoverableTimeoutChanged", slotDiscoverableTimeoutChanged(int));
    connectInterfaceToThis("MinorClassChanged", slotMinorClassChanged(const QString &));
    connectInterfaceToThis("NameChanged", slotNameChanged(const QString &));
    connectInterfaceToThis("DiscoveryStarted", slotDiscoveryStarted());
    connectInterfaceToThis("DiscoveryCompleted", slotDiscoveryCompleted());
    connectInterfaceToThis("RemoteDeviceDisappeared", slotRemoteDeviceDisappeared(const QString &));
    connectInterfaceToThis("RemoteDeviceFound", slotRemoteDeviceFound(const QString &, uint, short));
    connectInterfaceToThis("RemoteNameUpdated", slotRemoteNameUpdated(const QString &, const QString &));
    connectInterfaceToThis("RemoteDeviceConnected", slotRemoteDeviceConnected(const QString &));
    connectInterfaceToThis("RemoteDeviceDisconnected", slotRemoteDeviceDisconnected(const QString &));
    connectInterfaceToThis("TrustAdded", slotTrustAdded(const QString &));
    connectInterfaceToThis("TrustRemoved", slotTrustRemoved(const QString &));
    connectInterfaceToThis("BondingCreated", slotBondingCreated(const QString &));
    connectInterfaceToThis("BondingRemoved", slotBondingRemoved(const QString &));
*/

    connectInterfaceToThis("PropertyChanged", slotPropertyChanged(const QString &, const QVariant &));
    connectInterfaceToThis("DeviceCreated", slotDeviceCreated(const QDBusObjectPath &));
    connectInterfaceToThis("DeviceRemoved", slotDeviceRemoved(const QDBusObjectPath &));
    connectInterfaceToThis("DeviceDisappeared", slotDeviceDisappeared(const QString &));
    connectInterfaceToThis("DeviceFound", slotDeviceFound(const QString &, const QMap< QString,QVariant > &));


}

BluezBluetoothInterface::~BluezBluetoothInterface()
{
    delete d;
}

QString BluezBluetoothInterface::ubi() const
{
    return d->objectPath;
}

void BluezBluetoothInterface::cancelDeviceCreation(const QString &addr)
{
    d->iface.call("CancelDeviceCreation",addr);
}

QString BluezBluetoothInterface::createDevice(const QString &addr) const
{
    QDBusObjectPath path = objectReply("CreateDevice",addr);
    return path.path();
}

QString BluezBluetoothInterface::createPairedDevice(const QString &addr, const QString &agentUBI, const QString &capab) const
{
    QDBusReply< QDBusObjectPath > reply;
    reply = d->iface.call("CreatePairedDevice",addr,agentUBI,capab);

    if (!reply.isValid()) {
        return QString();
    }
    return reply.value().path();
}

QString BluezBluetoothInterface::findDevice(const QString &addr) const
{
    QDBusObjectPath path = objectReply("FindDevice",addr);
    return path.path();
}


QMap<QString, QVariant> BluezBluetoothInterface::getProperties() const
{
    QDBusReply< QMap<QString,QVariant> > prop = d->iface.call("GetProperties");
    if (!prop.isValid()) {
        return QMap< QString,QVariant >();
    }
    return prop.value();
}

QStringList BluezBluetoothInterface::listDevices() const
{
    QStringList deviceList;

    QDBusReply< QList<QDBusObjectPath> > devices = d->iface.call("ListDevices");
    if(!devices.isValid()) {
        return QStringList();
    }
    foreach(QDBusObjectPath path, devices.value()) {
        deviceList.append(path.path());
    }
    return deviceList;
}

void BluezBluetoothInterface::registerAgent(const QString &agentUBI, const QString &capab)
{
    d->iface.call("RegisterAgent",agentUBI,capab);
}

void BluezBluetoothInterface::releaseSession()
{
    d->iface.call("ReleaseSession");
}

void BluezBluetoothInterface::removeDevice(const QString &deviceUBI )
{
    d->iface.call("RemoveDevice",deviceUBI);
}

void BluezBluetoothInterface::requestSession()
{
    d->iface.call("RequestSession");
}

void BluezBluetoothInterface::setProperty(const QString &property, const QVariant &value)
{
    d->iface.call("SetProperty",property, qVariantFromValue(QDBusVariant(value)));
}


void BluezBluetoothInterface::startDiscovery()
{
    d->iface.call("StartDiscovery");
}

void BluezBluetoothInterface::stopDiscovery()
{
    d->iface.call("StopDiscovery");
}

void BluezBluetoothInterface::unregisterAgent(const QString &agentUBI)
{
    d->iface.call("UnregisterAgent",agentUBI);
}



/*

QString BluezBluetoothInterface::address() const
{
    return stringReply("GetAddress");
}

QString BluezBluetoothInterface::version() const
{
    return stringReply("GetVersion");
}

QString BluezBluetoothInterface::revision() const
{
    return stringReply("GetRevision");
}

QString BluezBluetoothInterface::manufacturer() const
{
    return stringReply("GetManufacturer");
}

QString BluezBluetoothInterface::company() const
{
    return stringReply("GetCompany");
}

Solid::Control::BluetoothInterface::Mode BluezBluetoothInterface::mode() const
{
    QString theMode = stringReply("GetMode");
    Solid::Control::BluetoothInterface::Mode modeEnum;
    if (theMode == "connectable")
    {
        modeEnum = Solid::Control::BluetoothInterface::Connectable;
    }
    else if (theMode == "discoverable")
    {
        modeEnum = Solid::Control::BluetoothInterface::Discoverable;
    } else {
        Q_ASSERT(theMode == "off");
        modeEnum = Solid::Control::BluetoothInterface::Off;
    }
    return modeEnum;
}

int BluezBluetoothInterface::discoverableTimeout() const
{
    QDBusReply< uint > timeout = d->iface.call("GetDiscoverableTimeout");
    if (timeout.isValid()) {
        return timeout.value();
    }

    return -1;
}

bool BluezBluetoothInterface::isDiscoverable() const
{
    return boolReply("IsDiscoverable");
}

QStringList BluezBluetoothInterface::listConnections() const
{
    QStringList list = listReply("ListConnections");
    for (int i = 0; i < list.size(); i++) {
        list[i] = ubi() + '/' + list[i];
    }
    return list;
}

QString BluezBluetoothInterface::majorClass() const
{
    return stringReply("GetMajorClass");
}

QStringList BluezBluetoothInterface::listAvailableMinorClasses() const
{
    return listReply("ListAvailableMinorClasses");
}

QString BluezBluetoothInterface::minorClass() const
{
    return stringReply("GetMinorClass");
}

QStringList BluezBluetoothInterface::serviceClasses() const
{
    return listReply("GetServiceClasses");
}

QString BluezBluetoothInterface::name() const
{
    return stringReply("GetName");
}

QString BluezBluetoothInterface::getRemoteName(const QString &mac)
{
    return stringReply("GetRemoteName",mac);
}

bool BluezBluetoothInterface::isTrusted(const QString &mac)
{
   return boolReply("IsTrusted",mac);
}

QStringList BluezBluetoothInterface::listBondings() const
{
    return listReply("ListBondings");
}

bool BluezBluetoothInterface::isPeriodicDiscoveryActive() const
{
    return boolReply("IsPeriodicDiscovery");
}

bool BluezBluetoothInterface::isPeriodicDiscoveryNameResolvingActive() const
{
    return boolReply("IsPeriodicDiscoveryNameResolving");
}

QStringList BluezBluetoothInterface::listRemoteDevices() const
{
    QStringList list = listReply("ListRemoteDevices");
    for (int i = 0; i < list.size(); i++) {
        list[i] = ubi() + '/' + list[i];
    }
    return list;
}

QStringList BluezBluetoothInterface::listRecentRemoteDevices(const QDateTime &) const
{
    return listReply("ListRecentRemoteDevices");
}

void BluezBluetoothInterface::setMode(const Solid::Control::BluetoothInterface::Mode mode)
{
    QString modeString;
    switch(mode)
    {
    case Solid::Control::BluetoothInterface::Off:
        modeString = "off";
        break;
    case Solid::Control::BluetoothInterface::Discoverable:
        modeString = "discoverable";
        break;
    case Solid::Control::BluetoothInterface::Connectable:
        modeString = "connectable";
        break;
    }
    d->iface.call("SetMode", modeString);
}

void BluezBluetoothInterface::setDiscoverableTimeout(int timeout)
{
    d->iface.call("SetDiscoverableTimeout", (uint)timeout);
}

void BluezBluetoothInterface::setMinorClass(const QString &minorClass)
{
    d->iface.call("SetMinorClass", minorClass);
}

void BluezBluetoothInterface::setName(const QString &name)
{
    d->iface.call("SetName", name);
}

void BluezBluetoothInterface::discoverDevices()
{
    d->iface.call("DiscoverDevices");
}

void BluezBluetoothInterface::discoverDevicesWithoutNameResolving()
{
    d->iface.call("DiscoverDevicesWithoutNameResolving");
}

void BluezBluetoothInterface::cancelDiscovery()
{
    d->iface.call("CancelDiscovery");
}

void BluezBluetoothInterface::startPeriodicDiscovery()
{
    d->iface.call("StartPeriodicDiscovery");
}

void BluezBluetoothInterface::stopPeriodicDiscovery()
{
    d->iface.call("StopPeriodicDiscovery");
}

void BluezBluetoothInterface::setPeriodicDiscoveryNameResolving(bool nameResolving)
{
    d->iface.call("SetPeriodicDiscoveryNameResolving", nameResolving);
}

void BluezBluetoothInterface::setTrusted(const QString& mac)
{
    d->iface.call("SetTrusted", mac);
}

void BluezBluetoothInterface::removeTrust(const QString& mac)
{
    d->iface.call("RemoveTrust", mac);
}

*/



/*

void BluezBluetoothInterface::slotModeChanged(const Solid::Control::BluetoothInterface::Mode mode)
{
    emit modeChanged(mode);
}

void BluezBluetoothInterface::slotDiscoverableTimeoutChanged(int timeout)
{
    emit discoverableTimeoutChanged(timeout);
}

void BluezBluetoothInterface::slotMinorClassChanged(const QString &minorClass)
{
    emit minorClassChanged(minorClass);
}

void BluezBluetoothInterface::slotNameChanged(const QString &name)
{
    emit nameChanged(name);
}

void BluezBluetoothInterface::slotDiscoveryStarted()
{
    emit discoveryStarted();
}

void BluezBluetoothInterface::slotDiscoveryCompleted()
{
    emit discoveryCompleted();
}

void BluezBluetoothInterface::slotRemoteDeviceFound(const QString &address, uint deviceClass, short rssi)
{
    QString remoteubi = QString("%1/%2").arg(ubi()).arg(address);
    emit remoteDeviceFound(remoteubi, deviceClass, rssi);
}

void BluezBluetoothInterface::slotRemoteDeviceDisappeared(const QString &address)
{
    QString remoteubi = QString("%1/%2").arg(ubi()).arg(address);
    emit remoteDeviceDisappeared(remoteubi);
}

void BluezBluetoothInterface::slotRemoteNameUpdated(const QString &address, const QString& name)
{
   emit remoteNameUpdated(address,name);
}

void BluezBluetoothInterface::slotRemoteDeviceConnected(const QString &address)
{
   emit remoteDeviceConnected(address);
}

void BluezBluetoothInterface::slotRemoteDeviceDisconnected(const QString &address)
{
   emit remoteDeviceDisconnected(address);
}

void BluezBluetoothInterface::slotTrustAdded(const QString &address)
{
   emit trustAdded(address);
}

void BluezBluetoothInterface::slotTrustRemoved(const QString &address)
{
   emit trustRemoved(address);
}

void BluezBluetoothInterface::slotBondingCreated(const QString &address)
{
   emit bondingCreated(address);
}

void BluezBluetoothInterface::slotBondingRemoved(const QString &address)
{
   emit bondingRemoved(address);
}

*/

void BluezBluetoothInterface::slotDeviceCreated(const QDBusObjectPath &path)
{
    kDebug() << "device created";
    emit deviceCreated(path.path());
}

void BluezBluetoothInterface::slotDeviceDisappeared(const QString &address)
{
    kDebug() << "device disappeared";
    emit deviceDisappeared(address);
}

void BluezBluetoothInterface::slotDeviceFound(const QString &address, const QMap< QString, QVariant > &properties)
{
    kDebug() << "device found " << address << " " << properties["Name"];
    emit deviceFound(address,properties);
}

void BluezBluetoothInterface::slotDeviceRemoved(const QDBusObjectPath &path)
{
    kDebug() << "device removed";
    emit deviceRemoved(path.path());
}

void BluezBluetoothInterface::slotPropertyChanged(const QString & property, const QVariant &value)
{
    kDebug() << "Property " << property << " changed to " << value;
    emit propertyChanged(property,value);
}



QObject *BluezBluetoothInterface::createBluetoothRemoteDevice(const QString &ubi)
{
    BluezBluetoothRemoteDevice *bluetoothInterface;
    if (d->devices.contains(ubi)) {
        bluetoothInterface = d->devices[ubi];
    } else {
        bluetoothInterface = new BluezBluetoothRemoteDevice(ubi);
        d->devices.insert(ubi, bluetoothInterface);
    }
    return bluetoothInterface;
}


/******************* DBus Calls *******************************/

QStringList BluezBluetoothInterface::listReply(const QString &method) const
{
    QDBusReply< QStringList > list = d->iface.call(method);
    if (!list.isValid()) {
        return QStringList();
    }

    return list.value();
}

QString BluezBluetoothInterface::stringReply(const QString &method, const QString &param) const
{
    QDBusReply< QString > reply;

    if (param.isEmpty())
	    reply = d->iface.call(method);
    else
	    reply = d->iface.call(method, param);
	    	
    if (reply.isValid()) {
        return reply.value();
    }

    return QString();
}

bool BluezBluetoothInterface::boolReply(const QString &method, const QString &param) const
{
    QDBusReply< bool > reply; 

    if (param.isEmpty())
	    reply = d->iface.call(method);
    else
	    reply = d->iface.call(method, param);

    if (reply.isValid()) {
        return reply.value();
    }

    return false;
}

QDBusObjectPath BluezBluetoothInterface::objectReply(const QString &method, const QString &param) const
{
    QDBusReply< QDBusObjectPath > reply;

    if (param.isEmpty())
	    reply = d->iface.call(method);
    else
	    reply = d->iface.call(method, param);
	    	
    if (reply.isValid()) {
        return reply.value();
    }

    return QDBusObjectPath();
}

#include "bluez-bluetoothinterface.moc"
