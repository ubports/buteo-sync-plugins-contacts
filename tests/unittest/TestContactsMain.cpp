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

#include "TestContactsClient.h"

#include "MockAuthenticator.h"
#include "MockRemoteSource.h"

#include "config-tests.h"

#include <UContactsBackend.h>
#include <UContactsCustomDetail.h>

#include <ProfileEngineDefs.h>

#include <QtVersit>
#include <QtContacts>
#include <QtCore>
#include <QtTest>

QTVERSIT_USE_NAMESPACE

class ContactSyncTest : public QObject
{
    Q_OBJECT

private:
    TestContactsClient *m_client;

    Buteo::SyncProfile *loadFromXmlFile(const QString &fName)
    {
        QFile file(fName);
        if (!file.open(QIODevice::ReadOnly)) {
            return 0;
        } // no else

        QDomDocument doc;
        if (!doc.setContent(&file)) {
            file.close();
            return 0;
        } // no else
        file.close();

        return new Buteo::SyncProfile(doc.documentElement());
    }

    QList<QContact> toContacts(const QString &vcardFile,
                               const QDateTime &createdAt = QDateTime())
    {
        QFile file(vcardFile);
        if (!file.open(QIODevice::ReadOnly)) {
            return QList<QContact>();
        }

        QVersitReader reader(file.readAll());
        reader.startReading();
        reader.waitForFinished();
        if (reader.results().count() > 0) {
            QList<QVersitDocument> documents = reader.results();
            QVersitContactImporter contactImporter;
            if (!contactImporter.importDocuments(documents)) {
                qWarning() << "Fail to import contacts";
                return QList<QContact>();
            }

            QList<QContact> contacts = contactImporter.contacts();
            if (createdAt.isValid()) {
                for(int i=0; i < contacts.size(); i++) {
                    QContact &c = contacts[i];
                    QContactTimestamp ts = c.detail<QContactTimestamp>();
                    ts.setCreated(createdAt);
                    c.saveDetail(&ts);
                }
            }
            return contacts;
        }
        return QList<QContact>();
    }


    void importContactsFromVCardFile(QContactManager *target,
                                     const QString &vcardFile,
                                     const QDateTime &createdAt = QDateTime())
    {
        QList<QContact> contacts = toContacts(vcardFile, createdAt);
        QVERIFY(contacts.size() > 0);
        QVERIFY(target->saveContacts(&contacts));
    }

    QString toVCard(const QList<QContact> &contacts)
    {
        QByteArray vcard;
        QVersitWriter writer(&vcard);

        QVersitContactExporter contactExporter;
        if (!contactExporter.exportContacts(contacts)) {
            qWarning() << "Fail to export contacts";
            return "";
        }

        QList<QVersitDocument> documents = contactExporter.documents();
        writer.startWriting(documents);
        writer.waitForFinished();
        return vcard;
    }

    QString toVCard(const QContact &c)
    {
        return toVCard(QList<QContact>() << c);
    }

    bool compareLines(const QString &vcard1, const QString &vcardFile)
    {
        QFile file(vcardFile);
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "Fail to open file:" << vcardFile;
            return false;
        }

        QStringList vcard1Lines = vcard1.split('\n');
        QString vcardFileData = QString(file.readAll());
        QStringList vcard2Lines = vcardFileData.split('\n');
        file.close();

        if (vcard1Lines.size() != vcard2Lines.size()) {
            qWarning() << "Vcard files size does not match:"
                       << "\n\tExpected:" << vcard2Lines.size()
                       << "\n\tCurrent:" << vcard1Lines.size();

            qWarning() << "VCARD1\n" << vcard1;
            qWarning() << "VCARD2\n" << vcardFileData;
            return false;
        }
        for(int l=0; l < vcard1Lines.size(); l++) {
            if (vcard1Lines[l] != vcard2Lines[l]) {
                qWarning() << "Vcard lines are diff:"
                           << "\n\t" << vcard2Lines[l]
                           << "\n\t" << vcard1Lines[l];
                return false;
            }
        }
        return true;
    }

