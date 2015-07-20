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

#include "UContactsClient.h"
#include "UContactsBackend.h"
#include "UAbstractRemoteSource.h"
#include "UAuth.h"
#include "config.h"

//Buteo
#include <LogMacros.h>
#include <ProfileEngineDefs.h>
#include <ProfileManager.h>

#include <QLibrary>
#include <QtNetwork>
#include <QDateTime>
#include <QContactGuid>
#include <QContactDetailFilter>
#include <QContactAvatar>

class UContactsClientPrivate
{
public:
    UContactsClientPrivate(const QString &serviceName)
        : mAuth(0),
          mContactBackend(0),
          mRemoteSource(0),
          mServiceName(serviceName)
    {
    }

    UAuth*                      mAuth;
    UContactsBackend*           mContactBackend;
    UAbstractRemoteSource*      mRemoteSource;
    bool                        mSlowSync;
    QString                     mServiceName;
    // local database information
    QSet<QContactId>            mAllLocalContactIds;
    RemoteToLocalIdMap  mAddedContactIds;
    RemoteToLocalIdMap  mModifiedContactIds;
    RemoteToLocalIdMap  mDeletedContactIds;
    // sync report
    QMap<QString, Buteo::DatabaseResults> mItemResults;
    Buteo::SyncResults          mResults;
    // sync profile
    QString mSyncTarget;
    qint32 mAccountId;
    Buteo::SyncProfile::SyncDirection mSyncDirection;
    Buteo::SyncProfile::ConflictResolutionPolicy mConflictResPolicy;
};

UContactsClient::UContactsClient(const QString& aPluginName,
                                 const Buteo::SyncProfile& aProfile,
                                 Buteo::PluginCbInterface *aCbInterface, const QString &serviceName)
    : ClientPlugin(aPluginName, aProfile, aCbInterface),
      d_ptr(new UContactsClientPrivate(serviceName))
{
    FUNCTION_CALL_TRACE;
}

UContactsClient::~UContactsClient()
{
    FUNCTION_CALL_TRACE;
    Q_D(UContactsClient);

    delete d->mAuth;
    delete d->mRemoteSource;
}

bool
UContactsClient::init()
{
    FUNCTION_CALL_TRACE;
    Q_D(UContactsClient);

    if (lastSyncTime().isNull()) {
        d->mSlowSync = true;
    } else {
        d->mSlowSync = false;
    }

    LOG_DEBUG ("Last sync date:" << lastSyncTime() << "Using slow sync?" << d->mSlowSync);
    if (!initConfig()) {
        LOG_CRITICAL("Fail to init configuration");
        return false;
    }

    d->mAuth = crateAuthenticator(this);
    if (!d->mAuth || !d->mAuth->init(d->mAccountId, d->mServiceName)) {
        LOG_CRITICAL("Fail to create auth object");
        goto init_fail;
    }

    d->mContactBackend = createContactsBackend(this);
    if (!d->mContactBackend || !d->mContactBackend->init(d->mAccountId,
                                                         d->mAuth->accountDisplayName())) {
        LOG_CRITICAL("Fail to create contact backend");
        goto init_fail;
    }


    // remote source must be initialized after mAuth because its uses the account name property
    d->mRemoteSource = createRemoteSource(this);
    if (!d->mRemoteSource) {
        LOG_CRITICAL("Fail to create remote contact backend");
        goto init_fail;
    }

    d->mItemResults.insert(syncTargetId(), Buteo::DatabaseResults());

    // sign in.
    connect(d->mAuth, SIGNAL(success()), SLOT(start()));
    connect(d->mAuth, SIGNAL(failed()), SLOT(onAuthenticationError()));

    // syncStateChanged to signal changes from CONNECTING, RECEIVING
    // SENDING, DISCONNECTING, CLOSED
    connect(this,
            SIGNAL(stateChanged(Sync::SyncProgressDetail)),
            SLOT(onStateChanged(Sync::SyncProgressDetail)));

    // Take necessary action when sync is finished
    connect(this,
            SIGNAL(syncFinished(Sync::SyncStatus)),
            SLOT(onSyncFinished(Sync::SyncStatus)));

    return true;

init_fail:

    delete d->mRemoteSource;
    delete d->mContactBackend;
    delete d->mAuth;
    d->mRemoteSource = 0;
    d->mContactBackend = 0;
    d->mAuth = 0;
    return false;
}

