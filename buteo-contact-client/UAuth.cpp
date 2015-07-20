/*
 * This file is part of buteo-sync-plugins-goole package
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

#include "config.h"
#include "UAuth.h"

#include <QVariantMap>
#include <QTextStream>
#include <QFile>
#include <QStringList>
#include <QDebug>

#include <Accounts/AccountService>

#include <ProfileEngineDefs.h>
#include <LogMacros.h>

using namespace Accounts;
using namespace SignOn;

class UAuthPrivate
{
public:
    UAuthPrivate() {}
    ~UAuthPrivate() {}

    QPointer<Accounts::Manager> mAccountManager;
    QPointer<SignOn::Identity> mIdentity;
    QPointer<SignOn::AuthSession> mSession;
    QPointer<Accounts::Account> mAccount;
    QString mServiceName;
};

UAuth::UAuth(QObject *parent)
    : QObject(parent),
      d_ptr(new UAuthPrivate)
{
}

UAuth::~UAuth()
{
}

bool
UAuth::init(const quint32 accountId, const QString serviceName)
{
    Q_D(UAuth);

    d->mServiceName = serviceName;
    if (d->mAccountManager && d_ptr->mAccount) {
        LOG_DEBUG("GAuth already initialized");
        return false;
    }

    if (!d->mAccountManager) {
        d->mAccountManager = new Accounts::Manager();
        if (d->mAccountManager == NULL) {
            LOG_DEBUG("Account manager is not created... Cannot authenticate");
            return false;
        }
    }

    if (!d->mAccount) {
        d->mAccount = Accounts::Account::fromId(d->mAccountManager.data(), accountId, this);
        if (d->mAccount == NULL) {
            LOG_DEBUG("Account is not created... Cannot authenticate");
            return false;
        }
        mDisplayName = d->mAccount->displayName();
    }

    return true;
}

void
UAuth::sessionResponse(const SessionData &sessionData)
{
    SignOn::AuthSession *session = qobject_cast<SignOn::AuthSession*>(sender());
    Q_ASSERT(session);
    session->disconnect(this);

    mToken = sessionData.getProperty(QStringLiteral("AccessToken")).toString();
    LOG_DEBUG("Authenticated !!!");

    emit success();
}

bool
UAuth::authenticate()
{
    Q_D(UAuth);
    if (d->mSession) {
        LOG_WARNING(QString("error: Account %1 Authenticate already requested")
                .arg(d->mAccount->displayName()));
        return true;
    }

    Accounts::Service srv(d->mAccountManager->service(d->mServiceName));
    if (!srv.isValid()) {
        LOG_WARNING(QString("error: Service [%1] not found for account [%2].")
                .arg(d->mServiceName)
                .arg(d->mAccount->displayName()));
        return false;
    }
    d->mAccount->selectService(srv);

    Accounts::AccountService *accSrv = new Accounts::AccountService(d->mAccount, srv);
    if (!accSrv) {
        LOG_WARNING(QString("error: Account %1 has no valid account service")
                .arg(d->mAccount->displayName()));
        return false;
    }
    if (!accSrv->isEnabled()) {
        LOG_WARNING(QString("error: Service %1 not enabled for account %2.")
                .arg(d->mServiceName)
                .arg(d->mAccount->displayName()));
        return false;
    }

    AuthData authData = accSrv->authData();
    d->mIdentity = SignOn::Identity::existingIdentity(authData.credentialsId());
    if (!d->mIdentity) {
        LOG_WARNING(QString("error: Account %1 has no valid credentials")
                .arg(d->mAccount->displayName()));
        return false;
    }

    d->mSession = d->mIdentity->createSession(authData.method());
    if (!d->mSession) {
        LOG_WARNING(QString("error: could not create signon session for Google account %1")
                .arg(d->mAccount->displayName()));
        accSrv->deleteLater();
        return false;
    }
    connect(d->mSession.data(),SIGNAL(response(SignOn::SessionData)),
            SLOT(sessionResponse(SignOn::SessionData)), Qt::QueuedConnection);
    connect(d->mSession.data(), SIGNAL(error(SignOn::Error)),
            SLOT(error(SignOn::Error)), Qt::QueuedConnection);
    accSrv->deleteLater();

    QVariantMap signonSessionData = authData.parameters();
    signonSessionData.insert("ClientId", GOOGLE_CONTACTS_CLIENT_ID);
    signonSessionData.insert("ClientSecret", GOOGLE_CONTACTS_CLIENT_SECRET);
    signonSessionData.insert("UiPolicy", SignOn::NoUserInteractionPolicy);
    d->mSession->process(signonSessionData, authData.mechanism());
    return true;
}

void UAuth::credentialsStored(const quint32 id)
{
    Q_D(UAuth);
    d->mAccount->setCredentialsId(id);
    d->mAccount->sync();
}

void UAuth::error(const SignOn::Error & error)
{
    LOG_WARNING("LOGIN ERROR:" << error.message());
    emit failed();
}