private Q_SLOTS:
    void initTestCase()
    {
        QCoreApplication::addLibraryPath(MOCK_PLUGIN_PATH);
        QVERIFY(QContactManager::availableManagers().contains("mock"));
    }

    void init()
    {
        Buteo::SyncProfile *syncP  = loadFromXmlFile(PROFILE_TEST_FN);
        QVERIFY(syncP);
        m_client = new TestContactsClient("test-plugin",
                                          *syncP, 0);

        QContact c;
        QContactRingtone r;
        r.setAudioRingtoneUrl(QUrl("http://audio.com"));
        r.setVibrationRingtoneUrl(QUrl("http://vibrating.com"));
        r.setVideoRingtoneUrl(QUrl("http://video.com"));
        c.saveDetail(&r);
    }

    void cleanup()
    {
        delete m_client;
        m_client = 0;
    }

    void testInitialization()
    {
        QVERIFY(m_client->init());
        QVERIFY(m_client->m_authenticator);
        QVERIFY(m_client->m_authenticator->m_initialized);
        QVERIFY(m_client->m_remoteSource);
    }

    void testFailToAuthenticate()
    {
        QVERIFY(m_client->init());
        QSignalSpy syncFinishedSpy(m_client, SIGNAL(syncFinished(Sync::SyncStatus)));
        // simulate fail in the authentication
        m_client->m_authenticator->failed();
        // check if the sync finished was fired with the correct arg
        QTRY_COMPARE(syncFinishedSpy.count(), 1);
        QList<QVariant> arguments = syncFinishedSpy.takeFirst();
        QCOMPARE(arguments.at(0).toInt(), int(Sync::SYNC_AUTHENTICATION_FAILURE));
    }

    void testSync_data()
    {
        QTest::addColumn<QDateTime>("createdAt");
        QTest::addColumn<QDateTime>("lastSyncTime");
        QTest::addColumn<QDateTime>("currentDateTime");
        QTest::addColumn<QString>("remoteFile");
        QTest::addColumn<int>("remoteInitalCount");
        QTest::addColumn<QString>("localFile");
        QTest::addColumn<int>("localInitalCount");
        QTest::addColumn<QString>("finalRemoteFile");
        QTest::addColumn<QString>("finalLocalFile");

        QTest::newRow("slow sync new database")     << QDateTime()
                                                    << QDateTime()
                                                    << QDateTime::currentDateTime()
                                                    << "new_database_remote.vcf"
                                                    << 3
                                                    << "new_database_local.vcf"
                                                    << 1
                                                    << "new_database_remote_result.vcf"
                                                    << "new_database_local_result.vcf";

        QTest::newRow("slow local previous synced") << QDateTime()
                                                    << QDateTime()
                                                    << QDateTime::currentDateTime()
                                                    << "local_previous_synced_remote.vcf"
                                                    << 3
                                                    << "local_previous_synced_local.vcf"
                                                    << 1
                                                    << "local_previous_synced_remote_result.vcf"
                                                    << "local_previous_synced_local_result.vcf";

        QTest::newRow("fast sync without changes")  << QDateTime::fromString("2015-06-15T08:00:00", Qt::ISODate)
                                                    << QDateTime::fromString("2015-06-15T10:30:00", Qt::ISODate)
                                                    << QDateTime::currentDateTime()
                                                    << "fast_sync_without_changes_remote.vcf"
                                                    << 4
                                                    << "fast_sync_without_changes_local.vcf"
                                                    << 4
                                                    << "fast_sync_without_changes_remote_result.vcf"
                                                    << "fast_sync_without_changes_local_result.vcf";

        QTest::newRow("fast sync with a remote change")
                                                    << QDateTime::fromString("2015-06-15T08:00:00", Qt::ISODate)
                                                    << QDateTime::fromString("2015-06-15T08:10:00", Qt::ISODate)
                                                    << QDateTime::fromString("2015-06-15T10:00:00", Qt::ISODate)
                                                    << "fast_sync_with_a_remote_change_remote.vcf"
                                                    << 4
                                                    << "fast_sync_with_a_remote_change_local.vcf"
                                                    << 4
                                                    << "fast_sync_with_a_remote_change_remote_result.vcf"
                                                    << "fast_sync_with_a_remote_change_local_result.vcf";

        QTest::newRow("fast sync with a local change")
                                                    << QDateTime::fromString("2015-06-15T08:00:00", Qt::ISODate)
                                                    << QDateTime::fromString("2015-06-15T08:10:00", Qt::ISODate)
                                                    << QDateTime::fromString("2015-06-15T10:00:00", Qt::ISODate)
                                                    << "fast_sync_with_a_local_change_remote.vcf"
                                                    << 4
                                                    << "fast_sync_with_a_local_change_local.vcf"
                                                    << 4
                                                    << "fast_sync_with_a_local_change_remote_result.vcf"
                                                    << "fast_sync_with_a_local_change_local_result.vcf";

        QTest::newRow("fast sync with a local removal")
                                                    << QDateTime::fromString("2015-06-15T08:00:00", Qt::ISODate)
                                                    << QDateTime::fromString("2015-06-15T08:10:00", Qt::ISODate)
                                                    << QDateTime::fromString("2015-06-15T10:00:00", Qt::ISODate)
                                                    << "fast_sync_with_a_local_removal_remote.vcf"
                                                    << 4
                                                    << "fast_sync_with_a_local_removal_local.vcf"
                                                    << 3
                                                    << "fast_sync_with_a_local_removal_remote_result.vcf"
                                                    << "fast_sync_with_a_local_removal_local_result.vcf";

        QTest::newRow("fast sync with a remote removal")
                                                    << QDateTime::fromString("2015-06-15T08:00:00", Qt::ISODate)
                                                    << QDateTime::fromString("2015-06-15T08:10:00", Qt::ISODate)
                                                    << QDateTime::fromString("2015-06-15T10:00:00", Qt::ISODate)
                                                    << "fast_sync_with_a_remote_removal_remote.vcf"
                                                    << 3
                                                    << "fast_sync_with_a_remote_removal_local.vcf"
                                                    << 4
                                                    << "fast_sync_with_a_remote_removal_remote_result.vcf"
                                                    << "fast_sync_with_a_remote_removal_local_result.vcf";

        QTest::newRow("fast sync changed both sides")
                                                    << QDateTime::fromString("2015-06-15T08:00:00", Qt::ISODate)
                                                    << QDateTime::fromString("2015-06-15T08:10:00", Qt::ISODate)
                                                    << QDateTime::fromString("2015-06-15T10:00:00", Qt::ISODate)
                                                    << "fast_sync_changed_both_sides_remote.vcf"
                                                    << 3
                                                    << "fast_sync_changed_both_sides_local.vcf"
                                                    << 3
                                                    << "fast_sync_changed_both_sides_remote_result.vcf"
                                                    << "fast_sync_changed_both_sides_local_result.vcf";

        QTest::newRow("fast sync delete same contact")
                                                    << QDateTime::fromString("2015-06-15T08:00:00", Qt::ISODate)
                                                    << QDateTime::fromString("2015-06-15T08:10:00", Qt::ISODate)
                                                    << QDateTime::fromString("2015-06-15T10:00:00", Qt::ISODate)
                                                    << "fast_sync_delete_same_contact_remote.vcf"
                                                    << 3
                                                    << "fast_sync_delete_same_contact_local.vcf"
                                                    << 3
                                                    << "fast_sync_delete_same_contact_remote_result.vcf"
                                                    << "fast_sync_delete_same_contact_local_result.vcf";

        QTest::newRow("fast sync with a new local contact")
                                                    << QDateTime()
                                                    << QDateTime::fromString("2015-06-15T08:10:00", Qt::ISODate)
                                                    << QDateTime::fromString("2015-06-15T10:00:00", Qt::ISODate)
                                                    << "fast_sync_with_new_local_contact_remote.vcf"
                                                    << 1
                                                    << "fast_sync_with_new_local_contact_local.vcf"
                                                    << 2
                                                    << "fast_sync_with_new_local_contact_remote_result.vcf"
                                                    << "fast_sync_with_new_local_contact_local_result.vcf";
    }

    void testSync()
    {
        QFETCH(QDateTime, createdAt);
        QFETCH(QDateTime, lastSyncTime);
        QFETCH(QDateTime, currentDateTime);
        QFETCH(QString, remoteFile);
        QFETCH(int, remoteInitalCount);
        QFETCH(QString, localFile);
        QFETCH(int, localInitalCount);
        QFETCH(QString, finalRemoteFile);
        QFETCH(QString, finalLocalFile);

        m_client->m_lastSyncTime = lastSyncTime;
        m_client->init();

        // prepare remote source
        m_client->m_remoteSource->manager()->setProperty("CURRENT_DATE_TIME", currentDateTime);
        importContactsFromVCardFile(m_client->m_remoteSource->manager(),
                                    TEST_DATA_DIR + remoteFile, createdAt);
        QTRY_COMPARE(m_client->m_remoteSource->count(), remoteInitalCount);

        // prepare local source
        importContactsFromVCardFile(m_client->m_localSource->manager(),
                                    TEST_DATA_DIR + localFile, createdAt);
        QTRY_COMPARE(m_client->m_localSource->getAllContactIds().count(), localInitalCount);

        QSignalSpy syncFinishedSpy(m_client, SIGNAL(syncFinished(Sync::SyncStatus)));
        m_client->startSync();

        // remote source will be initialized after success
        QTRY_VERIFY(m_client->m_remoteSource->m_initialized);

        // wait for sync finish and check sync result
        QTRY_COMPARE(syncFinishedSpy.count(), 1);
        QList<QVariant> arguments = syncFinishedSpy.takeFirst();
        QCOMPARE(arguments.at(0).toInt(), int(Sync::SYNC_DONE));

        // compare saved contacts with expected files
        QList<QContact> local = m_client->m_localSource->manager()->contacts();
        QList<QContact> remote = m_client->m_remoteSource->manager()->contacts();

        QVERIFY(local.size() > 0);
        QVERIFY(remote.size() > 0);

        QVERIFY(compareLines(toVCard(local), TEST_DATA_DIR + finalLocalFile));
        QVERIFY(compareLines(toVCard(remote), TEST_DATA_DIR + finalRemoteFile));
        QVERIFY(m_client->cleanUp());
    }

    void testFetchWithPagination_data()
    {
        QTest::addColumn<int>("pageSize");
        QTest::addColumn<int>("contactFetchedCount");

        QTest::newRow("page bigger than contact list")  << 30
                                                        << 1;

        QTest::newRow("page small than contact list")   << 10
                                                        << 2;

        QTest::newRow("page division exactly")          << 5
                                                        << 3;
    }

    void testFetchWithPagination()
    {
        QFETCH(int, pageSize);
        QFETCH(int, contactFetchedCount);

        m_client->init();

        // prepare remote source
        m_client->m_remoteSource->setPageSize(pageSize);
        importContactsFromVCardFile(m_client->m_remoteSource->manager(),
                                    TEST_DATA_DIR + QStringLiteral("slow_sync_with_pages_remote.vcf"),
                                    QDateTime::currentDateTime());
        QTRY_COMPARE(m_client->m_remoteSource->count(), 15);
        QCOMPARE(m_client->m_localSource->getAllContactIds().count(), 0);


        QSignalSpy contactFetched(m_client->m_remoteSource.data(),
                                  SIGNAL(contactsFetched(QList<QtContacts::QContact>,Sync::SyncStatus)));
        QSignalSpy syncFinishedSpy(m_client, SIGNAL(syncFinished(Sync::SyncStatus)));

        m_client->startSync();

        // we will receive one contactFetched for each page
        QTRY_COMPARE(contactFetched.count(), contactFetchedCount);

        // wait for sync finished signal
        QTRY_COMPARE(syncFinishedSpy.count(), 1);

        // check if all contacts was stored in local database
        QCOMPARE(m_client->m_localSource->getAllContactIds().count(), 15);
    }

    void testSlowSyncWithAnEmptyLocalDatabase()
    {
        m_client->init();

        // populate remote database
        importContactsFromVCardFile(m_client->m_remoteSource->manager(),
                                    TEST_DATA_DIR + QStringLiteral("new_database_local.vcf"),
                                    QDateTime::currentDateTime());
        QTRY_COMPARE(m_client->m_remoteSource->count(), 1);
        QCOMPARE(m_client->m_localSource->getAllContactIds().count(), 0);

        QSignalSpy remoteChangedSignal(m_client->m_remoteSource.data(),
                                       SIGNAL(contactsChanged(QList<QtContacts::QContact>,Sync::SyncStatus)));
        QSignalSpy remoteRemovedSignal(m_client->m_remoteSource.data(),
                                       SIGNAL(contactsRemoved(QStringList,Sync::SyncStatus)));
        QSignalSpy remoteCreatedSignal(m_client->m_remoteSource.data(),
                                       SIGNAL(contactsCreated(QList<QtContacts::QContact>,Sync::SyncStatus)));
        QSignalSpy remoteTransactionSignal(m_client->m_remoteSource.data(),
                                       SIGNAL(transactionCommited(QList<QtContacts::QContact>,
                                                                  QList<QtContacts::QContact>,
                                                                  QStringList,
                                                                  QMap<QString, int>,
                                                                  Sync::SyncStatus)));
        QSignalSpy syncFinishedSpy(m_client, SIGNAL(syncFinished(Sync::SyncStatus)));

        m_client->startSync();

        // wait for sync finished signal
        QTRY_COMPARE(syncFinishedSpy.count(), 1);

        // check if no change was made on remote side
        QCOMPARE(remoteChangedSignal.count(), 0);
        QCOMPARE(remoteRemovedSignal.count(), 0);
        QCOMPARE(remoteCreatedSignal.count(), 0);

        // transaction commit will be fired once with empty list
        QCOMPARE(remoteTransactionSignal.count(), 1);
        QList<QVariant> arguments = remoteTransactionSignal.takeFirst();
        QList<QtContacts::QContact> createdContacts = arguments.at(0).value<QList<QtContacts::QContact> >();
        QList<QtContacts::QContact> changedContacts = arguments.at(1).value<QList<QtContacts::QContact> >();
        QStringList removedContacts = arguments.at(2).value<QStringList>();
        QMap<QString, int> errorMap = arguments.at(3).value<QMap<QString, int> >();
        QVERIFY(createdContacts.isEmpty());
        QVERIFY(changedContacts.isEmpty());
        QVERIFY(removedContacts.isEmpty());
        QVERIFY(errorMap.isEmpty());
        QVERIFY(m_client->cleanUp());
    }
};

QTEST_MAIN(ContactSyncTest)

#include "TestContactsMain.moc"