bool
UContactsClient::uninit()
{
    FUNCTION_CALL_TRACE;
    Q_D(UContactsClient);

    delete d->mRemoteSource;
    delete d->mContactBackend;
    delete d->mAuth;
    d->mRemoteSource = 0;
    d->mContactBackend = 0;
    d->mAuth = 0;

    return true;
}

bool
UContactsClient::isReadyToSync() const
{
    const Q_D(UContactsClient);
    return (d->mContactBackend && d->mRemoteSource && d->mAuth);
}

UContactsBackend *UContactsClient::createContactsBackend(QObject *parent) const
{
    return new UContactsBackend(QCONTACTS_BACKEND_NAME, parent);
}

UAuth *UContactsClient::crateAuthenticator(QObject *parent) const
{
    return new UAuth(parent);
}

bool
UContactsClient::startSync()
{
    FUNCTION_CALL_TRACE;

    if (!isReadyToSync()) {
        LOG_WARNING ("Ubuntu plugin is not ready to sync.");
        return false;
    }

    Q_D(UContactsClient);
    LOG_DEBUG ("Init done. Continuing with sync");

    return d->mAuth->authenticate();
}

void
UContactsClient::abortSync(Sync::SyncStatus aStatus)
{
    FUNCTION_CALL_TRACE;
    Q_D(UContactsClient);

    d->mRemoteSource->abort();
    emit syncFinished(Sync::SYNC_ABORTED);
}

bool
UContactsClient::initConfig()
{
    FUNCTION_CALL_TRACE;
    Q_D(UContactsClient);

    //TODO: support multiple remote databases "scopes"
    QStringList accountList = iProfile.keyValues(Buteo::KEY_ACCOUNT_ID);
    if (!accountList.isEmpty()) {
        QString aId = accountList.first();
        if (aId != NULL) {
            d->mAccountId = aId.toInt();
        }
    } else {
        d->mAccountId = 0;
        LOG_WARNING("Account id not found in config profile");
        return false;
    }

    QStringList databaseName = iProfile.keyValues(Buteo::KEY_DISPLAY_NAME);
    if (databaseName.isEmpty()) {
        LOG_WARNING("\"displayname\" is missing on configuration file");
        return false;
    }
    d->mSyncTarget = databaseName.first();
    d->mSyncDirection = iProfile.syncDirection();
    d->mConflictResPolicy = iProfile.conflictResolutionPolicy();

    return true;
}

void
UContactsClient::onAuthenticationError()
{
    LOG_WARNING("Fail to authenticate with account");
    emit syncFinished (Sync::SYNC_AUTHENTICATION_FAILURE);
}

