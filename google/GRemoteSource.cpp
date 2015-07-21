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

#include "GRemoteSource.h"
#include "GTransport.h"
#include "GConfig.h"
#include "GContactStream.h"
#include "GContactImageDownloader.h"
#include "GContactImageUploader.h"
#include "buteosyncfw_p.h"

#include <UContactsBackend.h>
#include <UContactsCustomDetail.h>

#include <ProfileEngineDefs.h>
#include <ProfileManager.h>

#include <QtContacts/QContact>
#include <QtContacts/QContactGuid>
#include <QtContacts/QContact>


QTCONTACTS_USE_NAMESPACE

GRemoteSource::GRemoteSource(QObject *parent)
    : UAbstractRemoteSource(parent),
      mTransport(new GTransport),
      mState(GRemoteSource::STATE_IDLE),
      mStartIndex(0),
      mFetchAvatars(true)
{
    connect(mTransport.data(),
            SIGNAL(finishedRequest()),
            SLOT(networkRequestFinished()));

    connect(mTransport.data(),
            SIGNAL(error(int)),
            SLOT(networkError(int)));
}

GRemoteSource::~GRemoteSource()
{
}

bool GRemoteSource::init(const QVariantMap &properties)
{
    FUNCTION_CALL_TRACE;
    LOG_DEBUG("Creating HTTP transport");

    if (mState != GRemoteSource::STATE_IDLE) {
        LOG_WARNING("Init called with wrong state" << mState);
        return false;
    }

    mAccountName = properties.value("ACCOUNT-NAME").toString();
    mAuthToken = properties.value("AUTH-TOKEN").toString();
    mSyncTarget = properties.value("SYNC-TARGET").toString();
    mRemoteUri = properties.value(Buteo::KEY_REMOTE_DATABASE).toString();
    mStartIndex = 1;
    mPendingBatchOps.clear();
    mState = GRemoteSource::STATE_IDLE;

    if (mRemoteUri.isEmpty()) {
        // Set to the default value
        mRemoteUri = QStringLiteral("https://www.google.com/m8/feeds/contacts/default/full/");
    }

    LOG_DEBUG("Setting remote URI to" << mRemoteUri);
    mTransport->setUrl(mRemoteUri);

    QString proxyHost = properties.value(Buteo::KEY_HTTP_PROXY_HOST).toString();
    // Set proxy, if available
    if (!proxyHost.isEmpty()) {
        QString proxyPort = properties.value(Buteo::KEY_HTTP_PROXY_PORT).toString();
        mTransport->setProxy(proxyHost, proxyPort);
        LOG_DEBUG("Proxy host:" << proxyHost);
        LOG_DEBUG("Proxy port:" << proxyPort);
    } else {
        LOG_DEBUG("Not using proxy");
    }

    return true;
}

void GRemoteSource::abort()
{
    //TODO: Abort
}

void GRemoteSource::fetchContacts(const QDateTime &since, bool includeDeleted, bool fetchAvatar)
{
    FUNCTION_CALL_TRACE;
    if (mState != GRemoteSource::STATE_IDLE) {
        LOG_WARNING("GRemote source is not in idle state, current state is" << mState);
        return;
    }

    mFetchAvatars = fetchAvatar;
    mState = GRemoteSource::STATE_FETCHING_CONTACTS;
    fetchRemoteContacts(since, includeDeleted, 1);
}

void GRemoteSource::fetchAvatars(QList<QContact> *contacts)
{
    // keep downloader object live while GRemoteSource exists to avoid removing
    // the temporary files used to store avatars.
    // The files will be removed when the object get destroyed
    GContactImageDownloader *downloader = new GContactImageDownloader(mAuthToken, this);
    QMap<QUrl, QPair<QContactAvatar, QContact*> > avatars;

    for(int i=0; i < contacts->size(); i++) {
        QContact &c = (*contacts)[i];
        foreach (const QContactAvatar &avatar, c.details<QContactAvatar>()) {
            if (!avatar.imageUrl().isLocalFile()) {
                LOG_DEBUG("Download avatar:" << avatar.imageUrl());
                avatars.insert(avatar.imageUrl(), qMakePair(avatar, &c));
                downloader->push(avatar.imageUrl());
            }
        }
    }

    downloader->exec();

    QMap<QUrl, QUrl> downloaded = downloader->donwloaded();
    foreach (const QUrl &avatarUrl, avatars.keys()) {
        QPair<QContactAvatar, QContact*> &p = avatars[avatarUrl];
        p.first.setImageUrl(downloaded.value(avatarUrl));
        LOG_DEBUG("Set avatar image:" <<  p.first.imageUrl() << downloaded.value(avatarUrl));
        p.second->saveDetail(&p.first);
    }
}

