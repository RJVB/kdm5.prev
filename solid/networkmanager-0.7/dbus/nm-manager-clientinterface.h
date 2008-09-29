/*
 * This file was generated by dbusxml2cpp version 0.6
 * Command line was: dbusxml2cpp -N -m -p nm-manager-clientinterface /space/kde/sources/trunk/KDE/kdebase/workspace/solid/networkmanager-0.7/dbus/introspection/nm-manager-client.xml
 *
 * dbusxml2cpp is Copyright (C) 2006 Trolltech ASA. All rights reserved.
 *
 * This is an auto-generated file.
 * Do not edit! All changes made to it will be lost.
 */

#ifndef NM_MANAGER_CLIENTINTERFACE_H_1222729762
#define NM_MANAGER_CLIENTINTERFACE_H_1222729762

#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtDBus/QtDBus>

/*
 * Proxy class for interface org.freedesktop.NetworkManager
 */
class OrgFreedesktopNetworkManagerInterface: public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName()
    { return "org.freedesktop.NetworkManager"; }

public:
    OrgFreedesktopNetworkManagerInterface(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = 0);

    ~OrgFreedesktopNetworkManagerInterface();

    Q_PROPERTY(QList<QDBusObjectPath> ActiveConnections READ activeConnections)
    inline QList<QDBusObjectPath> activeConnections() const
    { return qvariant_cast< QList<QDBusObjectPath> >(internalPropGet("ActiveConnections")); }

    Q_PROPERTY(uint State READ state)
    inline uint state() const
    { return qvariant_cast< uint >(internalPropGet("State")); }

    Q_PROPERTY(bool WirelessEnabled READ wirelessEnabled WRITE setWirelessEnabled)
    inline bool wirelessEnabled() const
    { return qvariant_cast< bool >(internalPropGet("WirelessEnabled")); }
    inline void setWirelessEnabled(bool value)
    { internalPropSet("WirelessEnabled", qVariantFromValue(value)); }

    Q_PROPERTY(bool WirelessHardwareEnabled READ wirelessHardwareEnabled)
    inline bool wirelessHardwareEnabled() const
    { return qvariant_cast< bool >(internalPropGet("WirelessHardwareEnabled")); }

public Q_SLOTS: // METHODS
    inline QDBusReply<QDBusObjectPath> ActivateConnection(const QString &service_name, const QDBusObjectPath &connection, const QDBusObjectPath &device, const QDBusObjectPath &specific_object)
    {
        QList<QVariant> argumentList;
        argumentList << qVariantFromValue(service_name) << qVariantFromValue(connection) << qVariantFromValue(device) << qVariantFromValue(specific_object);
        return callWithArgumentList(QDBus::Block, QLatin1String("ActivateConnection"), argumentList);
    }

    inline QDBusReply<void> DeactivateConnection(const QDBusObjectPath &active_connection)
    {
        QList<QVariant> argumentList;
        argumentList << qVariantFromValue(active_connection);
        return callWithArgumentList(QDBus::Block, QLatin1String("DeactivateConnection"), argumentList);
    }

    inline QDBusReply<QList<QDBusObjectPath> > GetDevices()
    {
        QList<QVariant> argumentList;
        return callWithArgumentList(QDBus::Block, QLatin1String("GetDevices"), argumentList);
    }

    inline QDBusReply<void> Sleep(bool sleep)
    {
        QList<QVariant> argumentList;
        argumentList << qVariantFromValue(sleep);
        return callWithArgumentList(QDBus::Block, QLatin1String("Sleep"), argumentList);
    }

Q_SIGNALS: // SIGNALS
    void DeviceAdded(const QDBusObjectPath &state);
    void DeviceRemoved(const QDBusObjectPath &state);
    void PropertiesChanged(const QVariantMap &properties);
    void StateChanged(uint state);
};

#endif
