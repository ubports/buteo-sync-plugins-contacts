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

#include "config-tests.h"
#include "GRemoteSource.h"
#include "GTransport.h"
#include "GConfig.h"

#include <UContactsBackend.h>
#include <UContactsCustomDetail.h>

#include <ProfileEngineDefs.h>

#include <QtVersit>
#include <QtContacts>
#include <QtContacts/QContact>
#include <QtCore>
#include <QtTest>

QTVERSIT_USE_NAMESPACE
QTCONTACTS_USE_NAMESPACE

class GRemoteSourceTest : public QObject
{
    Q_OBJECT
private:
    int mGooglePage;

    QList<QContact> fullContacts()
    {
        QFile vcardFile(TEST_DATA_DIR + QStringLiteral("google_contact_full_fetch.vcf"));
        if (!vcardFile.open(QIODevice::ReadOnly)) {
            qWarning() << "Fail to open:" << vcardFile.fileName();
            return QList<QContact>();
        }

        QVersitReader reader(vcardFile.readAll());
        reader.startReading();
        reader.waitForFinished();
        if (reader.results().count() > 0) {
            QList<QVersitDocument> documents = reader.results();
            QVersitContactImporter contactImporter;
            if (!contactImporter.importDocuments(documents)) {
                qWarning() << "Fail to import contacts";
                return QList<QContact>();
            }

            return contactImporter.contacts();
        }
        return QList<QContact>();
    }

private Q_SLOTS:
    void onFetchContactsRequested(const QUrl &url, QByteArray *data)
    {
        Q_UNUSED(url);
        // populate data with the contacts data
        QFile fetchFile(TEST_DATA_DIR + QString("google_contact_full_fetch_page_%1.txt").arg(mGooglePage));
        if (fetchFile.open(QIODevice::ReadOnly)) {
            data->append(fetchFile.readAll());
            fetchFile.close();
        }
        mGooglePage++;
    }

    void onSaveNotFoundContactsRequested(const QUrl &url, QByteArray *data)
    {
        Q_UNUSED(url);
        // populate data with the contacts data
        QFile fetchFile(TEST_DATA_DIR + QString("google_not_found_contact_response.txt"));
        if (fetchFile.open(QIODevice::ReadOnly)) {
            data->append(fetchFile.readAll());
            fetchFile.close();
        }
    }

    void onCreateContactRequested(const QUrl &url, QByteArray *data)
    {
        Q_UNUSED(url);
        // populate data with the contacts data
        QFile fetchFile(TEST_DATA_DIR + QStringLiteral("google_contact_created_page.txt"));
        if (fetchFile.open(QIODevice::ReadOnly)) {
            data->append(fetchFile.readAll());
            fetchFile.close();
        }
    }

    void onUpdatedContactRequested(const QUrl &url, QByteArray *data)
    {
        Q_UNUSED(url);
        // populate data with the contacts data
        QFile fetchFile(TEST_DATA_DIR + QStringLiteral("google_contact_updated_reply.txt"));
        if (fetchFile.open(QIODevice::ReadOnly)) {
            data->append(fetchFile.readAll());
            fetchFile.close();
        }
    }

    void initTestCase()
    {
        qRegisterMetaType<QMap<QString,QString> >("QMap<QString,QString>");
    }

    void testInitialization()
    {
        QScopedPointer<GRemoteSource> src(new GRemoteSource());
        QVariantMap props;
        props.insert(Buteo::KEY_REMOTE_DATABASE, "http://google.com/contacts");
        props.insert(Buteo::KEY_HTTP_PROXY_HOST, "http://proxy.com");
        props.insert(Buteo::KEY_HTTP_PROXY_PORT, 8080);
        src->init(props);

        QCOMPARE(src->state(), 0);
        QCOMPARE(src->transport()->property("URL").toString(), QStringLiteral("http://google.com/contacts"));
        QCOMPARE(src->transport()->property("ProxyHost").toString(), QStringLiteral("http://proxy.com"));
        QCOMPARE(src->transport()->property("ProxyPort").toInt(), 8080);
    }

