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

#include "MockAuthenticator.h"

MockAuthenticator::MockAuthenticator(QObject *parent)
    : UAuth(parent)
{
}

MockAuthenticator::~MockAuthenticator()
{
}

bool MockAuthenticator::authenticate()
{
    // simulate authentication sucess
    emit success();
    return true;
}

bool MockAuthenticator::init(const quint32 accountId, const QString serviceName)
{
    m_accountId = accountId;
    m_serviceName = serviceName;
    m_initialized = true;
    return true;
}