void GRemoteSource::uploadAvatars(QList<QContact> *contacts)
{
    GContactImageUploader uploader(mAuthToken, mAccountName);

    foreach(const QContact &c, *contacts) {
        QString localId = UContactsBackend::getLocalId(c);
        if (mLocalIdToAvatar.contains(localId)) {
            QContactExtendedDetail rEtag =
                    UContactsCustomDetail::getCustomField(c,
                                                          UContactsCustomDetail::FieldContactAvatarETag);

            QPair<QString, QUrl> avatar = mLocalIdToAvatar.value(localId);
            // check if the remote etag has changed
            if (avatar.second.isLocalFile() &&
                (avatar.first.isEmpty() || (avatar.first != rEtag.data().toString()))) {
                QString remoteId = UContactsBackend::getRemoteId(c);
                LOG_DEBUG("Uploade avatar:" << remoteId << avatar.second);
                uploader.push(remoteId, avatar.second);
            } else if (!avatar.second.isLocalFile()) {
                LOG_DEBUG("Contact avatar is not local" << avatar.second);
            } else {
                LOG_DEBUG("Avatar did not change");
            }
        } else {
            LOG_WARNING("Local id not found found on avatar map:" << localId);
        }
    }

    uploader.exec();

    QMap<QString, GContactImageUploader::UploaderReply> result = uploader.result();
    for(int i =0; i < contacts->size(); i++) {
        QContact &c = (*contacts)[i];

        GContactImageUploader::UploaderReply reply = result.value(UContactsBackend::getRemoteId(c));
        if (!reply.newEtag.isEmpty()) {
            UContactsCustomDetail::setCustomField(c,
                                                  UContactsCustomDetail::FieldContactETag,
                                                  reply.newEtag);
        }
        if (!reply.newAvatarEtag.isEmpty()) {
            QString localId = UContactsBackend::getLocalId(c);

            // copy local url to new remote avatar
            QContactAvatar avatar = c.detail<QContactAvatar>();
            avatar.setImageUrl(mLocalIdToAvatar.value(localId).second);
            c.saveDetail(&avatar);

            UContactsCustomDetail::setCustomField(c,
                                                  UContactsCustomDetail::FieldContactAvatarETag,
                                                  reply.newAvatarEtag);
        }
    }
}

void GRemoteSource::saveContactsNonBatch(const QList<QContact> contacts)
{
    FUNCTION_CALL_TRACE;
    if (mState != GRemoteSource::STATE_IDLE) {
        LOG_WARNING("GRemote source is not in idle state, current state is" << mState);
        return;
    }

    mState = GRemoteSource::STATE_BATCH_RUNNING;
    foreach (const QContact &contact, contacts) {
        QString remoteId = UContactsBackend::getRemoteId(contact);
        if (remoteId.isEmpty()) {
            mPendingBatchOps.insertMulti(GoogleContactStream::Add,
                                         qMakePair(contact, QStringList()));
        } else {
            mPendingBatchOps.insertMulti(GoogleContactStream::Modify,
                                         qMakePair(contact, QStringList()));
        }
    }

    batchOperationContinue();
}

void GRemoteSource::removeContactsNonBatch(const QList<QContact> contacts)
{
    FUNCTION_CALL_TRACE;
    if (mState != GRemoteSource::STATE_IDLE) {
        LOG_WARNING("GRemote source is not in idle state, current state is" << mState);
        return;
    }

    mState = GRemoteSource::STATE_BATCH_RUNNING;
    foreach (const QContact &contact, contacts) {
        mPendingBatchOps.insertMulti(GoogleContactStream::Remove,
                                     qMakePair(contact, QStringList()));
    }

    batchOperationContinue();
}

