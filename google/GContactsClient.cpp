/*
 * This file is part of buteo-sync-plugins-contacts package
 *
 * Copyright (C) 2013 Jolla Ltd. and/or its subsidiary(-ies).
 *               2015 Canonical Ltd
 *
 * Contributors: Sateesh Kavuri <sateesh.kavuri@gmail.com>
 *               Mani Chandrasekar <maninc@gmail.com>
 *               Renato Araujo Oliveira Filho <renato.filho@canonical.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include "GContactsClient.h"
#include "GTransport.h"
#include "GConfig.h"
#include "GRemoteSource.h"

#include <QLibrary>
#include <QtNetwork>
#include <QDateTime>
#include <QContactGuid>
#include <QContactDetailFilter>
#include <QContactAvatar>

#include "buteosyncfw_p.h"

static const QString GOOGLE_CONTACTS_SERVICE          ("google-contacts");

extern "C" GContactsClient* createPlugin(const QString& aPluginName,
                                         const Buteo::SyncProfile& aProfile,
                                         Buteo::PluginCbInterface *aCbInterface)
{
    return new GContactsClient(aPluginName, aProfile, aCbInterface);
}

extern "C" void destroyPlugin(GContactsClient *aClient)
{
    delete aClient;
}

GContactsClient::GContactsClient(const QString& aPluginName,
                                 const Buteo::SyncProfile& aProfile,
                                 Buteo::PluginCbInterface *aCbInterface)
    : UContactsClient(aPluginName, aProfile, aCbInterface, GOOGLE_CONTACTS_SERVICE)
{
    FUNCTION_CALL_TRACE;
}

GContactsClient::~GContactsClient()
{
    FUNCTION_CALL_TRACE;
}

QVariantMap GContactsClient::remoteSourceProperties() const
{
    QVariantMap remoteProperties;
    remoteProperties.insert("AUTH-TOKEN", authToken());
    remoteProperties.insert("SYNC-TARGET", syncTargetId());
    remoteProperties.insert("ACCOUNT-NAME", accountName());
    remoteProperties.insert(Buteo::KEY_REMOTE_DATABASE, iProfile.key(Buteo::KEY_REMOTE_DATABASE));
    remoteProperties.insert(Buteo::KEY_HTTP_PROXY_HOST, iProfile.key(Buteo::KEY_HTTP_PROXY_HOST));
    remoteProperties.insert(Buteo::KEY_HTTP_PROXY_PORT, iProfile.key(Buteo::KEY_HTTP_PROXY_PORT));
    return remoteProperties;
}

UAbstractRemoteSource *GContactsClient::createRemoteSource(QObject *parent) const
{
    return qobject_cast<UAbstractRemoteSource*>(new GRemoteSource(parent));
}