bool
UContactsClient::start()
{
    FUNCTION_CALL_TRACE;
    Q_D(UContactsClient);

    /*
      1. If no previous sync, go for slow-sync. Fetch all contacts
         from server
      2. Check if previous sync happened (from SyncLog). If yes,
         fetch the time of last sync
      3. Using the last sync time, retrieve all contacts from server
         that were added/modified/deleted
      4. Fetch all added/modified/deleted items from device
      5. Check for conflicts. Take the default policy as "server-wins"
      6. Save the list from the server to device
      7. Push "client changes" - "conflicting items" to the server
      8. Save the sync log
     */

    // Remote source will be create after authentication since it needs some information
    // about the authentication (auth-token, etc..)

    LOG_INFO("Sync Started at:" << QDateTime::currentDateTime().toUTC().toString(Qt::ISODate));

    if (!d->mRemoteSource->init(remoteSourceProperties())) {
        LOG_WARNING("Fail to init remote source");
        return false;
    }

    switch (d->mSyncDirection)
    {
    case Buteo::SyncProfile::SYNC_DIRECTION_TWO_WAY:
    {
        QDateTime sinceDate = d->mSlowSync ? QDateTime() : lastSyncTime();

        LOG_DEBUG("load all contacts since" << sinceDate << sinceDate.isValid());
        // load changed contact since the last sync date or all contacts if no
        // sync was done before
        loadLocalContacts(sinceDate);

        // load remote contacts
        if (d->mSlowSync) {
            connect(d->mRemoteSource,
                    SIGNAL(contactsFetched(QList<QtContacts::QContact>,Sync::SyncStatus)),
                    SLOT(onRemoteContactsFetchedForSlowSync(QList<QtContacts::QContact>,Sync::SyncStatus)));
        } else {
            connect(d->mRemoteSource,
                    SIGNAL(contactsFetched(QList<QtContacts::QContact>,Sync::SyncStatus)),
                    SLOT(onRemoteContactsFetchedForFastSync(QList<QtContacts::QContact>,Sync::SyncStatus)));
        }
        d->mRemoteSource->fetchContacts(sinceDate, !d->mSlowSync, true);
        break;
    }
    case Buteo::SyncProfile::SYNC_DIRECTION_FROM_REMOTE:
        LOG_WARNING("SYNC_DIRECTION_FROM_REMOTE: not implemented");
        return false;
    case Buteo::SyncProfile::SYNC_DIRECTION_TO_REMOTE:
        LOG_WARNING("SYNC_DIRECTION_TO_REMOTE: not implemented");
        return false;
    case Buteo::SyncProfile::SYNC_DIRECTION_UNDEFINED:
        // Not required
    default:
        // throw configuration error
        return false;
        break;
    };

    return true;
}

QList<QContact>
UContactsClient::prepareContactsToUpload(UContactsBackend *backend,
                                         const QSet<QContactId> &ids)
{
    QList<QContact> toUpdate;

    foreach(const QContactId &id, ids) {
        QContact contact = backend->getContact(id);
        if (!contact.isEmpty()) {
            toUpdate << contact;
        } else {
            LOG_CRITICAL("Fail to find local contact with id:" << id);
            return QList<QContact>();
        }
    }

    return toUpdate;
}

void
UContactsClient::onRemoteContactsFetchedForSlowSync(const QList<QContact> contacts,
                                                    Sync::SyncStatus status)
{
    FUNCTION_CALL_TRACE;
    Q_D(UContactsClient);
    if ((status != Sync::SYNC_PROGRESS) && (status != Sync::SYNC_STARTED)) {
        disconnect(d->mRemoteSource);
    }

    if ((status == Sync::SYNC_PROGRESS) || (status == Sync::SYNC_DONE)) {
        // save remote contacts locally
        storeToLocalForSlowSync(contacts);

        if (status == Sync::SYNC_DONE) {
            QList<QContact> toUpload = prepareContactsToUpload(d->mContactBackend, d->mAllLocalContactIds);
            connect(d->mRemoteSource,
                    SIGNAL(transactionCommited(QList<QtContacts::QContact>,
                                               QList<QtContacts::QContact>,
                                               QStringList,Sync::SyncStatus)),
                    SLOT(onContactsSavedForSlowSync(QList<QtContacts::QContact>,
                                                    QList<QtContacts::QContact>,
                                                    QStringList,Sync::SyncStatus)));

            d->mRemoteSource->transaction();
            d->mRemoteSource->saveContacts(toUpload);
            d->mRemoteSource->commit();
        }
    } else {
        emit syncFinished(status);
    }
}