void GRemoteSource::batch(const QList<QContact> &contactsToCreate,
                          const QList<QContact> &contactsToUpdate,
                          const QList<QContact> &contactsToRemove)
{
    FUNCTION_CALL_TRACE;
    if (mState != GRemoteSource::STATE_IDLE) {
        LOG_WARNING("GRemote source is not in idle state, current state is" << mState);
        emit transactionCommited(QList<QContact>(),
                                 QList<QContact>(),
                                 QStringList(),
                                 Sync::SYNC_ERROR);
        return;
    }

    mLocalIdToAvatar.clear();
    mState = GRemoteSource::STATE_BATCH_RUNNING;

    foreach (const QContact &contact, contactsToCreate) {
        QString localID = UContactsBackend::getLocalId(contact);
        QString avatarEtag =
                UContactsCustomDetail::getCustomField(contact,
                                                      UContactsCustomDetail::FieldContactAvatarETag).data().toString();

        mLocalIdToAvatar.insert(QString("qtcontacts:galera::%1").arg(localID),
                                qMakePair(avatarEtag, contact.detail<QContactAvatar>().imageUrl()));
        mPendingBatchOps.insertMulti(GoogleContactStream::Add,
                                     qMakePair(contact, QStringList()));
    }

    foreach (const QContact &contact, contactsToUpdate) {
        QString localID = UContactsBackend::getLocalId(contact);
        QString avatarEtag =
                UContactsCustomDetail::getCustomField(contact,
                                                      UContactsCustomDetail::FieldContactAvatarETag).data().toString();

        mLocalIdToAvatar.insert(QString("qtcontacts:galera::%1").arg(localID),
                                qMakePair(avatarEtag, contact.detail<QContactAvatar>().imageUrl()));
        mPendingBatchOps.insertMulti(GoogleContactStream::Modify,
                                     qMakePair(contact, QStringList()));
    }

    foreach (const QContact &contact, contactsToRemove) {
        mPendingBatchOps.insertMulti(GoogleContactStream::Remove,
                                     qMakePair(contact, QStringList()));
    }

    batchOperationContinue();
}

void
GRemoteSource::batchOperationContinue()
{
    FUNCTION_CALL_TRACE;

    int limit = qMin(mPendingBatchOps.size(), GConfig::MAX_RESULTS);
    // no pending batch ops
    if (limit < 1)  {
        LOG_DEBUG ("No pending operations");
        emit transactionCommited(QList<QContact>(),
                                 QList<QContact>(),
                                 QStringList(),
                                 Sync::SYNC_DONE);
        return;
    }
    QMultiMap<GoogleContactStream::UpdateType, QPair<QContact, QStringList> > batchPage;
    QPair<QContact, QStringList> value;

    mLocalIdToContact.clear();

    while (batchPage.size() < limit) {
        GoogleContactStream::UpdateType type;

        if (!mPendingBatchOps.values(GoogleContactStream::Add).isEmpty()) {
            type = GoogleContactStream::Add;
        } else if (!mPendingBatchOps.values(GoogleContactStream::Modify).isEmpty()) {
            type = GoogleContactStream::Modify;
        } else if (!mPendingBatchOps.values(GoogleContactStream::Remove).isEmpty()) {
            type = GoogleContactStream::Remove;
        }

        value = mPendingBatchOps.values(type).first();
        mPendingBatchOps.remove(type, value);
        batchPage.insertMulti(type, value);

        // keep contacts to handle error if necessary
        mLocalIdToContact.insert(UContactsBackend::getLocalId(value.first),
                                 value.first);
    }

    GoogleContactStream encoder(false, mAccountName);
    QByteArray encodedContacts = encoder.encode(batchPage);

    mTransport->reset();
    mTransport->setUrl(mRemoteUri + "batch");
    mTransport->setGDataVersionHeader();
    mTransport->setAuthToken(mAuthToken);
    mTransport->setData(encodedContacts);
    mTransport->addHeader("Content-Type", "application/atom+xml; charset=UTF-8; type=feed");
    LOG_TRACE("POST DATA:" << encodedContacts);
    mTransport->request(GTransport::POST);
}

