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

#ifndef MOCK_AUTH_H
#define MOCK_AUTH_H

#include <UAuth.h>

#include <QObject>
#include <QScopedPointer>

class MockAuthenticator : public UAuth
{
    Q_OBJECT
public:
    explicit MockAuthenticator(QObject *parent = 0);
    ~MockAuthenticator();

    bool authenticate();
    bool init(const quint32 accountId, const QString serviceName);

    //test helpers
    bool m_initialized;
    quint32 m_accountId;
    QString m_serviceName;

};

#endif // MOCK_AUTH_H