void
UContactsClient::onContactsSavedForSlowSync(const QList<QtContacts::QContact> &createdContacts,
                                            const QList<QtContacts::QContact> &updatedContacts,
                                            const QStringList &removedContacts,
                                            Sync::SyncStatus status)
{
    FUNCTION_CALL_TRACE;
    Q_D(UContactsClient);

    LOG_DEBUG("AFTER UPLOAD(Slow sync):"
                << "\n\tCreated on remote:" << createdContacts.size()
                << "\n\tUpdated on remote:" << updatedContacts.size()
                << "\n\tRemoved from remote:" << removedContacts.size());

    if ((status != Sync::SYNC_PROGRESS) && (status != Sync::SYNC_STARTED)) {
        disconnect(d->mRemoteSource);
    }

    if ((status == Sync::SYNC_PROGRESS) || (status == Sync::SYNC_DONE)) {
        QList<QContact> changedContacts;

        changedContacts += createdContacts;
        changedContacts += updatedContacts;
        updateIdsToLocal(changedContacts);

        // sync report
        addProcessedItem(Sync::ITEM_ADDED,
                         Sync::REMOTE_DATABASE,
                         syncTargetId(),
                         createdContacts.size());

        if (status == Sync::SYNC_PROGRESS) {
            // sync still in progress
            return;
        }
    }

    emit syncFinished(status);
}

void UContactsClient::onRemoteContactsFetchedForFastSync(const QList<QContact> contacts,
                                                         Sync::SyncStatus status)
{
    FUNCTION_CALL_TRACE;
    Q_D(UContactsClient);
    if ((status != Sync::SYNC_PROGRESS) && (status != Sync::SYNC_STARTED)) {
        disconnect(d->mRemoteSource);
    }

    if ((status == Sync::SYNC_PROGRESS) || (status == Sync::SYNC_DONE)) {
        // save remote contacts locally
        storeToLocalForFastSync(contacts);

        if (status == Sync::SYNC_DONE) {
            QList<QContact> contactsToUpload;
            QList<QContact> contactsToRemove;

            // Contacts created locally
            LOG_DEBUG("Total number of Contacts ADDED : " << d->mAddedContactIds.count());
            contactsToUpload = prepareContactsToUpload(d->mContactBackend,
                                                       d->mAddedContactIds.values().toSet());

            // Contacts modified locally
            LOG_DEBUG("Total number of Contacts MODIFIED : " << d->mModifiedContactIds.count());
            contactsToUpload += prepareContactsToUpload(d->mContactBackend,
                                                        d->mModifiedContactIds.values().toSet());

            // Contacts deleted locally
            LOG_DEBUG("Total number of Contacts DELETED : " << d->mDeletedContactIds.count());
            contactsToRemove = prepareContactsToUpload(d->mContactBackend,
                                                       d->mDeletedContactIds.values().toSet());

            connect(d->mRemoteSource,
                    SIGNAL(transactionCommited(QList<QtContacts::QContact>,
                                               QList<QtContacts::QContact>,
                                               QStringList,Sync::SyncStatus)),
                    SLOT(onContactsSavedForFastSync(QList<QtContacts::QContact>,
                                                    QList<QtContacts::QContact>,
                                                    QStringList,Sync::SyncStatus)));

            d->mRemoteSource->transaction();
            d->mRemoteSource->saveContacts(contactsToUpload);
            d->mRemoteSource->removeContacts(contactsToRemove);
            d->mRemoteSource->commit();
        }
    } else {
        emit syncFinished(status);
    }
}