bool GRemoteSource::handleUploadError(const GoogleContactAtom::BatchOperationResponse &response,
                                      const QContact &contact,
                                      QList<QContact> *removedContacts)
{
    // try to solve the sync error
    // return 'true' if the error was solved or 'false' if it fails to solve the error

    if ((response.code == "404") && (response.type == "update")) {
        // contact not found on remote side:
        // In this case we will notify that the contact was removed
        // This can happen if the remote contact was removed manually while the sync operation was running
        removedContacts->append(contact);
        return true;
    }

    return false;
}

void GRemoteSource::emitTransactionCommited(const QList<QContact> &created,
                                            const QList<QContact> &changed,
                                            const QList<QContact> &removed,
                                            Sync::SyncStatus status)
{
    FUNCTION_CALL_TRACE;
    LOG_INFO("ADDED:" << created.size() <<
             "CHANGED" << changed.size() <<
             "REMOVED" << removed.size());

    if (!created.isEmpty()) {
        emit contactsCreated(created, status);
    }

    if (!changed.isEmpty()) {
        emit contactsChanged(changed, status);
    }

    QStringList removedIds;
    foreach(const QContact &c, removed) {
        removedIds << UContactsCustomDetail::getCustomField(c, UContactsCustomDetail::FieldRemoteId).data().toString();
    }

    if (!removedIds.isEmpty()) {
        emit contactsRemoved(removedIds, status);
    }

    emit transactionCommited(created, changed, removedIds, status);
}

void
GRemoteSource::fetchRemoteContacts(const QDateTime &since, bool includeDeleted, int startIndex)
{
    FUNCTION_CALL_TRACE;
    /**
     o Get last sync time
     o Get etag value from local file system (this is a soft etag)
     o Connect finishedRequest to parseResults & network error slots
     o Use mTransport to perform network fetch
    */
    if (since.isValid()) {
        mTransport->setUpdatedMin(since);
    }

    if (startIndex > 1) {
        mTransport->setStartIndex(startIndex);
    }

    mTransport->setMaxResults(GConfig::MAX_RESULTS);
    if (includeDeleted) {
        mTransport->setShowDeleted();
    }

    mTransport->setGDataVersionHeader();
    mTransport->addHeader(QByteArray("Authorization"),
                          QString("Bearer " + mAuthToken).toUtf8());
    mTransport->request(GTransport::GET);
}

/**
  * The state machine is pretty much maintained in this method.
  * Maybe it is better to create a separate class that can handle
  * the sync state. It can act of Qt signals that will be emitted
  * by this method
  */
