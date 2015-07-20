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

#ifndef UCONTACSTCLIENT_H
#define UCONTACSTCLIENT_H

#include <QNetworkReply>
#include <QContact>
#include <QList>
#include <QPair>

#include <ClientPlugin.h>

class UContactsClientPrivate;
class UAuth;
class UAbstractRemoteSource;
class UContactsBackend;

class UContactsClient : public Buteo::ClientPlugin
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(UContactsClient)

public:

    /*! \brief Constructor
     *
     * @param aPluginName Name of this client plugin
     * @param aProfile Sync profile
     * @param aCbInterface Pointer to the callback interface
     * @param authenticator a instance of UAuth class to be used during the authentication
     */
    UContactsClient(const QString& aPluginName,
                    const Buteo::SyncProfile &aProfile,
                    Buteo::PluginCbInterface *aCbInterface,
                    const QString& serviceName);

    /*! \brief Destructor
     *
     * Call uninit before destroying the object.
     */
    virtual ~UContactsClient();

    //! @see SyncPluginBase::init
    virtual bool init();

    //! @see SyncPluginBase::uninit
    virtual bool uninit();

    //! @see ClientPlugin::startSync
    virtual bool startSync();

    //! @see SyncPluginBase::abortSync
    virtual void abortSync(Sync::SyncStatus aStatus = Sync::SYNC_ABORTED);

    //! @see SyncPluginBase::getSyncResults
    virtual Buteo::SyncResults getSyncResults() const;

    //! @see SyncPluginBase::cleanUp
    virtual bool cleanUp();

public slots:
    //! @see SyncPluginBase::connectivityStateChanged
    virtual void connectivityStateChanged( Sync::ConnectivityType aType,
                                           bool aState );

protected:
    QString authToken() const;
    QString syncTargetId() const;
    QString accountName() const;

    // Must be implemented by the plugins
    virtual UAbstractRemoteSource* createRemoteSource(QObject *parent) const = 0;
    virtual QVariantMap remoteSourceProperties() const = 0;

    virtual UContactsBackend* createContactsBackend(QObject *parent) const;
    virtual UAuth* crateAuthenticator(QObject *parent) const;

    virtual bool isReadyToSync() const;
    virtual const QDateTime lastSyncTime() const;

signals:
    void stateChanged(Sync::SyncProgressDetail progress);
    void itemProcessed(Sync::TransferType type,
                       Sync::TransferDatabase db,
                       int committedItems);
    void syncFinished(Sync::SyncStatus);


private:
    QScopedPointer<UContactsClientPrivate> d_ptr;

    void loadLocalContacts(const QDateTime &since);

    bool initConfig();
    void generateResults(bool aSuccessful);
    void updateIdsToLocal(const QList<QtContacts::QContact> &contacts);
    void filterRemoteAddedModifiedDeletedContacts(const QList<QTCONTACTS_PREPEND_NAMESPACE(QContact)> remoteContacts,
                                                  QList<QTCONTACTS_PREPEND_NAMESPACE(QContact)> &remoteAddedContacts,
                                                  QList<QTCONTACTS_PREPEND_NAMESPACE(QContact)> &remoteModifiedContacts,
                                                  QList<QTCONTACTS_PREPEND_NAMESPACE(QContact)> &remoteDeletedContacts);
    void resolveConflicts(QList<QTCONTACTS_PREPEND_NAMESPACE(QContact)> &modifiedRemoteContacts,
                          QList<QTCONTACTS_PREPEND_NAMESPACE(QContact)> &deletedRemoteContacts);
    void addProcessedItem(Sync::TransferType modificationType,
                          Sync::TransferDatabase database,
                          const QString &modifiedDatabase,
                          int count = 1);
    QList<QTCONTACTS_PREPEND_NAMESPACE(QContact)> prepareContactsToUpload(UContactsBackend *backend,
                                                                          const QSet<QTCONTACTS_PREPEND_NAMESPACE(QContactId)> &ids);

    /* slow sync */
    bool storeToLocalForSlowSync(const QList<QTCONTACTS_PREPEND_NAMESPACE(QContact)> &remoteContacts);

    /* fast sync */
    bool storeToLocalForFastSync(const QList<QTCONTACTS_PREPEND_NAMESPACE(QContact)> &remoteContacts);

private slots:
    bool start();
    void onAuthenticationError();
    void onStateChanged(Sync::SyncProgressDetail progress);
    void onSyncFinished(Sync::SyncStatus status);

    /* slow sync */
    void onRemoteContactsFetchedForSlowSync(const QList<QtContacts::QContact> contacts,
                                            Sync::SyncStatus status);
    void onContactsSavedForSlowSync(const QList<QtContacts::QContact> &createdContacts,
                                    const QList<QtContacts::QContact> &updatedContacts,
                                    const QStringList &removedContacts,
                                    Sync::SyncStatus status);
    /* fast sync */
    void onRemoteContactsFetchedForFastSync(const QList<QtContacts::QContact> contacts,
                                            Sync::SyncStatus status);
    void onContactsSavedForFastSync(const QList<QtContacts::QContact> &createdContacts,
                                    const QList<QtContacts::QContact> &updatedContacts,
                                    const QStringList &removedContacts,
                                    Sync::SyncStatus status);

};

#endif // UCONTACTCLIENT_H