    void testFetchContacts()
    {
        mGooglePage = 0;

        QList<QContact> remoteContacts;
        QScopedPointer<GRemoteSource> src(new GRemoteSource());
        QVariantMap props;
        props.insert(Buteo::KEY_REMOTE_DATABASE, "http://google.com/contacts");
        props.insert("AUTH-TOKEN", "1234567890");
        src->init(props);

        QSignalSpy contactsFetched(src.data(), SIGNAL(contactsFetched(QList<QtContacts::QContact>,Sync::SyncStatus)));
        connect(src->transport(), SIGNAL(requested(QUrl,QByteArray*)), SLOT(onFetchContactsRequested(QUrl,QByteArray*)));
        src->fetchContacts(QDateTime(), false, false);

        QTRY_COMPARE(contactsFetched.count(), 2);

        QVERIFY(!src->transport()->property("UpdatedMin").toDateTime().isValid());
        QCOMPARE(src->transport()->property("MaxResults").toInt(), GConfig::MAX_RESULTS);

        QVariantMap headers = src->transport()->property("Headers").value<QVariantMap>();
        QCOMPARE(headers["Authorization"].toString(), QStringLiteral("Bearer 1234567890"));
        QCOMPARE(headers["GData-Version"].toString(), QStringLiteral("3.0"));

        QTRY_COMPARE(contactsFetched.count(), 2);

        // first page signal
        QList<QVariant> arguments = contactsFetched.takeFirst();
        QList<QContact> contacts = arguments.at(0).value<QList<QtContacts::QContact> >();
        QCOMPARE(arguments.at(1).toInt(), int(Sync::SYNC_PROGRESS));
        QCOMPARE(contacts.size(), 10);
        remoteContacts += contacts;

        // second page signal
        arguments = contactsFetched.takeFirst();
        contacts = arguments.at(0).value<QList<QtContacts::QContact> >();
        QCOMPARE(arguments.at(1).toInt(), int(Sync::SYNC_DONE));
        QCOMPARE(contacts.size(), 5);
        remoteContacts += contacts;

        QList<QContact> expectedContacts = fullContacts();
        QCOMPARE(remoteContacts.size(), expectedContacts.size());
        foreach(const QContact &c, remoteContacts) {
            QString cRID = UContactsCustomDetail::getCustomField(c, UContactsCustomDetail::FieldRemoteId).data().toString();
            bool found = false;
            foreach(const QContact &e, expectedContacts) {
                QString eRID = UContactsCustomDetail::getCustomField(e, UContactsCustomDetail::FieldRemoteId).data().toString();
                if (cRID == eRID) {
                    found = true;
                    QList<QContactDetail> detA = c.details();
                    QList<QContactDetail> detB = e.details();
                    if (detA.size() != detB.size()) {
                        qDebug() << detA;
                        qDebug() << detB;
                    }
                    QCOMPARE(detA.size(), detB.size());
                    for(int i = 0; i < detA.size(); i++) {
                        // skip timestamp since this will be update to the current time while importing
                        if (detA[i].type() == QContactTimestamp::Type) {
                            QContactTimestamp tA(detA[i]);
                            QContactTimestamp tB(detB[i]);
                            qDebug() << detA[i] << detB[i];
                            QCOMPARE(tA.created(), tB.created());
                        } else {
                            if (detA[i] != detB[i]) {
                                qWarning() << "DETAILS NOT EQUAL\n\t" << detA[i] << "\n\t" << detB[i];
                            }
                            QCOMPARE(detA[i], detB[i]);
                        }
                    }
                }
            }
            QVERIFY2(found, "remote contact not found in local contact list");
        }
    }

