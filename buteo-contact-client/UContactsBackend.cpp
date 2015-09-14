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

#include "config.h"
#include "UContactsBackend.h"
#include "UContactsCustomDetail.h"

#include <LogMacros.h>

#include <QContactTimestamp>
#include <QContactIdFilter>
#include <QContactIntersectionFilter>
#include <QContactSyncTarget>
#include <QContactDetailFilter>
#include <QContactGuid>
#include <QContactDisplayLabel>
#include <QContactExtendedDetail>
#include <QContactSyncTarget>

#include <QBuffer>
#include <QSet>
#include <QHash>

#include <QDBusInterface>
#include <QDBusReply>

static const QString CPIM_SERVICE_NAME             ("com.canonical.pim");
static const QString CPIM_ADDRESSBOOK_OBJECT_PATH  ("/com/canonical/pim/AddressBook");
static const QString CPIM_ADDRESSBOOK_IFACE_NAME   ("com.canonical.pim.AddressBook");

UContactsBackend::UContactsBackend(const QString &managerName, QObject* parent)
    : QObject (parent),
      iMgr(new QContactManager(managerName))
{
    FUNCTION_CALL_TRACE;
}

UContactsBackend::~UContactsBackend()
{
    FUNCTION_CALL_TRACE;

    delete iMgr;
    iMgr = NULL;
}

bool
UContactsBackend::init(uint syncAccount, const QString &syncTarget)
{
    FUNCTION_CALL_TRACE;

    // create address book it it does not exists
    // check if the source already exists
    QContactDetailFilter filter;
    filter.setDetailType(QContactDetail::TypeType, QContactType::FieldType);
    filter.setValue(QContactType::TypeGroup);

    QList<QContact> sources = iMgr->contacts(filter);
    Q_FOREACH(const QContact &contact, sources) {
        QContactExtendedDetail exd = UContactsCustomDetail::getCustomField(contact,
                                                                           "ACCOUNT-ID");
        if (!exd.isEmpty() && (exd.data().toUInt() == syncAccount)) {
            mSyncTargetId = contact.detail<QContactGuid>().guid();
            reloadCache();
            return true;
        }
    }

    // memory/mock manager does not support syncTarget
    if (iMgr->managerName() != "mock") {
        // create a new source if necessary
        QContact contact;
        contact.setType(QContactType::TypeGroup);

        QContactDisplayLabel label;
        label.setLabel(syncTarget);
        contact.saveDetail(&label);

        // set the new source as default
        QContactExtendedDetail isDefault;
        isDefault.setName("IS-PRIMARY");
        isDefault.setData(true);
        contact.saveDetail(&isDefault);

        // Link source with account
        QContactExtendedDetail accountId;
        accountId.setName("ACCOUNT-ID");
        accountId.setData(syncAccount);
        contact.saveDetail(&accountId);

        if (!iMgr->saveContact(&contact)) {
            qWarning() << "Fail to create contact source:" << syncTarget;
            return false;
        }

        mSyncTargetId = contact.detail<QContactGuid>().guid();
    }

    return true;
}

bool
UContactsBackend::uninit()
{
    FUNCTION_CALL_TRACE;
    mRemoteIdToLocalId.clear();

    return true;
}

QList<QContactId>
UContactsBackend::getAllContactIds()
{
    FUNCTION_CALL_TRACE;
    Q_ASSERT (iMgr);
    return iMgr->contactIds(getSyncTargetFilter());
}

RemoteToLocalIdMap
UContactsBackend::getAllNewContactIds(const QDateTime &aTimeStamp)
{
    FUNCTION_CALL_TRACE;
    LOG_DEBUG("Retrieve New Contacts Since " << aTimeStamp);

    RemoteToLocalIdMap idList;
    const QContactChangeLogFilter::EventType eventType =
            QContactChangeLogFilter::EventAdded;

    getSpecifiedContactIds(eventType, aTimeStamp, &idList);

    return idList;
}

RemoteToLocalIdMap
UContactsBackend::getAllModifiedContactIds(const QDateTime &aTimeStamp)
{

    FUNCTION_CALL_TRACE;

    LOG_DEBUG("Retrieve Modified Contacts Since " << aTimeStamp);

    RemoteToLocalIdMap idList;
    const QContactChangeLogFilter::EventType eventType =
            QContactChangeLogFilter::EventChanged;

    getSpecifiedContactIds(eventType, aTimeStamp, &idList);

    return idList;
}

