/*
 * This file was generated by dbusxml2cpp version 0.6
 * Command line was: dbusxml2cpp -N -m -p nm-ip4-configinterface /space/kde/sources/trunk/KDE/kdebase/workspace/solid/networkmanager-0.7/dbus/introspection/nm-ip4-config.xml
 *
 * dbusxml2cpp is Copyright (C) 2006 Trolltech ASA. All rights reserved.
 *
 * This is an auto-generated file.
 * Do not edit! All changes made to it will be lost.
 */

#ifndef NM_IP4_CONFIGINTERFACE_H_1207427199
#define NM_IP4_CONFIGINTERFACE_H_1207427199

#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtDBus/QtDBus>

/*
 * Proxy class for interface org.freedesktop.NetworkManager.IP4Config
 */
class OrgFreedesktopNetworkManagerIP4ConfigInterface: public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName()
    { return "org.freedesktop.NetworkManager.IP4Config"; }

public:
    OrgFreedesktopNetworkManagerIP4ConfigInterface(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = 0);

    ~OrgFreedesktopNetworkManagerIP4ConfigInterface();

    Q_PROPERTY(uint Address READ address)
    inline uint address() const
    { return qvariant_cast< uint >(internalPropGet("Address")); }

    Q_PROPERTY(uint Broadcast READ broadcast)
    inline uint broadcast() const
    { return qvariant_cast< uint >(internalPropGet("Broadcast")); }

    Q_PROPERTY(QStringList Domains READ domains)
    inline QStringList domains() const
    { return qvariant_cast< QStringList >(internalPropGet("Domains")); }

    Q_PROPERTY(uint Gateway READ gateway)
    inline uint gateway() const
    { return qvariant_cast< uint >(internalPropGet("Gateway")); }

    Q_PROPERTY(QString Hostname READ hostname)
    inline QString hostname() const
    { return qvariant_cast< QString >(internalPropGet("Hostname")); }

    Q_PROPERTY(UIntList Nameservers READ nameservers)
    inline UIntList nameservers() const
    { return qvariant_cast< UIntList >(internalPropGet("Nameservers")); }

    Q_PROPERTY(uint Netmask READ netmask)
    inline uint netmask() const
    { return qvariant_cast< uint >(internalPropGet("Netmask")); }

    Q_PROPERTY(QString NisDomain READ nisDomain)
    inline QString nisDomain() const
    { return qvariant_cast< QString >(internalPropGet("NisDomain")); }

    Q_PROPERTY(UIntList NisServers READ nisServers)
    inline UIntList nisServers() const
    { return qvariant_cast< UIntList >(internalPropGet("NisServers")); }

public Q_SLOTS: // METHODS
Q_SIGNALS: // SIGNALS
};

#endif
