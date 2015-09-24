/*
 * This file is part of buteo-gcontact-plugin package
 *
 * Copyright (C) 2015 Canonical Ltd.
 *
 * Contributors: Renato Araujo Oliveira Filho <renato.filho@canonical.com>
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

#include "TestContactsClient.h"
#include "MockRemoteSource.h"
#include "MockAuthenticator.h"

#include <UContactsBackend.h>

#include <QLibrary>
#include <QtNetwork>
#include <QDateTime>
#include <QContactGuid>
#include <QContactDetailFilter>
#include <QContactAvatar>


TestContactsClient::TestContactsClient(const QString& aPluginName,
                                       const Buteo::SyncProfile& aProfile,
                                       Buteo::PluginCbInterface *aCbInterface)
    : UContactsClient(aPluginName, aProfile, aCbInterface, "test-plugin"),
      m_remoteSource(0),
      m_authenticator(0)
{
}

TestContactsClient::~TestContactsClient()
{
}

QVariantMap TestContactsClient::remoteSourceProperties() const
{
    QVariantMap remoteProperties;
    return remoteProperties;
}

UAbstractRemoteSource *TestContactsClient::createRemoteSource(QObject *parent) const
{
    const_cast<TestContactsClient*>(this)->m_remoteSource= new MockRemoteSource(parent);
    return qobject_cast<UAbstractRemoteSource*>(m_remoteSource.data());
}

UContactsBackend *TestContactsClient::createContactsBackend(QObject *parent) const
{
    const_cast<TestContactsClient*>(this)->m_localSource = new UContactsBackend(QStringLiteral("mock"), parent);
    return m_localSource.data();
}

UAuth *TestContactsClient::crateAuthenticator(QObject *parent) const
{
    const_cast<TestContactsClient*>(this)->m_authenticator = new MockAuthenticator(parent);
    return  qobject_cast<UAuth*>(m_authenticator.data());
}

const QDateTime TestContactsClient::lastSyncTime() const
{
    return m_lastSyncTime;
}