void
GRemoteSource::networkRequestFinished()
{
    FUNCTION_CALL_TRACE;

    // o Error - if network error, set the sync results with the code
    // o Call uninit
    // o Stop sync
    // o If success, invoke the mParser->parse ()
    Sync::SyncStatus syncStatus = Sync::SYNC_ERROR;
    GTransport::HTTP_REQUEST_TYPE requestType = mTransport->requestType();
    if (mTransport->hasReply()) {
        QByteArray data = mTransport->replyBody();
        LOG_TRACE(data);
        if (data.isNull () || data.isEmpty()) {
            LOG_INFO("Nothing returned from server");
            syncStatus = Sync::SYNC_CONNECTION_ERROR;
            goto operationFailed;
        }

        GoogleContactStream parser(false);
        GoogleContactAtom *atom = parser.parse(data);
        if (!atom) {
            LOG_CRITICAL("NULL atom object. Something wrong with parsing");
            goto operationFailed;
        }

        if ((requestType == GTransport::POST) ||
            (requestType == GTransport::PUT)) {
            QList<QContact> addedContacts;
            QList<QContact> modContacts;
            QList<QContact> delContacts;

            LOG_DEBUG("@@@PREVIOUS REQUEST TYPE=POST");
            QMap<QString, GoogleContactAtom::BatchOperationResponse> operationResponses = atom->batchOperationResponses();
            QMap<QString, QString> batchOperationRemoteIdToType;
            QMap<QString, QString> batchOperationRemoteToLocalId;

            bool errorOccurredInBatch = false;
            LOG_DEBUG("RESPONSE SIZE:" << operationResponses.size());
            foreach (const GoogleContactAtom::BatchOperationResponse &response, operationResponses) {
                if (response.isError) {
                    LOG_CRITICAL("batch operation error:\n"
                              "    id:     " << response.operationId << "\n"
                              "    type:   " << response.type << "\n"
                              "    code:   " << response.code << "\n"
                              "    reason: " << response.reason << "\n"
                              "    descr:  " << response.reasonDescription << "\n");

                    errorOccurredInBatch = errorOccurredInBatch || !handleUploadError(response, mLocalIdToContact.value(response.operationId), &delContacts);
                } else {
                    LOG_DEBUG("RESPONSE" << response.contactGuid << response.type);
                    batchOperationRemoteToLocalId.insert(response.contactGuid, response.operationId);
                    batchOperationRemoteIdToType.insert(response.contactGuid, response.type);
                }
            }
            if (errorOccurredInBatch) {
                LOG_CRITICAL("error occurred during batch operation with Google account");
                syncStatus = Sync::SYNC_ERROR;
                goto operationFailed;
                return;
            }

            QList<QPair<QContact, QStringList> > remoteAddModContacts = atom->entryContacts();
            for (int i = 0; i < remoteAddModContacts.size(); i++) {
                QContact &c = remoteAddModContacts[i].first;
                QContactGuid guid = c.detail<QContactGuid>();
                QString cRemoteId = guid.guid();
                if (cRemoteId.isEmpty()) {
                    LOG_WARNING("Remote contact without remote id");
                    continue;
                }
                UContactsBackend::setRemoteId(c, cRemoteId);
                UContactsBackend::setLocalId(c, batchOperationRemoteToLocalId.value(cRemoteId, ""));

                QString opType = batchOperationRemoteIdToType.value(cRemoteId, "");
                c.removeDetail(&guid);
                if (opType == QStringLiteral("insert")) {
                    addedContacts << c;
                } else {
                    modContacts << c;
                }
            }
            delContacts += atom->deletedEntryContacts();

            if (!atom->nextEntriesUrl().isEmpty()) {
                syncStatus = Sync::SYNC_PROGRESS;
            } else {
                //TODO: avatars
                syncStatus = Sync::SYNC_DONE;
                mState = GRemoteSource::STATE_IDLE;
            }

            uploadAvatars(&addedContacts);
            uploadAvatars(&modContacts);

            emitTransactionCommited(addedContacts, modContacts, delContacts, syncStatus);

            if (syncStatus == Sync::SYNC_PROGRESS) {
                batchOperationContinue();
            } else {
                mLocalIdToContact.clear();
                mLocalIdToAvatar.clear();
            }
        } else if (requestType == GTransport::GET) {
            LOG_DEBUG ("@@@PREVIOUS REQUEST TYPE=GET");
            if (mState != GRemoteSource::STATE_FETCHING_CONTACTS) {
                LOG_WARNING("Received a network request finish but the state is not fetching contacts" << mState);
                return;
            }

            LOG_INFO("received information about" <<
                     atom->entryContacts().size() << "add/mod contacts and " <<
                     atom->deletedEntryContacts().size() << "del contacts");

            QList<QContact> remoteContacts;

            // for each remote contact, there are some associated XML elements which
            // could not be stored in QContactDetail form (eg, link URIs etc).
            // build up some datastructures to help us retrieve that information
            // when we need it.
            // we also store the etag data out-of-band to avoid spurious contact saves
            // when the etag changes are reported by the remote server.
            // finally, we can set the id of the contact.
            QList<QPair<QContact, QStringList> > remoteAddModContacts = atom->entryContacts();
            for (int i = 0; i < remoteAddModContacts.size(); ++i) {
                QContact c = remoteAddModContacts[i].first;
                QContactGuid guid =  c.detail<QContactGuid>();
                UContactsBackend::setRemoteId(c, guid.guid());
                c.removeDetail(&guid);
                // FIXME: This code came from the meego implementation, until now we did not face
                // any unsupported xml element. Keep the code here in case some unsupported element
                // apper. Then we should store it some how on our backend.
                //  m_unsupportedXmlElements[accountId].insert(
                //          c.detail<QContactGuid>().guid(),
                //          remoteAddModContacts[i].second);
                //  m_contactEtags[accountId].insert(c.detail<QContactGuid>().guid(), c.detail<QContactOriginMetadata>().id());
                // c.setId(QContactId::fromString(m_contactIds[accountId].value(c.detail<QContactGuid>().guid())));
                // m_remoteAddMods[accountId].append(c);
                remoteContacts << c;
            }

            if (mFetchAvatars) {
                fetchAvatars(&remoteContacts);
            }

            QList<QContact> remoteDelContacts = atom->deletedEntryContacts();
            for (int i = 0; i < remoteDelContacts.size(); ++i) {
                QContact c = remoteDelContacts[i];
                QContactGuid guid =  c.detail<QContactGuid>();
                UContactsBackend::setRemoteId(c, guid.guid());
                c.removeDetail(&guid);
                // FIXME
                // c.setId(QContactId::fromString(m_contactIds[accountId].value(c.detail<QContactGuid>().guid())));
                // m_contactAvatars[accountId].remove(c.detail<QContactGuid>().guid()); // just in case the avatar was outstanding.
                // m_remoteDels[accountId].append(c);
                remoteContacts << c;
            }

            bool hasMore = (!atom->nextEntriesUrl().isNull() ||
                            !atom->nextEntriesUrl().isEmpty());
            if (hasMore) {
                // Request for the next batch
                // This condition will make this slot to be
                // called again and again until there are no more
                // entries left to be fetched from the server
                mStartIndex += GConfig::MAX_RESULTS;
                mTransport->setUrl(atom->nextEntriesUrl());
                syncStatus = Sync::SYNC_PROGRESS;
                LOG_DEBUG("Has more contacts to retrieve");
            } else {
                LOG_DEBUG("NO contacts to retrieve");
                syncStatus = Sync::SYNC_DONE;
                mState = GRemoteSource::STATE_IDLE;
            }

            LOG_TRACE("NOTIFY CONTACTS FETCHED:" << remoteContacts.size());
            emit contactsFetched(remoteContacts, syncStatus);

            if (hasMore) {
                LOG_DEBUG("FETCH MORE CONTACTS FROM INDEX:" << mStartIndex);
                fetchRemoteContacts(QDateTime(),
                                    mTransport->showDeleted(),
                                    mStartIndex);
            }
        }
        delete atom;
    }
    return;

operationFailed:
    switch(mState) {
    case GRemoteSource::STATE_FETCHING_CONTACTS:
        contactsFetched(QList<QContact>(), syncStatus);
        break;
    case GRemoteSource::STATE_BATCH_RUNNING:
        emitTransactionCommited(QList<QContact>(),
                                QList<QContact>(),
                                QList<QContact>(),
                                syncStatus);
        break;
    default:
        break;
    }
    mState = GRemoteSource::STATE_IDLE;
}