    void testCreateContact()
    {
        mGooglePage = 0;

        QScopedPointer<GRemoteSource> src(new GRemoteSource());
        QVariantMap props;
        props.insert(Buteo::KEY_REMOTE_DATABASE, "http://google.com/contacts");
        props.insert("AUTH-TOKEN", "1234567890");
        props.insert("ACCOUNT-NAME", "renato_teste_2@gmail.com");
        src->init(props);
        connect(src->transport(),
                SIGNAL(requested(QUrl,QByteArray*)),
                SLOT(onCreateContactRequested(QUrl,QByteArray*)));

        QContact c;
        QContactName nm;
        nm.setFirstName("Renato");
        nm.setLastName("Oliveira Filho");
        c.saveDetail(&nm);

        QList<QContact> lc;
        lc << c;

        QSignalSpy onTransactionCommited(src.data(),
                                         SIGNAL(transactionCommited(QList<QtContacts::QContact>, QList<QtContacts::QContact>,QStringList,Sync::SyncStatus)));
        QSignalSpy onContactCreated(src.data(),
                                    SIGNAL(contactsCreated(QList<QtContacts::QContact>,Sync::SyncStatus)));

        src->transaction();
        src->saveContacts(lc);
        src->commit();

         // check create command data
        QCOMPARE(QString::fromLatin1(src->transport()->property("DATA").toByteArray()),
                 QStringLiteral("<atom:feed xmlns:atom=\"http://www.w3.org/2005/Atom\" "
                                            "xmlns:gContact=\"http://schemas.google.com/contact/2008\" "
                                            "xmlns:gd=\"http://schemas.google.com/g/2005\" "
                                            "xmlns:batch=\"http://schemas.google.com/gdata/batch\">"
                                "<atom:entry>"
                                    "<batch:id>qtcontacts:::</batch:id>"
                                    "<batch:operation type=\"insert\"/>"
                                    "<atom:category schema=\"http://schemas.google.com/g/2005#kind\" term=\"http://schemas.google.com/contact/2008#contact\"/>"
                                    "<gd:name>"
                                        "<gd:givenName>Renato</gd:givenName>"
                                        "<gd:familyName>Oliveira Filho</gd:familyName>"
                                    "</gd:name>"
                                    "<gContact:groupMembershipInfo deleted=\"false\" href=\"http://www.google.com/m8/feeds/groups/renato_teste_2@gmail.com/base/6\"/>"
                                "</atom:entry></atom:feed>\n"));

        QTRY_COMPARE(onContactCreated.count(), 1);
        QTRY_COMPARE(onTransactionCommited.count(), 1);

        QList<QVariant> transactionCommitedArgs = onTransactionCommited.first();
        QCOMPARE(transactionCommitedArgs.size(), 4);
        QList<QContact> createdContacts = transactionCommitedArgs.at(0).value<QList<QtContacts::QContact> >();
        QList<QContact> updatedContacts = transactionCommitedArgs.at(1).value<QList<QtContacts::QContact> >();
        QStringList removedContacts = transactionCommitedArgs.at(2).value<QStringList >();
        Sync::SyncStatus syncStatus = transactionCommitedArgs.at(3).value<Sync::SyncStatus>();

        QCOMPARE(createdContacts.size(), 1);
        QCOMPARE(createdContacts.at(0).detail<QContactGuid>().guid(),
                 QStringLiteral("qtcontacts:galera::f55c2eeb760ffd6843d2e98319d3544ff3e987b5"));
        QCOMPARE(UContactsCustomDetail::getCustomField(createdContacts.at(0), UContactsCustomDetail::FieldRemoteId).data().toString(),
                 QStringLiteral("5f66bbf78bed2765"));

        QCOMPARE(updatedContacts.size(), 0);
        QCOMPARE(removedContacts.size(), 0);
        QCOMPARE(syncStatus, Sync::SYNC_DONE);
    }