void
UContactsClient::onContactsSavedForFastSync(const QList<QtContacts::QContact> &createdContacts,
                                            const QList<QtContacts::QContact> &updatedContacts,
                                            const QStringList &removedContacts,
                                            Sync::SyncStatus status)
{
    Q_UNUSED(updatedContacts)
    Q_UNUSED(removedContacts)
    FUNCTION_CALL_TRACE;
    Q_D(UContactsClient);

    LOG_DEBUG("AFTER UPLOAD(Fast sync):" << status
                << "\n\tCreated on remote:" << createdContacts.size()
                << "\n\tUpdated on remote:" << updatedContacts.size()
                << "\n\tRemoved from remote:" << removedContacts.size());

    if ((status != Sync::SYNC_PROGRESS) && (status != Sync::SYNC_STARTED)) {
        disconnect(d->mRemoteSource);
    }

    if ((status == Sync::SYNC_PROGRESS) || (status == Sync::SYNC_DONE)) {
        QList<QContact> changedContacts;

        changedContacts += createdContacts;
        changedContacts += updatedContacts;

        updateIdsToLocal(changedContacts);

        // sync report
        addProcessedItem(Sync::ITEM_ADDED,
                         Sync::REMOTE_DATABASE,
                         syncTargetId(),
                         createdContacts.size());
        addProcessedItem(Sync::ITEM_MODIFIED,
                         Sync::REMOTE_DATABASE,
                         syncTargetId(),
                         updatedContacts.size());
        addProcessedItem(Sync::ITEM_DELETED,
                         Sync::REMOTE_DATABASE,
                         syncTargetId(),
                         removedContacts.size());

        if (status == Sync::SYNC_PROGRESS) {
            // sync still in progress
            return;
        }
    }

    emit syncFinished(status);
}

bool
UContactsClient::storeToLocalForSlowSync(const QList<QContact> &remoteContacts)
{
    FUNCTION_CALL_TRACE;

    Q_D(UContactsClient);
    Q_ASSERT(d->mSlowSync);

    bool syncSuccess = false;

    LOG_DEBUG ("@@@storeToLocal#SLOW SYNC");
    // Since we request for all the deleted contacts, if
    // slow sync is performed many times, even deleted contacts
    // will appear in *remoteContacts. Filter them out while
    // saving them to device
    LOG_DEBUG ("TOTAL REMOTE CONTACTS:" << remoteContacts.size());

    if (!remoteContacts.isEmpty()) {
        QMap<int, UContactsStatus> statusMap;
        QList<QContact> cpyContacts(remoteContacts);
        if (d->mContactBackend->addContacts(cpyContacts, &statusMap)) {
            // TODO: Saving succeeded. Update sync results
            syncSuccess = true;

            // sync report
            addProcessedItem(Sync::ITEM_ADDED,
                             Sync::LOCAL_DATABASE,
                             syncTargetId(),
                             cpyContacts.count());
        } else {
            // TODO: Saving failed. Update sync results and probably stop sync
            syncSuccess = false;
        }
    }

    return syncSuccess;
}

bool
UContactsClient::storeToLocalForFastSync(const QList<QContact> &remoteContacts)
{
    FUNCTION_CALL_TRACE;
    Q_D(UContactsClient);
    Q_ASSERT(!d->mSlowSync);

    bool syncSuccess = false;
    LOG_DEBUG ("@@@storeToLocal#FAST SYNC");
    QList<QContact> remoteAddedContacts, remoteModifiedContacts, remoteDeletedContacts;
    filterRemoteAddedModifiedDeletedContacts(remoteContacts,
                                             remoteAddedContacts,
                                             remoteModifiedContacts,
                                             remoteDeletedContacts);

    resolveConflicts(remoteModifiedContacts, remoteDeletedContacts);

    if (!remoteAddedContacts.isEmpty()) {
        LOG_DEBUG ("***Adding " << remoteAddedContacts.size() << " contacts");
        QMap<int, UContactsStatus> addedStatusMap;
        syncSuccess = d->mContactBackend->addContacts(remoteAddedContacts, &addedStatusMap);

        if (syncSuccess) {
            // sync report
            addProcessedItem(Sync::ITEM_ADDED,
                             Sync::LOCAL_DATABASE,
                             syncTargetId(),
                             remoteAddedContacts.count());
        }
    }

    if (!remoteModifiedContacts.isEmpty()) {
        LOG_DEBUG ("***Modifying " << remoteModifiedContacts.size() << " contacts");
        QMap<int, UContactsStatus> modifiedStatusMap =
                d->mContactBackend->modifyContacts(&remoteModifiedContacts);

        syncSuccess = (modifiedStatusMap.size() > 0);

        if (syncSuccess) {
            // sync report
            addProcessedItem(Sync::ITEM_MODIFIED,
                             Sync::LOCAL_DATABASE,
                             syncTargetId(),
                             modifiedStatusMap.size());
        }
    }

    if (!remoteDeletedContacts.isEmpty()) {
        LOG_DEBUG ("***Deleting " << remoteDeletedContacts.size() << " contacts");
        QStringList guidList;
        for (int i=0; i<remoteDeletedContacts.size(); i++) {
            guidList << UContactsBackend::getRemoteId(remoteDeletedContacts.at(i));
        }

        QStringList localIdList = d->mContactBackend->localIds(guidList);
        QMap<int, UContactsStatus> deletedStatusMap =
                d->mContactBackend->deleteContacts(localIdList);

        syncSuccess = (deletedStatusMap.size() > 0);
        if (syncSuccess) {
            // sync report
            addProcessedItem(Sync::ITEM_DELETED,
                             Sync::LOCAL_DATABASE,
                             syncTargetId(),
                             localIdList.size());
        }
    }

    return syncSuccess;
}

