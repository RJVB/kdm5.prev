/*  This file is part of the KDE project
    Copyright (C) 2007 Will Stephenson <wstephenson@kde.org>

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

#include <stdlib.h>
//#include <wireless.h>
// stuff copied from wireless.h so we don't have to include it.
// take aim at foot, prepare to fire
#define IW_AUTH_ALG_OPEN_SYSTEM 0x00000001
#define IW_AUTH_ALG_SHARED_KEY  0x00000002
#define IW_AUTH_WPA_VERSION_WPA         0x00000002
#define IW_AUTH_WPA_VERSION_WPA2        0x00000004
#define IW_AUTH_KEY_MGMT_802_1X 1
#define IW_AUTH_KEY_MGMT_PSK    2
#define IW_AUTH_CIPHER_NONE 0x00000001

#include <NetworkManager/NetworkManager.h>
#include <NetworkManager/cipher.h>
#include <NetworkManager/cipher-wep-ascii.h>
#include <NetworkManager/cipher-wep-hex.h>
#include <NetworkManager/cipher-wep-passphrase.h>
#include <NetworkManager/cipher-wpa-psk-hex.h>
#include <NetworkManager/cipher-wpa-psk-passphrase.h>
#include <QtDBus>

#include <kdebug.h>
#include <solid/control/ifaces/authentication.h>

#include "NetworkManager-dbushelper.h"

QList<QVariant> NMDBusHelper::serialize(Solid::Control::Authentication * auth, const QString  & essid, QList<QVariant>  & args, bool * error)
{
    if (auth)
    {
        Solid::Control::AuthenticationNone * none = dynamic_cast<Solid::Control::AuthenticationNone *>(auth) ;
        if (none)
            return doSerialize(none, essid, args, error);
        Solid::Control::AuthenticationWep * wep = dynamic_cast<Solid::Control::AuthenticationWep *>(auth) ;
        if (wep)
            return doSerialize(wep, essid, args, error);
        Solid::Control::AuthenticationWpaPersonal * wpap = dynamic_cast<Solid::Control::AuthenticationWpaPersonal *>(auth) ;
        if (wpap)
            return doSerialize(wpap, essid, args, error);
        Solid::Control::AuthenticationWpaEnterprise * wpae = dynamic_cast<Solid::Control::AuthenticationWpaEnterprise *>(auth);
        if (wpae)
            return doSerialize(wpae, essid, args, error);
    }
    *error = true;
    return QList<QVariant>();
}

QList<QVariant> NMDBusHelper::doSerialize(Solid::Control::AuthenticationNone * none, const QString  & essid, QList<QVariant>  & args, bool * error)
{
    *error = false;
    args << QVariant(IW_AUTH_CIPHER_NONE);
    return args;
}

QList<QVariant> NMDBusHelper::doSerialize(Solid::Control::AuthenticationWep * auth, const QString  & essid, QList<QVariant>  & args, bool * error)
{
    // get correct cipher
    // what's the algorithm for deciding the right cipher to use?
    *error = false;
    IEEE_802_11_Cipher * cipher = 0;
    if (auth->type() == Solid::Control::AuthenticationWep::WepAscii)
    {
        if (auth->keyLength() == 40 || auth->keyLength() == 64)
            cipher = cipher_wep64_ascii_new();
        else if (auth->keyLength() == 104 || auth->keyLength() == 128)
            cipher = cipher_wep128_ascii_new();
        else
            //NM only supports these 2 key lengths, flag an error!
            *error = true;
    }
    else if (auth->type() == Solid::Control::AuthenticationWep::WepHex)
    {
        if (auth->keyLength() == 40 || auth->keyLength() == 64)
            cipher = cipher_wep64_hex_new();
        else if (auth->keyLength() == 104 || auth->keyLength() == 128)
            cipher = cipher_wep128_hex_new();
        else
            //NM only supports these 2 key lengths, flag an error!
            *error = true;
    }
    else if (auth->type() == Solid::Control::AuthenticationWep::WepPassphrase)
    {
        if (auth->keyLength() == 40 || auth->keyLength() == 64)
            cipher = cipher_wep64_passphrase_new();
        else if (auth->keyLength() == 104 || auth->keyLength() == 128)
            cipher = cipher_wep128_passphrase_new();
        else
            //NM only supports these 2 key lengths, flag an error!
            *error = true;
    }
    else
        // unrecognised auth type, error!
        *error = true;
    if (!(*error))
    {
        // int32 cipher
        int we_cipher = ieee_802_11_cipher_get_we_cipher(cipher);
        args << QVariant(we_cipher);
        // s key
        //   cipher, essid, key
        char * rawHashedKey = 0;
        rawHashedKey = ieee_802_11_cipher_hash(cipher, essid.toUtf8(), auth->secrets()["key"].toUtf8());
        QString hashedKey = QString::fromAscii(rawHashedKey);
        free(rawHashedKey);
        args << QVariant(hashedKey);
        // int32 auth alg
        if (auth->method() == Solid::Control::AuthenticationWep::WepOpenSystem)
            args << QVariant(IW_AUTH_ALG_OPEN_SYSTEM);
        else
            args << QVariant(IW_AUTH_ALG_SHARED_KEY);
    }
    if (cipher)
        kDebug(1441) << "FIXME: delete cipher object";

    return args;
}

QList<QVariant> NMDBusHelper::doSerialize(Solid::Control::AuthenticationWpaPersonal * auth, const QString  & essid, QList<QVariant>  & args, bool * error)
{
    *error = false;
    IEEE_802_11_Cipher * cipher = 0;
    IEEE_802_11_Cipher * hexCipher = 0;
    IEEE_802_11_Cipher * ppCipher = 0;
    hexCipher = cipher_wpa_psk_hex_new();
    ppCipher = cipher_wpa_psk_passphrase_new();
    QString rawKey = auth->secrets()["key"];

    // cipher identification algorithm
    // can be either hex or passphrase
    // we try both methods

    // cipher needs the cipher setting on it
    // which is the protocol

    switch (auth->protocol())
    {
    case Solid::Control::AuthenticationWpaPersonal::WpaTkip:
        cipher_wpa_psk_hex_set_we_cipher(hexCipher, NM_AUTH_TYPE_WPA_PSK_TKIP);
        cipher_wpa_psk_passphrase_set_we_cipher(ppCipher, NM_AUTH_TYPE_WPA_PSK_TKIP);
        break;
    case Solid::Control::AuthenticationWpaPersonal::WpaCcmpAes:
        cipher_wpa_psk_hex_set_we_cipher(hexCipher, NM_AUTH_TYPE_WPA_PSK_CCMP);
        cipher_wpa_psk_passphrase_set_we_cipher(ppCipher, NM_AUTH_TYPE_WPA_PSK_CCMP);
        break;
    case Solid::Control::AuthenticationWpaPersonal::WpaAuto:
    default:
        cipher_wpa_psk_hex_set_we_cipher(hexCipher, NM_AUTH_TYPE_WPA_PSK_AUTO);
        cipher_wpa_psk_passphrase_set_we_cipher(ppCipher, NM_AUTH_TYPE_WPA_PSK_AUTO);
        break;
    }
    // now try both ciphers on the raw key
    if (ieee_802_11_cipher_validate(hexCipher, essid.toUtf8(), rawKey.toUtf8()) == 0)
    {
        kDebug() << "HEX";
        cipher = hexCipher;
    }
    else if (ieee_802_11_cipher_validate(ppCipher, essid.toUtf8(), rawKey.toUtf8()) == 0)
    {
        kDebug() << "PP";
        cipher = ppCipher;
    }
    else
        *error = true;

    if (!(*error))
    {
        // int32 cipher
        int we_cipher = ieee_802_11_cipher_get_we_cipher(cipher);
        args << QVariant(we_cipher);
        // s key
        char * rawHashedKey = 0;
        rawHashedKey = ieee_802_11_cipher_hash(cipher, essid.toUtf8(), rawKey.toUtf8());
        QString hashedKey = QString::fromAscii(rawHashedKey);
        free(rawHashedKey);
        args << QVariant(hashedKey);
        // int32 wpa version
        if (auth->version() == Solid::Control::AuthenticationWpaPersonal::Wpa1)
            args << QVariant(IW_AUTH_WPA_VERSION_WPA);
        else
            args << QVariant(IW_AUTH_WPA_VERSION_WPA2);
        // int32 key management
        if (auth->keyManagement() == Solid::Control::AuthenticationWpaPersonal::WpaPsk)
            args << QVariant(IW_AUTH_KEY_MGMT_PSK);
        else
            args << QVariant(IW_AUTH_KEY_MGMT_802_1X);
        kDebug(1411) << "Outbound args are: " << args;
    }
    return args;
}

QList<QVariant> NMDBusHelper::doSerialize(Solid::Control::AuthenticationWpaEnterprise * auth, const QString  & essid, QList<QVariant>  & args, bool * error)
{
    Q_UNUSED(essid)
    Q_UNUSED(error)
    kDebug() << "Implement me!";
    // int32 cipher, always NM_AUTH_TYPE_WPA_EAP
    args << NM_AUTH_TYPE_WPA_EAP;
    switch (auth->method())
    {
    case Solid::Control::AuthenticationWpaEnterprise::EapPeap:
        args << NM_EAP_METHOD_PEAP;
        break;
    case Solid::Control::AuthenticationWpaEnterprise::EapTls:
        args << NM_EAP_METHOD_TLS;
        break;
    case Solid::Control::AuthenticationWpaEnterprise::EapTtls :
        args << NM_EAP_METHOD_TTLS;
        break;
    case Solid::Control::AuthenticationWpaEnterprise::EapMd5:
        args << NM_EAP_METHOD_MD5;
        break;
    case Solid::Control::AuthenticationWpaEnterprise::EapMsChap:
        args << NM_EAP_METHOD_MSCHAP;
        break;
    case Solid::Control::AuthenticationWpaEnterprise::EapOtp:
        args << NM_EAP_METHOD_OTP;
        break;
    case Solid::Control::AuthenticationWpaEnterprise::EapGtc:
        args << NM_EAP_METHOD_GTC;
        break;
    }
    // int32 key type
    args << NM_AUTH_TYPE_WPA_PSK_AUTO;
    // s identity
    args << auth->identity();
    // s password
    args << auth->idPasswordKey();
    // s anon identity
    args << auth->anonIdentity();
    // s priv key password
    args << auth->certPrivatePasswordKey();
    // s priv key file
    args << auth->certPrivate();
    // s client cert file
    args << auth->certClient();
    // s ca cert file
    args << auth->certCA();
    // int32 wpa version => {IW_AUTH_WPA_VERSION_WPA,IW_AUTH_WPA_VERSION_WPA2}
    args << auth->version();
    return QList<QVariant>();
}
