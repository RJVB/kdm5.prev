/*  This file is part of the KDE project
    Copyright (C) 2006 Kevin Ottens <ervin@kde.org>

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

#include "networkhw.h"

#include "haldevice.h"

#include <QStringList>

NetworkHw::NetworkHw( HalDevice *device )
    : Capability( device )
{

}

NetworkHw::~NetworkHw()
{

}

QString NetworkHw::ifaceName() const
{
    return m_device->property( "net.interface" ).toString();
}

bool NetworkHw::isWireless() const
{
    QStringList capabilities = m_device->property( "info.capabilities" ).toStringList();

    return capabilities.contains( "net.80211" );
}

QString NetworkHw::hwAddress() const
{
    return m_device->property( "net.address" ).toString();
}

qulonglong NetworkHw::macAddress() const
{
    if ( m_device->propertyExists( "net.80211.mac_address" ) )
    {
        return m_device->property( "net.80211.mac_address" ).toULongLong();
    }
    else
    {
        return m_device->property( "net.80203.mac_address" ).toULongLong();
    }
}

#include "networkhw.moc"
