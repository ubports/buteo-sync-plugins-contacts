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

#include "MockRemoteSource.h"

#include <UContactsCustomDetail.h>
#include <UContactsBackend.h>

#include <ProfileEngineDefs.h>
#include <ProfileManager.h>

#include <QtVersit/QVersitReader>
#include <QtVersit/QVersitDocument>
#include <QtVersit/QVersitContactImporter>

#include <QtContacts/QContactGuid>
#include <QtContacts/QContactTimestamp>
#include <QtContacts/QContactIntersectionFilter>
#include <QtContacts/QContactUnionFilter>

#include <QDebug>

#define LOCALID_FIELD_NAME      "X-LOCALID"

QTVERSIT_USE_NAMESPACE
QTCONTACTS_USE_NAMESPACE

MockRemoteSource::MockRemoteSource(QObject *parent)
    : UAbstractRemoteSource(parent),
      m_pageSize(-1)
{
    QMap<QString, QString> params;
    params.insert("id", "remote-source");
    m_manager.reset(new QContactManager("mock", params));
    connect(m_manager.data(), SIGNAL(contactsAdded(QList<QContactId>)),
            this, SLOT(onContactCreated(QList<QContactId>)));
    connect(m_manager.data(), SIGNAL(contactsChanged(QList<QContactId>)),
            this, SLOT(onContactChanged(QList<QContactId>)));
    connect(m_manager.data(), SIGNAL(contactsRemoved(QList<QContactId>)),
            this, SLOT(onContactRemoved(QList<QContactId>)));
}

MockRemoteSource::~MockRemoteSource()
{
}

bool MockRemoteSource::init(const QVariantMap &properties)
{
    m_properties = properties;
    m_initialized = true;
    return true;
}

void MockRemoteSource::abort()
{
}

QContactManager *MockRemoteSource::manager() const
{
    return m_manager.data();
}

void MockRemoteSource::setPageSize(int pageSize)
{
    m_pageSize = pageSize;
}

bool MockRemoteSource::exixts(const QContactId &remoteId) const
{
    if (remoteId.isNull()) {
        return false;
    }

    QContact c = m_manager->contact(remoteId);
    return !c.isEmpty();
}

void MockRemoteSource::fetchContacts(const QDateTime &since, bool includeDeleted, bool fetchAvatar)
{
    Q_UNUSED(fetchAvatar);

    QList<QContact> contacts;
    if (since.isValid()) {
        QContactUnionFilter iFilter;
        QContactChangeLogFilter changed(QContactChangeLogFilter::EventChanged);
        changed.setSince(since);

        QContactChangeLogFilter added(QContactChangeLogFilter::EventAdded);
        added.setSince(since);

        iFilter << changed << added;

        if (includeDeleted) {
            QContactChangeLogFilter removed(QContactChangeLogFilter::EventRemoved);
            removed.setSince(since);
            iFilter << removed;
        }

        contacts = m_manager->contacts(iFilter);
    } else {
        contacts = m_manager->contacts();
    }

    //simulate page fetch
    QList<QContact> localContacts = toLocalContacts(contacts);
    if (m_pageSize > 0) {
        while(!localContacts.isEmpty()) {
            int pageSize = qMin(localContacts.size(), m_pageSize);
            emit contactsFetched(localContacts.mid(0, pageSize),
                                 localContacts.size() > pageSize ? Sync::SYNC_PROGRESS : Sync::SYNC_DONE);
            localContacts = localContacts.mid(pageSize);
        }
    } else {
        emit contactsFetched(localContacts, Sync::SYNC_DONE);
    }
}

int MockRemoteSource::count() const
{
    return m_manager->contactIds().size();
}

void MockRemoteSource::saveContactsNonBatch(const QList<QContact> contacts)
{
    QList<QContact> remoteContacts = toRemoteContact(contacts);
    m_manager->saveContacts(&remoteContacts);
}

void MockRemoteSource::removeContactsNonBatch(const QList<QContact> contacts)
{
    QList<QContactId> ids;
    foreach(const QContact &contact, contacts) {
        QString remoteId = UContactsBackend::getRemoteId(contact);
        if (!remoteId.isEmpty()) {
            ids << QContactId::fromString(remoteId);
        }
    }

    m_manager->removeContacts(ids);
}

void MockRemoteSource::batch(const QList<QContact> &contactsToCreate,
                             const QList<QContact> &contactsToUpdate,
                             const QList<QContact> &contactsToRemove)
{
    QList<QContact> createdContacts;
    QList<QContact> updatedContacts;
    QStringList removedContacts;

    // create
    QList<QContact> remoteContacts = toRemoteContact(contactsToCreate);
    m_manager->saveContacts(&remoteContacts);
    for(int i=0; i < remoteContacts.size(); i++) {
        QContact newContact(remoteContacts.at(i));
        // local id
        QContactGuid guid = newContact.detail<QContactGuid>();
        guid.setGuid(contactsToCreate.at(i).id().toString());
        newContact.saveDetail(&guid);
        // remote id
        UContactsBackend::setRemoteId(newContact, remoteContacts.at(i).id().toString());

        createdContacts << newContact;
    }
    remoteContacts.clear();

    // update
    remoteContacts = toRemoteContact(contactsToUpdate);
    m_manager->saveContacts(&remoteContacts);
    for(int i=0; i < remoteContacts.size(); i++) {
        QContact newContact(remoteContacts.at(i));

        // local id
        QContactGuid guid = newContact.detail<QContactGuid>();
        guid.setGuid(contactsToUpdate.at(i).id().toString());
        newContact.saveDetail(&guid);

        // remote id
        UContactsBackend::setRemoteId(newContact, remoteContacts.at(i).id().toString());

        updatedContacts << newContact;
    }
    remoteContacts.clear();

    // remove
    QList<QContactId> removedIds;
    foreach(const QContact &contact, contactsToRemove) {
        QString remoteId = UContactsBackend::getRemoteId(contact);
        if (!remoteId.isEmpty()) {
            removedIds << QContactId::fromString(remoteId);
            removedContacts << remoteId;
        }
    }
    m_manager->removeContacts(removedIds);

    emit transactionCommited(createdContacts, updatedContacts, removedContacts, Sync::SYNC_DONE);
}

void MockRemoteSource::onContactChanged(const QList<QContactId> &)
{

}

void MockRemoteSource::onContactRemoved(const QList<QContactId> &)
{

}

void MockRemoteSource::onContactCreated(const QList<QContactId> &)
{

}

QList<QContact> MockRemoteSource::toLocalContacts(const QList<QContact> contacts) const
{
    QList<QContact> result;
    foreach(const QContact &c, contacts) {
        QContact localContact(c);
        localContact.setId(QContactId());
        UContactsCustomDetail::setCustomField(localContact, UContactsCustomDetail::FieldRemoteId, c.id().toString());
        result << localContact;
    }
    return result;
}

QList<QContact> MockRemoteSource::toRemoteContact(const QList<QContact> contacts) const
{
    QList<QContact> result;
    foreach(const QContact &c, contacts) {
        QContactId remoteId;
        QContact remoteContact(c);
        foreach(QContactExtendedDetail d, c.details<QContactExtendedDetail>()) {
            if (d.name() == UContactsCustomDetail::FieldRemoteId) {
                // remove remote-id from remote contact (we do not want to save that)
                remoteContact.removeDetail(&d);
                remoteId = QContactId::fromString(d.data().toString());
            }
        }
        if (exixts(remoteId)) {
            remoteContact.setId(remoteId);
        } else {
            remoteContact.setId(QContactId());
        }
        result << remoteContact;
    }


    return result;
}