void
GRemoteSource::networkError(int errorCode)
{
    FUNCTION_CALL_TRACE;

    Sync::SyncStatus syncStatus = Sync::SYNC_ERROR;
    switch (errorCode)
    {
    case 400:
        // Bad request. Better to bail out, since it could be a problem with the
        // data format of the request/response
        syncStatus = Sync::SYNC_BAD_REQUEST;
        break;
    case 401:
        syncStatus = Sync::SYNC_AUTHENTICATION_FAILURE;
        break;
    case 403:
    case 408:
        syncStatus = Sync::SYNC_ERROR;
        break;
    case 500:
    case 503:
    case 504:
        // Server failures
        syncStatus = Sync::SYNC_SERVER_FAILURE;
        break;
    default:
        break;
    };

    switch(mState) {
    case GRemoteSource::STATE_FETCHING_CONTACTS:
        contactsFetched(QList<QContact>(), syncStatus);
        break;
    case GRemoteSource::STATE_BATCH_RUNNING:
        emitTransactionCommited(QList<QContact>(),
                                QList<QContact>(),
                                QList<QContact>(),
                                syncStatus);
        break;
    default:
        break;
    }
    mState = GRemoteSource::STATE_IDLE;
}

const GTransport *GRemoteSource::transport() const
{
    return mTransport.data();
}

int GRemoteSource::state() const
{
    return mState;
}