RemoteToLocalIdMap
UContactsBackend::getAllDeletedContactIds(const QDateTime &aTimeStamp)
{
    FUNCTION_CALL_TRACE;
    LOG_DEBUG("Retrieve Deleted Contacts Since " << aTimeStamp);

    RemoteToLocalIdMap idList;
    const QContactChangeLogFilter::EventType eventType =
            QContactChangeLogFilter::EventRemoved;

    getSpecifiedContactIds(eventType, aTimeStamp, &idList);

    return idList;
}

bool
UContactsBackend::addContacts(QList<QContact>& aContactList,
                              QMap<int, UContactsStatus> *aStatusMap)
{
    FUNCTION_CALL_TRACE;
    Q_ASSERT(iMgr);
    Q_ASSERT(aStatusMap);

    QMap<int, QContactManager::Error> errorMap;

    // Check if contact already exists if it exists set the contact id
    // to cause an update instead of create a new one
    for(int i=0; i < aContactList.size(); i++) {
        QContact &c = aContactList[i];
        QString remoteId = getRemoteId(c);
        QContactId id = entryExists(remoteId);
        if (!id.isNull()) {
            c.setId(id);
        } else {
            // make sure that all contacts retrieved are saved on the correct sync target
            QContactSyncTarget syncTarget = c.detail<QContactSyncTarget>();
            syncTarget.setSyncTarget(syncTargetId());
            c.saveDetail(&syncTarget);
        }

        // remove guid field if it exists
        QContactGuid guid = c.detail<QContactGuid>();
        if (!guid.isEmpty()) {
            c.removeDetail(&guid);
        }
    }

    bool retVal = iMgr->saveContacts(&aContactList, &errorMap);
    if (!retVal) {
        LOG_WARNING( "Errors reported while saving contacts:" << iMgr->error() );
    }

    // QContactManager will populate errorMap only for errors, but we use this as a status map,
    // so populate NoError if there's no error.
    for (int i = 0; i < aContactList.size(); i++)
    {
        UContactsStatus status;
        status.id = i;
        if (!errorMap.contains(i)) {
            status.errorCode = QContactManager::NoError;

            // update remote id map
            const QContact &c = aContactList.at(i);
            QString remoteId = getRemoteId(c);
            mRemoteIdToLocalId.insert(remoteId, c.id());
        } else {
            LOG_WARNING("Contact with id " <<  aContactList.at(i).id() << " and index " << i <<" is in error");
            status.errorCode = errorMap.value(i);
        }
        aStatusMap->insert(i, status);
    }

    return retVal;
}

QMap<int,UContactsStatus>
UContactsBackend::modifyContacts(QList<QContact> *aContactList)
{
    FUNCTION_CALL_TRACE;

    Q_ASSERT (iMgr);
    UContactsStatus status;

    QMap<int,QContactManager::Error> errors;
    QMap<int,UContactsStatus> statusMap;

    // WORKAROUND: Our backend uses GUid as contact id due problems with contact id serialization
    // we can not use this field
    for (int i = 0; i < aContactList->size(); i++) {
        QContact &newContact = (*aContactList)[i];
        QString remoteId = getRemoteId(newContact);

        // if the contact was created the remoteId will not exists on local database
        QContactId localId = entryExists(remoteId);

        // nt this case we should use the guid stored on contact
        QContactGuid guid = newContact.detail<QContactGuid>();

        if (localId.isNull() && !guid.isEmpty()) {
            // try the guid (should contains the local id) field
            localId = QContactId::fromString(guid.guid());
        }
        newContact.setId(localId);
        newContact.removeDetail(&guid);
    }

    if(iMgr->saveContacts(aContactList , &errors)) {
        LOG_DEBUG("Batch Modification of Contacts Succeeded");
    } else {
        LOG_DEBUG("Batch Modification of Contacts Failed");
    }

    // QContactManager will populate errorMap only for errors, but we use this as a status map,
    // so populate NoError if there's no error.
    // TODO QContactManager populates indices from the aContactList, but we populate keys, is this OK?
    for (int i = 0; i < aContactList->size(); i++) {
        const QContact &c = aContactList->at(i);
        QContactId contactId = c.id();
        if( !errors.contains(i) ) {
            LOG_DEBUG("No error for contact with id " << contactId << " and index " << i);
            status.errorCode = QContactManager::NoError;
            statusMap.insert(i, status);

            // update remote it map
            QString oldRemoteId = mRemoteIdToLocalId.key(c.id());
            mRemoteIdToLocalId.remove(oldRemoteId);

            QString remoteId = getRemoteId(c);
            mRemoteIdToLocalId.insert(remoteId, c.id());
        } else {
            LOG_DEBUG("contact with id " << contactId << " and index " << i <<" is in error");
            QContactManager::Error errorCode = errors.value(i);
            status.errorCode = errorCode;
            statusMap.insert(i, status);
        }
    }
    return statusMap;
}

