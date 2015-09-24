/*
 * This file is part of buteo-sync-plugins-contacts package
 *
 * Copyright (C) 2015 Canonical Ltd
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

#ifndef UABSTRACTREMOTESOURCE_H
#define UABSTRACTREMOTESOURCE_H

#include <QObject>
#include <QDateTime>
#include <QVariantMap>

#include <QtContacts/QContact>

#include <SyncCommonDefs.h>

class UAbstractRemoteSourcePrivate;

class UAbstractRemoteSource : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(UAbstractRemoteSource)

public:

    UAbstractRemoteSource(QObject *parent = 0);
    ~UAbstractRemoteSource();

    virtual void abort() = 0;
    virtual bool init(const QVariantMap &properties) = 0;
    virtual void fetchContacts(const QDateTime &since, bool includeDeleted, bool fetchAvatar = true) = 0;

    /*!
     * \brief Begins a transaction on the remote database.
     */
    void transaction();

    /*!
     * \brief Commits a transaction to the remote database.
     */
    bool commit();

    /*!
     * \brief Rolls back a transaction on the remote database.
     */
    bool rollback();

    virtual void saveContacts(const QList<QtContacts::QContact> &contacts);
    virtual void removeContacts(const QList<QtContacts::QContact> &contacts);

signals:
    void contactsFetched(const QList<QtContacts::QContact> &contacts,
                         Sync::SyncStatus status,
                         qreal progress);

    /*!
     * \brief This signal is emitted, when a remote contact is created
     * \param contacts A list of created contacts
     * \param status The operation status
     */
    void contactsCreated(const QList<QtContacts::QContact> &contacts, Sync::SyncStatus status);

    /*!
     * \brief This signal is emitted, when a remote contact is changed
     * \param contacts A list of changed contacts
     * \param status The operation status
     */
    void contactsChanged(const QList<QtContacts::QContact> &contacts, Sync::SyncStatus status);

    /*!
     * \brief This signal is emitted, when a remote contact is removed
     * \param ids A list with remoteId of removed contacts
     * \param status The operation status
     */
    void contactsRemoved(const QStringList &ids, Sync::SyncStatus status);

    /*!
     * \brief This signal is emitted, when a batch operation finishes
     * \param createdContacts A list of created contacts
     * \param updatedContacts A list of updated contacts
     * \param removedContacts A list with remoteId of removed contacts
     * \param errorMap A map with the list of errors found during the batch operation
     * the key value contains the contact local id and the value is a int based
     * on QContactManager::Error enum
     * \param status The operation status
     */
    void transactionCommited(const QList<QtContacts::QContact> &createdContacts,
                             const QList<QtContacts::QContact> &updatedContacts,
                             const QStringList &removedContacts,
                             const QMap<QString, int> &errorMap,
                             Sync::SyncStatus status);

protected:
    virtual void batch(const QList<QtContacts::QContact> &contactsToCreate,
                       const QList<QtContacts::QContact> &contactsToUpdate,
                       const QList<QtContacts::QContact> &contactsToRemove) = 0;

    virtual void saveContactsNonBatch(const QList<QtContacts::QContact> contacts) = 0;
    virtual void removeContactsNonBatch(const QList<QtContacts::QContact> contacts) = 0;

private:
    QScopedPointer<UAbstractRemoteSourcePrivate> d_ptr;
};

Q_DECLARE_METATYPE(QList<QtContacts::QContact>)

#endif // UABSTRACTREMOTESOURCE_H
