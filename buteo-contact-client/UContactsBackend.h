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

#ifndef UCONTACTSBACKEND_H_
#define UCONTACTSBACKEND_H_

#include <QContact>
#include <QContactId>
#include <QContactFetchRequest>
#include <QContactExtendedDetail>
#include <QContactChangeLogFilter>
#include <QContactManager>

#include <QStringList>

QTCONTACTS_USE_NAMESPACE

struct UContactsStatus
{
    int id;
    QContactManager::Error errorCode;
};

typedef QMultiMap<QString, QContactId> RemoteToLocalIdMap;

//! \brief Harmattan Contact storage plugin backend interface class
///
/// This class interfaces with the QtContact backend implementation
class UContactsBackend : public QObject
{

public:
    explicit UContactsBackend(const QString &managerName = "", QObject* parent = 0);

    /*!
     * \brief Destructor
     */
    ~UContactsBackend();

    /*!
     * \brief Initialize the backend, must be called before any other function
     * \param syncTarget The name of the collection used to store contacts
     * \return Returns true if initialized with sucess false otherwise
     */
    bool init(uint syncAccount, const QString &syncTarget);

    /*!
     * \brief releases the resources held.
     * @returnReturns true if sucess false otherwise
     */
    bool uninit();

    /*!
     * \brief Return ids of all contacts stored locally
     * @return List of contact IDs
     */
    QList<QContactId> getAllContactIds();


    /*!
     * \brief Return all new contacts ids in a QList of QStrings
     * @param aTimeStamp Timestamp of the oldest contact ID to be returned
     * @return List of contact IDs
     */
    RemoteToLocalIdMap getAllNewContactIds(const QDateTime& aTimeStamp);

    /*!
     * \brief Return all modified contact ids in a QList of QStrings
     * @param aTimeStamp Timestamp of the oldest contact ID to be returned
     * @return List of contact IDs
     */
    RemoteToLocalIdMap getAllModifiedContactIds(const QDateTime& aTimeStamp);


    /*!
     * \brief Return all deleted contacts ids in a QList of QStrings
     * @param aTimeStamp Timestamp of the oldest contact ID to be returned
     * @return List of contact IDs
     */
    RemoteToLocalIdMap getAllDeletedContactIds(const QDateTime& aTimeStamp);

    /*!
     * \brief Get contact data for a given contact ID as a QContact object
     * @param aContactId The ID of the contact
     * @return The data of the contact
     */
    QContact getContact(const QContactId& aContactId);

    /*!
     * \brief Returns a contact for the specified remoteId
     * @param remoteId The remote id of the contact to be returned
     * @return The data of the contact
     */
    QContact getContact(const QString& remoteId);

    /*!
     * \brief Batch addition of contacts
     * @param aContactDataList Contact data
     * @param aStatusMap Returned status data
     * @return Errors
     */
    bool addContacts(QList<QContact>& aContactList,
                     QMap<int, UContactsStatus> *aStatusMap );

    // Functions for modifying contacts
    /*!
     * \brief Batch modification
     * @param aContactDataList Contact data
     * @param aContactsIdList Contact IDs
     * @return Errors
     */
    QMap<int, UContactsStatus> modifyContacts(QList<QtContacts::QContact> *aContactList);

    /*!
     * \brief Batch deletion of contacts
     * @param aContactIDList Contact IDs
     * @return Errors
     */
    QMap<int, UContactsStatus> deleteContacts(const QStringList &aContactIDList);

    /*!
     * \brief Batch deletion of contacts
     * @param aContactIDList Contact IDs
     * @return Errors
     */
    QMap<int, UContactsStatus> deleteContacts(const QList<QContactId> &aContactIDList);

    /*!
     * \brief Check if a contact exists
     * \param remoteId The remoteId of the contact
     * \return The localId of the contact
     */
    QContactId entryExists(const QString remoteId);

    /*!
     * \brief Retrieve the current address book used by the backend
     * \return The address book id
     */
    QString syncTargetId() const;

    /*!
     * \brief Retrieve the local id of a list of remote ids
     * \param remoteIds A list with remote ids
     * \return A list with local ids
     */
    const QStringList localIds(const QStringList remoteIds);

    /*!
     * \brief Return the value of the remote id field in a contact
     * \param contact The contact object
     * \return A string with the remoteId
     */
    static QString getRemoteId(const QContact &contact);

    /*!
     * \brief Update the value of the remote id field in a contact
     * \param contact The contact object
     * \param remoteId A string with the remoteId
     */
    static void setRemoteId(QContact &contact, const QString &remoteId);

    /*!
     * \brief Return the valueof the local id field in a contact
     * \param contact The contact object
     * \return A string with the localId
     */
    static QString getLocalId(const QContact &contact);

    /*!
     * \brief Update the value of the local id field in a contact
     * \param contact The contact object
     * \param remoteId A string with the localId
     */
    static void setLocalId(QContact &contact, const QString &localId);

    /*!
     * \brief Check if the contact is marked as deleted
     * \param contact The contact object
     * \return Returns true if the contact is marked as deleted, otherwise returns false
     */
    static bool deleted(const QContact &contact);

    /*!
     * \brief Purge all deleted contacts from the server
     */
    void purgecontacts(const QDateTime &date);

    QContactManager *manager() const;

private: // functions

    /*!
     * \brief Returns contact IDs specified by event type and timestamp
     * @param aEventType Added/changed/removed contacts
     * @param aTimeStamp Contacts older than aTimeStamp are filtered out
     * @param aIdList Returned contact IDs
     */
    void getSpecifiedContactIds(const QContactChangeLogFilter::EventType aEventType,
                                const QDateTime &aTimeStamp,
                                RemoteToLocalIdMap *aIdList);

    /*!
     * \brief Constructs and returns the filter for accessing only contacts allowed to be synchronized
     * Contacts not allowed to be synchronized are Instant messaging contacts and contacts with origin from other sync backends;
     * those contacts have QContactSyncTarget::SyncTarget value different from address book or buteo sync clients.
     * It is designed that buteo sync clients don't restrict access to contacts among themselves
     * - value for QContactSyncTarget::SyncTarget written by this backend is "buteo".
     */
    QContactFilter getSyncTargetFilter() const;

    QContactFilter getRemoteIdFilter(const QString &remoteId) const;

private: // data

    // if there is more than one Manager we need to have a list of Managers
    QContactManager     *iMgr;      ///< A pointer to contact manager
    QString             mSyncTargetId;


    void createSourceForAccount(uint accountId, const QString &label);
};

#endif /* CONTACTSBACKEND_H_ */