bool
UContactsClient::cleanUp()
{
    FUNCTION_CALL_TRACE;
    //TODO
    return true;
}

void UContactsClient::connectivityStateChanged(Sync::ConnectivityType aType, bool aState)
{
    FUNCTION_CALL_TRACE;
    LOG_DEBUG("Received connectivity change event:" << aType << " changed to " << aState);
}

void
UContactsClient::loadLocalContacts(const QDateTime &since)
{
    FUNCTION_CALL_TRACE;
    Q_D(UContactsClient);

    if (!since.isValid()) {
        d->mAllLocalContactIds = d->mContactBackend->getAllContactIds().toSet();

        LOG_DEBUG ("Number of contacts:" << d->mAllLocalContactIds.size ());
    } else {
        d->mAddedContactIds = d->mContactBackend->getAllNewContactIds(since);
        d->mModifiedContactIds = d->mContactBackend->getAllModifiedContactIds(since);
        d->mDeletedContactIds = d->mContactBackend->getAllDeletedContactIds(since);

        LOG_DEBUG ("Number of local added contacts:" << d->mAddedContactIds.size());
        LOG_DEBUG ("Number of local modified contacts:" << d->mModifiedContactIds.size());
        LOG_DEBUG ("Number of local removed contacts:" << d->mDeletedContactIds.size());
    }
}

void
UContactsClient::onStateChanged(Sync::SyncProgressDetail aState)
{
    FUNCTION_CALL_TRACE;

    switch(aState) {
    case Sync::SYNC_PROGRESS_SENDING_ITEMS: {
        emit syncProgressDetail(getProfileName(), Sync::SYNC_PROGRESS_SENDING_ITEMS);
        break;
    }
    case Sync::SYNC_PROGRESS_RECEIVING_ITEMS: {
        emit syncProgressDetail(getProfileName(), Sync::SYNC_PROGRESS_RECEIVING_ITEMS);
        break;
    }
    case Sync::SYNC_PROGRESS_FINALISING: {
        emit syncProgressDetail(getProfileName(), Sync::SYNC_PROGRESS_FINALISING);
        break;
    }
    default:
        //do nothing
        break;
    };
}

void
UContactsClient::onSyncFinished(Sync::SyncStatus aState)
{
    FUNCTION_CALL_TRACE;
    Q_D(UContactsClient);

    switch(aState)
    {
        case Sync::SYNC_ERROR:
        case Sync::SYNC_AUTHENTICATION_FAILURE:
        case Sync::SYNC_DATABASE_FAILURE:
        case Sync::SYNC_CONNECTION_ERROR:
        case Sync::SYNC_NOTPOSSIBLE:
        {
            generateResults(false);
            emit error(getProfileName(), "", aState);
            break;
        }
        case Sync::SYNC_DONE:
            // purge all deleted contacts
            d->mContactBackend->purgecontacts(lastSyncTime());
        case Sync::SYNC_ABORTED:
        {
            generateResults(true);
            emit success(getProfileName(), QString::number(aState));
            break;
        }
        case Sync::SYNC_QUEUED:
        case Sync::SYNC_STARTED:
        case Sync::SYNC_PROGRESS:
        default:
        {
            generateResults(false);
            emit error(getProfileName(), "", aState);
            break;
        }
    }
}