    void testModifyAContact()
    {
        mGooglePage = 0;

        QDateTime currentDateTime = QDateTime::fromString(QStringLiteral("2015-07-06T20:17:06.117Z"), Qt::ISODate);

        QScopedPointer<GRemoteSource> src(new GRemoteSource());
        QVariantMap props;
        props.insert(Buteo::KEY_REMOTE_DATABASE, "http://google.com/contacts");
        props.insert("AUTH-TOKEN", "1234567890");
        props.insert("ACCOUNT-NAME", "renato_teste_2@gmail.com");
        src->init(props);
        connect(src->transport(),
                SIGNAL(requested(QUrl,QByteArray*)),
                SLOT(onUpdatedContactRequested(QUrl,QByteArray*)));

        QContact c;
        c.setId(QContactId::fromString("qtcontacts::memory:99999"));

        QContactGuid guid;
        guid.setGuid("2c79456ba52fb29ecab9afedc4068f3421b77779");
        c.saveDetail(&guid);

        QContactTimestamp ts;
        ts.setLastModified(currentDateTime);
        c.saveDetail(&ts);

        QContactName nm;
        nm.setFirstName("Renato");
        nm.setLastName("Oliveira Filho");
        c.saveDetail(&nm);

        QContactAvatar avatar;
        avatar.setImageUrl(QUrl::fromLocalFile("/tmp/avatar.png"));
        c.saveDetail(&avatar);

        UContactsCustomDetail::setCustomField(c, UContactsCustomDetail::FieldRemoteId, QVariant("012345"));

        QList<QContact> lc;
        lc << c;

        QSignalSpy onTransactionCommited(src.data(),
                                         SIGNAL(transactionCommited(QList<QtContacts::QContact>, QList<QtContacts::QContact>,QStringList,Sync::SyncStatus)));
        QSignalSpy onContactChanged(src.data(),
                                    SIGNAL(contactsChanged(QList<QtContacts::QContact>,Sync::SyncStatus)));

        src->transaction();
        src->saveContacts(lc);
        src->commit();

         // check modify command data
        QCOMPARE(QString::fromLatin1(src->transport()->property("DATA").toByteArray()),
                 QStringLiteral("<atom:feed xmlns:atom=\"http://www.w3.org/2005/Atom\" "
                                           "xmlns:gContact=\"http://schemas.google.com/contact/2008\" "
                                           "xmlns:gd=\"http://schemas.google.com/g/2005\" "
                                           "xmlns:batch=\"http://schemas.google.com/gdata/batch\">"
                                "<atom:entry>"
                                    "<batch:id>qtcontacts:::</batch:id>"
                                    "<batch:operation type=\"update\"/>"
                                    "<atom:category schema=\"http://schemas.google.com/g/2005#kind\" term=\"http://schemas.google.com/contact/2008#contact\"/>"
                                    "<atom:id>http://www.google.com/m8/feeds/contacts/renato_teste_2@gmail.com/full/012345</atom:id>"
                                    "<updated>2015-07-06T20:17:06.117Z</updated>"
                                    "<gd:name>"
                                        "<gd:givenName>Renato</gd:givenName>"
                                        "<gd:familyName>Oliveira Filho</gd:familyName>"
                                    "</gd:name>"
                                    "<gContact:groupMembershipInfo deleted=\"false\" href=\"http://www.google.com/m8/feeds/groups/renato_teste_2@gmail.com/base/6\"/>"
                                "</atom:entry></atom:feed>\n"));

        QTRY_COMPARE(onContactChanged.count(), 1);
        QTRY_COMPARE(onTransactionCommited.count(), 1);

        QList<QVariant> transactionCommitedArgs = onTransactionCommited.first();
        QCOMPARE(transactionCommitedArgs.size(), 4);
        QList<QContact> createdContacts = transactionCommitedArgs.at(0).value<QList<QtContacts::QContact> >();
        QList<QContact> updatedContacts = transactionCommitedArgs.at(1).value<QList<QtContacts::QContact> >();
        QStringList removedContacts = transactionCommitedArgs.at(2).value<QStringList >();
        Sync::SyncStatus syncStatus = transactionCommitedArgs.at(3).value<Sync::SyncStatus>();

        QCOMPARE(createdContacts.size(), 0);
        QCOMPARE(updatedContacts.size(), 1);
        QCOMPARE(removedContacts.size(), 0);
        QCOMPARE(syncStatus, Sync::SYNC_DONE);

        QContact newContact(updatedContacts.at(0));

        QList<QContactExtendedDetail> exDetails = newContact.details<QContactExtendedDetail>();
        QCOMPARE(exDetails.size(), 4);

        QCOMPARE(newContact.detail<QContactGuid>().guid(),
                 QStringLiteral("qtcontacts:galera::2c79456ba52fb29ecab9afedc4068f3421b77779"));
        QCOMPARE(UContactsCustomDetail::getCustomField(newContact, UContactsCustomDetail::FieldRemoteId).data().toString(),
                 QStringLiteral("5b56e6f60f3d43d3"));
        QCOMPARE(UContactsCustomDetail::getCustomField(newContact, UContactsCustomDetail::FieldContactETag).data().toString(),
                 QStringLiteral("5b56e6f60f3d43d3-new"));

        // check avatar
        QList<QContactAvatar> avatars = newContact.details<QContactAvatar>();
        QCOMPARE(avatars.size(), 1);
        QCOMPARE(avatars.at(0).imageUrl(), QUrl::fromLocalFile("/tmp/avatar.png"));
        QCOMPARE(UContactsCustomDetail::getCustomField(newContact, UContactsCustomDetail::FieldContactAvatarETag).data().toString(),
                 QString("%1-avatar").arg("5b56e6f60f3d43d3"));

    }