QMap<int, UContactsStatus>
UContactsBackend::deleteContacts(const QStringList &aContactIDList)
{
    FUNCTION_CALL_TRACE;

    QList<QContactId> qContactIdList;
    foreach (QString id, aContactIDList) {
        qContactIdList.append(QContactId::fromString(id));
    }

    return deleteContacts(qContactIdList);
}

QMap<int, UContactsStatus>
UContactsBackend::deleteContacts(const QList<QContactId> &aContactIDList) {
    FUNCTION_CALL_TRACE;

    Q_ASSERT (iMgr);
    UContactsStatus status;
    QMap<int, QContactManager::Error> errors;
    QMap<int, UContactsStatus> statusMap;

    if(iMgr->removeContacts(aContactIDList , &errors)) {
        LOG_DEBUG("Successfully Removed all contacts ");
    }
    else {
        LOG_WARNING("Failed Removing Contacts");
    }

    // QContactManager will populate errorMap only for errors, but we use this as a status map,
    // so populate NoError if there's no error.
    for (int i = 0; i < aContactIDList.size(); i++) {
        const QContactId &contactId = aContactIDList.at(i);
        if( !errors.contains(i) )
        {
            LOG_DEBUG("No error for contact with id " << contactId << " and index " << i);
            status.errorCode = QContactManager::NoError;
            statusMap.insert(i, status);

            // remove from remote id map
            QString remoteId = mRemoteIdToLocalId.key(contactId);
            mRemoteIdToLocalId.remove(remoteId);
        }
        else
        {
            LOG_DEBUG("contact with id " << contactId << " and index " << i <<" is in error");
            QContactManager::Error errorCode = errors.value(i);
            status.errorCode = errorCode;
            statusMap.insert(i, status);
        }
    }

    return statusMap;
}


void
UContactsBackend::getSpecifiedContactIds(const QContactChangeLogFilter::EventType aEventType,
                                         const QDateTime& aTimeStamp,
                                         RemoteToLocalIdMap *aIdList)
{
    FUNCTION_CALL_TRACE;
    Q_ASSERT(aIdList);

    QList<QContactId> localIdList;
    QContactChangeLogFilter filter(aEventType);
    filter.setSince(aTimeStamp);

    localIdList = iMgr->contactIds(filter  & getSyncTargetFilter());
    LOG_DEBUG("Local ID added =  " << localIdList.size() << "    Datetime from when this " << aTimeStamp.toString());
    // Filter out ids for items that were added after the specified time.
    if (aEventType != QContactChangeLogFilter::EventAdded)
    {
        filter.setEventType(QContactChangeLogFilter::EventAdded);
        QList<QContactId> addedList = iMgr->contactIds(filter  & getSyncTargetFilter());
        foreach (const QContactId &id, addedList)
        {
            localIdList.removeAll(id);
        }
    }

    // This is a defensive procedure to prevent duplicate items being sent.
    // QSet does not allow duplicates, thus transforming QList to QSet and back
    // again will remove any duplicate items in the original QList.
    int originalIdCount = localIdList.size();
    QSet<QContactId> idSet = localIdList.toSet();
    int idCountAfterDupRemoval = idSet.size();

    LOG_DEBUG("Item IDs found (returned / incl. duplicates): " << idCountAfterDupRemoval << "/" << originalIdCount);
    if (originalIdCount != idCountAfterDupRemoval) {
        LOG_WARNING("Contacts backend returned duplicate items for requested list");
        LOG_WARNING("Duplicate item IDs have been removed");
    } // no else

    localIdList = idSet.toList();

    QContactFetchHint remoteIdHint;
    QList <QContactDetail::DetailType> detailTypes;
    detailTypes << QContactExtendedDetail::Type;
    remoteIdHint.setDetailTypesHint(detailTypes);

    QList<QContact> contacts = iMgr->contacts(localIdList, remoteIdHint);
    foreach (const QContact &contact, contacts) {
        QString rid = getRemoteId(contact);
        aIdList->insertMulti(rid, contact.id());
    }
}

/*!
    \fn GContactsBackend::getContact(QContactId aContactId)
 */
QContact
UContactsBackend::getContact(const QContactId& aContactId)
{
    FUNCTION_CALL_TRACE;
    Q_ASSERT (iMgr);
    QList<QContact> returnedContacts;

    LOG_DEBUG("Contact ID to be retreived = " << aContactId.toString());
    returnedContacts = iMgr->contacts(QList<QContactId>() << aContactId);

    LOG_DEBUG("Contacts retreived from Contact manager  = " << returnedContacts.count());
    return returnedContacts.value(0, QContact());
}

