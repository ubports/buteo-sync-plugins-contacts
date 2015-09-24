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

#include <UAbstractRemoteSource.h>

#include <QtContacts/QContact>
#include <QtContacts/QContactManager>

#include <QtCore/QScopedPointer>

QTCONTACTS_USE_NAMESPACE

class MockRemoteSource : public UAbstractRemoteSource
{
    Q_OBJECT
public:
    MockRemoteSource(QObject *parent = 0);
    ~MockRemoteSource();

    // UAbstractRemoteSource
    bool init(const QVariantMap &properties);
    void abort();
    void fetchContacts(const QDateTime &since, bool includeDeleted, bool fetchAvatar = true);

    //test helpers
    bool m_initialized;
    QVariantMap m_properties;
    QtContacts::QContactManager *manager() const;
    void setPageSize(int pageSize);

    int count() const;

protected:
    void saveContactsNonBatch(const QList<QtContacts::QContact> contacts);
    void removeContactsNonBatch(const QList<QtContacts::QContact> contacts);

    void batch(const QList<QtContacts::QContact> &contactsToCreate,
               const QList<QtContacts::QContact> &contactsToUpdate,
               const QList<QtContacts::QContact> &contactsToRemove);

private slots:
    void onContactChanged(const QList<QContactId> &);
    void onContactRemoved(const QList<QContactId> &);
    void onContactCreated(const QList<QContactId> &);

private:
    QScopedPointer<QtContacts::QContactManager> m_manager;
    int m_pageSize;

    QList<QtContacts::QContact> toLocalContacts(const QList<QtContacts::QContact> contacts) const;
    QList<QtContacts::QContact> toRemoteContact(const QList<QtContacts::QContact> contacts) const;

    bool exixts(const QContactId &remoteId) const;
};