    void testSaveNotFoundContact()
    {
        QScopedPointer<GRemoteSource> src(new GRemoteSource());
        QVariantMap props;
        props.insert(Buteo::KEY_REMOTE_DATABASE, "http://google.com/contacts");
        props.insert("AUTH-TOKEN", "1234567890");
        props.insert("ACCOUNT-NAME", "renato_teste_2@gmail.com");
        src->init(props);
        connect(src->transport(),
                SIGNAL(requested(QUrl,QByteArray*)),
                SLOT(onSaveNotFoundContactsRequested(QUrl,QByteArray*)));

        // prepare contacts
        QList<QContact> contacts;

        // Contact to Update
        QContact c;
        c.setId(QContactId::fromString("qtcontacts::memory:1"));

        QContactGuid guid;
        guid.setGuid("df8fd2e011e64624459c66f8d72417f7559d9c1d");
        c.saveDetail(&guid);

        UContactsCustomDetail::setCustomField(c,
                                              UContactsCustomDetail::FieldRemoteId,
                                              QStringLiteral("415f25f8a58b972"));
        UContactsCustomDetail::setCustomField(c,
                                              UContactsCustomDetail::FieldContactETag,
                                              QStringLiteral("415f25f8a58b972-ETAG"));
        contacts << c;

        // Contact to create
        c = QContact();
        c.setId(QContactId::fromString("qtcontacts::memory:2"));

        guid = QContactGuid();
        guid.setGuid("f55c2eeb760ffd6843d2e98319d3544ff3e987b5");
        c.saveDetail(&guid);

        QContactName name;
        name.setFirstName("My");
        name.setLastName("Contact");
        c.saveDetail(&name);

        contacts << c;

        QSignalSpy onTransactionCommited(src.data(),
                                         SIGNAL(transactionCommited(QList<QtContacts::QContact>, QList<QtContacts::QContact>,QStringList,Sync::SyncStatus)));
        src->transaction();
        src->saveContacts(contacts);
        src->commit();
        QTRY_COMPARE(onTransactionCommited.count(), 1);

        QList<QVariant> transactionCommitedArgs = onTransactionCommited.first();
        QCOMPARE(transactionCommitedArgs.size(), 4);
        QList<QContact> createdContacts = transactionCommitedArgs.at(0).value<QList<QtContacts::QContact> >();
        QList<QContact> updatedContacts = transactionCommitedArgs.at(1).value<QList<QtContacts::QContact> >();
        QStringList removedContacts = transactionCommitedArgs.at(2).value<QStringList >();
        Sync::SyncStatus syncStatus = transactionCommitedArgs.at(3).value<Sync::SyncStatus>();

        QCOMPARE(createdContacts.size(), 1);
        QCOMPARE(updatedContacts.size(), 0);
        QCOMPARE(removedContacts.size(), 1);
        QCOMPARE(syncStatus, Sync::SYNC_DONE);
    }
};

QTEST_MAIN(GRemoteSourceTest)

#include "TestGRemoteSource.moc"