Buteo::SyncResults
UContactsClient::getSyncResults() const
{
    return d_ptr->mResults;
}

QString
UContactsClient::authToken() const
{
    return d_ptr->mAuth->token();
}

QString
UContactsClient::syncTargetId() const
{
    return d_ptr->mContactBackend->syncTargetId();
}

QString UContactsClient::accountName() const
{
    if (d_ptr->mAuth) {
        return d_ptr->mAuth->accountDisplayName();
    }
    return QString();
}

const QDateTime
UContactsClient::lastSyncTime() const
{
    FUNCTION_CALL_TRACE;

    Buteo::ProfileManager pm;
    Buteo::SyncProfile* sp = pm.syncProfile (iProfile.name ());
    QDateTime lastTime = sp->lastSuccessfulSyncTime();
    if (!lastTime.isNull()) {
        // return UTC time used by google
        return lastTime.addSecs(3).toUTC();
    } else {
        return lastTime;
    }
}


void
UContactsClient::filterRemoteAddedModifiedDeletedContacts(const QList<QContact> remoteContacts,
                                                          QList<QContact> &remoteAddedContacts,
                                                          QList<QContact> &remoteModifiedContacts,
                                                          QList<QContact> &remoteDeletedContacts)
{
    FUNCTION_CALL_TRACE;
    Q_D(UContactsClient);

    foreach (const QContact &contact, remoteContacts) {
        if (UContactsBackend::deleted(contact)) {
            remoteDeletedContacts.append(contact);
            continue;
        }

        QString remoteId = UContactsBackend::getRemoteId(contact);
        QContactId localId = d->mContactBackend->entryExists(remoteId);
        if (localId.isNull()) {
            remoteAddedContacts.append(contact);
        } else {
            remoteModifiedContacts.append(contact);
        }
    }
}

void
UContactsClient::resolveConflicts(QList<QContact> &modifiedRemoteContacts,
                                  QList<QContact> &deletedRemoteContacts)
{
    FUNCTION_CALL_TRACE;
    Q_D(UContactsClient);

    // TODO: Handle conflicts. The steps:
    // o Compare the list of local modified/deleted contacts with
    //   the list of remote modified/deleted contacts
    // o Create a new list (a map maybe) that has the contacts to
    //   be modified/deleted using the conflict resolution policy
    //   (server-wins, client-wins, add-new)
    // o Return the list

    //QListIterator<GContactEntry*> iter (modifiedRemoteContacts);
    QList<QContact>::iterator iter;
    for (iter = modifiedRemoteContacts.begin (); iter != modifiedRemoteContacts.end (); ++iter) {
        QContact contact = *iter;
        QString remoteId = UContactsBackend::getRemoteId(contact);

        if (d->mModifiedContactIds.contains(remoteId)) {
            if (d->mConflictResPolicy == Buteo::SyncProfile::CR_POLICY_PREFER_LOCAL_CHANGES) {
                modifiedRemoteContacts.erase(iter);
            } else {
                d->mModifiedContactIds.remove(remoteId);
            }
        }

        if (d->mDeletedContactIds.contains(remoteId)) {
            if (d->mConflictResPolicy == Buteo::SyncProfile::CR_POLICY_PREFER_LOCAL_CHANGES) {
                modifiedRemoteContacts.erase(iter);
            } else {
                d->mDeletedContactIds.remove(remoteId);
            }
        }
    }

    for (iter = deletedRemoteContacts.begin (); iter != deletedRemoteContacts.end (); ++iter) {
        QContact contact = *iter;
        QString remoteId = UContactsBackend::getRemoteId(contact);

        if (d->mModifiedContactIds.contains(remoteId)) {
            if (d->mConflictResPolicy == Buteo::SyncProfile::CR_POLICY_PREFER_LOCAL_CHANGES) {
                deletedRemoteContacts.erase(iter);
            } else {
                d->mModifiedContactIds.remove(remoteId);
            }
        }

        if (d->mDeletedContactIds.contains(remoteId)) {
            // If the entry is deleted both at the server and
            // locally, then just remove it from the lists
            // so that no further action need to be taken
            deletedRemoteContacts.erase(iter);
            d->mDeletedContactIds.remove(remoteId);
        }
    }
}