QContact
UContactsBackend::getContact(const QString& remoteId)
{
    FUNCTION_CALL_TRACE;
    Q_ASSERT (iMgr);
    LOG_DEBUG("Remote id to be searched for = " << remoteId);

    // use contact id if possible
    QContactId cId = entryExists(remoteId);
    if (!cId.isNull()) {
        return iMgr->contact(cId);
    }
    return QContact();
}

QContactId
UContactsBackend::entryExists(const QString remoteId)
{
    if (remoteId.isEmpty()) {
        return QContactId();
    }

    // check cache
    return mRemoteIdToLocalId.value(remoteId);
}

QString
UContactsBackend::syncTargetId() const
{
    return mSyncTargetId;
}

const QStringList
UContactsBackend::localIds(const QStringList remoteIds)
{
    QStringList localIdList;
    foreach (QString guid , remoteIds) {
        QString localId = entryExists(guid).toString();
        if (!localId.isEmpty()) {
            localIdList << localId;
        }
    }
    Q_ASSERT(localIdList.count() == remoteIds.count());
    return localIdList;
}

void UContactsBackend::reloadCache()
{
    FUNCTION_CALL_TRACE;
    QContactFetchHint hint;
    QList<QContactSortOrder> sortOrder;
    QContactFilter sourceFilter;
    if (!mSyncTargetId.isEmpty()) {
        sourceFilter = getSyncTargetFilter();
    }

    mRemoteIdToLocalId.clear();
    hint.setDetailTypesHint(QList<QContactDetail::DetailType>() << QContactExtendedDetail::Type);
    Q_FOREACH(const QContact &c,  iMgr->contacts(sourceFilter, sortOrder, hint)) {
        QString remoteId = getRemoteId(c);
        if (!remoteId.isEmpty()) {
            mRemoteIdToLocalId.insert(remoteId, c.id());
        }
    }
}

QString
UContactsBackend::getRemoteId(const QContact &contact)
{
    return UContactsCustomDetail::getCustomField(contact, UContactsCustomDetail::FieldRemoteId).data().toString();
}

void UContactsBackend::setRemoteId(QContact &contact, const QString &remoteId)
{
    UContactsCustomDetail::setCustomField(contact, UContactsCustomDetail::FieldRemoteId, QVariant(remoteId));
}

QString UContactsBackend::getLocalId(const QContact &contact)
{
    QContactGuid guid = contact.detail<QContactGuid>();
    return guid.guid();
}

void UContactsBackend::setLocalId(QContact &contact, const QString &localId)
{
    QContactGuid guid = contact.detail<QContactGuid>();
    guid.setGuid(localId);
    contact.saveDetail(&guid);
}

bool UContactsBackend::deleted(const QContact &contact)
{
    QString deletedAt = UContactsCustomDetail::getCustomField(contact, UContactsCustomDetail::FieldDeletedAt).data().toString();
    return !deletedAt.isEmpty();
}

void
UContactsBackend::purgecontacts(const QDateTime &date)
{
    QDBusInterface iface(CPIM_SERVICE_NAME,
                         CPIM_ADDRESSBOOK_OBJECT_PATH,
                         CPIM_ADDRESSBOOK_IFACE_NAME);
    QDBusReply<void> reply = iface.call("purgeContacts", date.toString(Qt::ISODate), mSyncTargetId);
    if (reply.error().isValid()) {
        LOG_WARNING("Fail to purge contacts" << reply.error());
    } else {
        LOG_DEBUG("Purged backend contacts");
    }
}

QContactFilter
UContactsBackend::getSyncTargetFilter() const
{
    // user entered contacts, i.e. all other contacts that are not sourcing
    // from restricted backends or instant messaging service
    static QContactDetailFilter detailFilterDefaultSyncTarget;

    if (!mSyncTargetId.isEmpty() &&
        detailFilterDefaultSyncTarget.value().isNull()) {
        detailFilterDefaultSyncTarget.setDetailType(QContactSyncTarget::Type,
                                                    QContactSyncTarget::FieldSyncTarget + 1);
        detailFilterDefaultSyncTarget.setValue(mSyncTargetId);
    } else if (mSyncTargetId.isEmpty()) {
        return QContactFilter();
    }

    // return the union
    return detailFilterDefaultSyncTarget;
}


QtContacts::QContactManager *UContactsBackend::manager() const
{
    return iMgr;
}
