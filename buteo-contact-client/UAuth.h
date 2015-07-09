/*
 * This file is part of buteo-gcontact-plugins package
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

#ifndef UAUTH_H
#define UAUTH_H

#include <QObject>
#include <QScopedPointer>

#include <SignOn/AuthService>
#include <SignOn/Identity>

#include <Accounts/Account>
#include <Accounts/Manager>

class UAuthPrivate;

class UAuth : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(UAuth)
public:
    explicit UAuth(QObject *parent = 0);
    ~UAuth();

    virtual bool authenticate();
    virtual bool init(const quint32 accountId, const QString serviceName);

    inline QString accountDisplayName() const { return mDisplayName; }
    inline QString token() const { return mToken; }

signals:
    void success();
    void failed();

protected:
    QString mToken;
    QString mDisplayName;

private:
    QScopedPointer<UAuthPrivate> d_ptr;

private slots:
    void credentialsStored(const quint32);
    void error(const SignOn::Error &);
    void sessionResponse(const SignOn::SessionData &);
};

#endif // GAUTH_H
