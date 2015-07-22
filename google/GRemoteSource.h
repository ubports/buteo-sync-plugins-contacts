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

#include "GContactStream.h"

#include <UAbstractRemoteSource.h>

#include <QHash>
#include <QScopedPointer>

class GTransport;

class GRemoteSource : public UAbstractRemoteSource
{
    Q_OBJECT
public:
    GRemoteSource(QObject *parent = 0);
    ~GRemoteSource();

    // UAbstractRemoteSource
    bool init(const QVariantMap &properties);
    void abort();
    void fetchContacts(const QDateTime &since, bool includeDeleted, bool fetchAvatar = true);

    // help on tests
    const GTransport *transport() const;
    int state() const;

protected:
    void saveContactsNonBatch(const QList<QtContacts::QContact> contacts);
    void removeContactsNonBatch(const QList<QtContacts::QContact> contacts);

    void batch(const QList<QtContacts::QContact> &contactsToCreate,
               const QList<QtContacts::QContact> &contactsToUpdate,
               const QList<QtContacts::QContact> &contactsToRemove);

private slots:
    void networkRequestFinished();
    void networkError(int errorCode);

private:
    enum SyncState {
        STATE_IDLE = 0,
        STATE_FETCHING_CONTACTS,
        STATE_BATCH_RUNNING,
    };

    QScopedPointer<GTransport> mTransport;
    QString mRemoteUri;
    QString mAuthToken;
    QString mSyncTarget;
    QString mAccountName;
    SyncState mState;
    int mStartIndex;
    bool mFetchAvatars;
    QMap<QString, QPair<QString, QUrl> > mLocalIdToAvatar;
    QMap<QString, QContact> mLocalIdToContact;
    QMultiMap<GoogleContactStream::UpdateType, QPair<QtContacts::QContact, QStringList> > mPendingBatchOps;

    void fetchAvatars(QList<QtContacts::QContact> *contacts);
    void uploadAvatars(QList<QContact> *contacts);
    void fetchRemoteContacts(const QDateTime &since, bool includeDeleted, int startIndex);
    void batchOperationContinue();
    int parseErrorReponse(const GoogleContactAtom::BatchOperationResponse &response);
    void emitTransactionCommited(const QList<QtContacts::QContact> &created,
                                 const QList<QtContacts::QContact> &changed,
                                 const QList<QContact> &removed,
                                 const QMap<QString, int> &errorMap,
                                 Sync::SyncStatus status);

};