void
UContactsClient::updateIdsToLocal(const QList<QContact> &contacts)
{
    FUNCTION_CALL_TRACE;
    Q_D(UContactsClient);
    QList<QContact> newList(contacts);
    d->mContactBackend->modifyContacts(&newList);
}

void
UContactsClient::addProcessedItem(Sync::TransferType modificationType,
                                  Sync::TransferDatabase database,
                                  const QString &modifiedDatabase,
                                  int count)
{
    FUNCTION_CALL_TRACE;
    Q_D(UContactsClient);

    Buteo::DatabaseResults& results = d->mItemResults[modifiedDatabase];
    if (database == Sync::LOCAL_DATABASE) {
        if (modificationType == Sync::ITEM_ADDED) {
            results.iLocalItemsAdded += count;
        } else if (modificationType == Sync::ITEM_MODIFIED) {
            results.iLocalItemsModified += count;
        } else if (modificationType == Sync::ITEM_DELETED) {
            results.iLocalItemsDeleted += count;
        }
    } else if (database == Sync::REMOTE_DATABASE) {
        if( modificationType == Sync::ITEM_ADDED) {
            results.iRemoteItemsAdded += count;
        } else if (modificationType == Sync::ITEM_MODIFIED) {
            results.iRemoteItemsModified += count;
        } else if (modificationType == Sync::ITEM_DELETED) {
            results.iRemoteItemsDeleted += count;
        }
    }
}

void
UContactsClient::generateResults(bool aSuccessful)
{
    FUNCTION_CALL_TRACE;
    Q_D(UContactsClient);

    d->mResults = Buteo::SyncResults();
    d->mResults.setMajorCode(aSuccessful ? Buteo::SyncResults::SYNC_RESULT_SUCCESS :
                                           Buteo::SyncResults::SYNC_RESULT_FAILED );
    d->mResults.setTargetId(iProfile.name());
    if (d->mItemResults.isEmpty()) {
        LOG_INFO("No items transferred");
    } else {
        QMapIterator<QString, Buteo::DatabaseResults> i(d->mItemResults);
        while (i.hasNext())
        {
            i.next();
            const Buteo::DatabaseResults &r = i.value();
            Buteo::TargetResults targetResults(i.key(), // Target name
                                               Buteo::ItemCounts(r.iLocalItemsAdded,
                                                                 r.iLocalItemsDeleted,
                                                                 r.iLocalItemsModified),
                                               Buteo::ItemCounts(r.iRemoteItemsAdded,
                                                                 r.iRemoteItemsDeleted,
                                                                 r.iRemoteItemsModified));
            d->mResults.addTargetResults(targetResults);
            LOG_INFO("Sync finished at:" << d->mResults.syncTime().toUTC().toString(Qt::ISODate));
            LOG_INFO("Items for" << targetResults.targetName() << ":");
            LOG_INFO("LA:" << targetResults.localItems().added <<
                     "LD:" << targetResults.localItems().deleted <<
                     "LM:" << targetResults.localItems().modified <<
                     "RA:" << targetResults.remoteItems().added <<
                     "RD:" << targetResults.remoteItems().deleted <<
                     "RM:" << targetResults.remoteItems().modified);
        }
    }
}
