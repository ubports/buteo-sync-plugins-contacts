/*
 * This file is part of buteo-gcontact-plugin package
 *
 * Copyright (C) 2013 Jolla Ltd. and/or its subsidiary(-ies).
 *               2015 Canonical Ltd.
 *
 * Contributors: Sateesh Kavuri <sateesh.kavuri@gmail.com>
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

#ifndef GCONTACTSCLIENT_H
#define GCONTACTSCLIENT_H

#include <UContactsClient.h>

#include "buteosyncfw_p.h"
#include "buteo-gcontact-plugin_global.h"

class GContactEntry;
class GTransport;

class BUTEOGCONTACTPLUGINSHARED_EXPORT GContactsClient : public UContactsClient
{
    Q_OBJECT
public:
    GContactsClient(const QString& aPluginName,
                    const Buteo::SyncProfile &aProfile,
                    Buteo::PluginCbInterface *aCbInterface);

    virtual ~GContactsClient();

protected:
    QVariantMap remoteSourceProperties() const;
    UAbstractRemoteSource* createRemoteSource(QObject *parent) const;
};

extern "C" GContactsClient* createPlugin(const QString& aPluginName,
                                         const Buteo::SyncProfile& aProfile,
                                         Buteo::PluginCbInterface *aCbInterface);

extern "C" void destroyPlugin(GContactsClient *aClient );

#endif // GCONTACTSCLIENT_H
