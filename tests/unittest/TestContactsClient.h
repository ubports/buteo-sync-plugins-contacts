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

#ifndef TEST_CONTACTSCLIENT_H
#define TEST_CONTACTSCLIENT_H

#include <UContactsClient.h>

#include <QPointer>

class MockRemoteSource;
class MockAuthenticator;


class TestContactsClient : public UContactsClient
{
    Q_OBJECT
public:
    TestContactsClient(const QString& aPluginName,
                    const Buteo::SyncProfile &aProfile,
                    Buteo::PluginCbInterface *aCbInterface);

    virtual ~TestContactsClient();

    // test helpers
    QPointer<MockRemoteSource> m_remoteSource;
    QPointer<UContactsBackend> m_localSource;
    QPointer<MockAuthenticator> m_authenticator;
    QDateTime m_lastSyncTime;

protected:
    QVariantMap remoteSourceProperties() const;
    UAbstractRemoteSource* createRemoteSource(QObject *parent) const;
    UContactsBackend* createContactsBackend(QObject *parent) const;
    UAuth* crateAuthenticator(QObject *parent) const;
    virtual const QDateTime lastSyncTime() const;
};

#endif // TEST_CONTACTSCLIENT_H
