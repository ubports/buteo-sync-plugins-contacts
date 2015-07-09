/****************************************************************************
 **
 ** Copyright (C) 2015 Canonical Ltd.
 **
 ** Contact: Renato Araujo Oliveira Filho <renato.filho@canonical.com>
 **
 ** This program/library is free software; you can redistribute it and/or
 ** modify it under the terms of the GNU Lesser General Public License
 ** version 2.1 as published by the Free Software Foundation.
 **
 ** This program/library is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 ** Lesser General Public License for more details.
 **
 ** You should have received a copy of the GNU Lesser General Public
 ** License along with this program/library; if not, write to the Free
 ** Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 ** 02110-1301 USA
 **
 ****************************************************************************/

#include "GContactImageUploader.h"
#include "GContactStream.h"
#include "GContactAtom.h"
#include "UContactsCustomDetail.h"

#include <LogMacros.h>

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QScopedPointer>
#include <QDomDocument>

#define GOOGLE_URL          "https://www.google.com/m8/feeds/contacts/default/full"

GContactImageUploader::GContactImageUploader(const QString &authToken,
                                             const QString &accountId,
                                             QObject *parent)
    : QObject(parent),
      mEventLoop(0),
      mAuthToken(authToken),
      mAccountId(accountId),
      mAbort(false)
{
}

void GContactImageUploader::push(const QString &remoteId, const QUrl &imgUrl)
{
    mQueue.push_back(qMakePair(remoteId, imgUrl));
}

QMap<QString, GContactImageUploader::UploaderReply> GContactImageUploader::result()
{
    return mResults;
}

/*
 * This is a sync function since the contact sync process happen in a different
 * thread we do not need this function to be async
 */
void GContactImageUploader::exec()
{
    if (mQueue.isEmpty()) {
        return;
    }

    QDateTime startTime = QDateTime::currentDateTime().toUTC();
    QScopedPointer<QNetworkAccessManager> networkAccessManager(new QNetworkAccessManager);
    connect(networkAccessManager.data(),
            SIGNAL(finished(QNetworkReply*)),
            SLOT(onRequestFinished(QNetworkReply*)),
            Qt::QueuedConnection);

    QEventLoop eventLoop;
    mEventLoop = &eventLoop;
    mUploadCompleted = false;

    while(!mQueue.isEmpty()) {
        QPair<QString, QUrl> data = mQueue.takeFirst();

        QByteArray imgData;
        QFile imgFile(data.second.toLocalFile());
        if (!imgFile.open(QIODevice::ReadOnly)) {
            LOG_WARNING("Fail to open file:" << data.second.toLocalFile());
            continue;
        }
        imgData = imgFile.readAll();
        imgFile.close();;
        mCurrentRemoteId = data.first;

        // upload photo
        QString requestUrl = QString("https://www.google.com/m8/feeds/photos/media/%1/%2")
                .arg(mAccountId)
                .arg(mCurrentRemoteId);
        QNetworkRequest request(requestUrl);
        request.setRawHeader("GData-Version", "3.0");
        request.setRawHeader(QString(QLatin1String("Authorization")).toUtf8(),
                             QString(QLatin1String("Bearer ") + mAuthToken).toUtf8());
        request.setRawHeader("Content-Type", "image/*");
        request.setRawHeader("If-Match", "*");
        networkAccessManager->put(request, imgData);

        // wait for the upload to finish
        eventLoop.exec();

        // should we abort?
        if (mAbort) {
            break;
        }
    }

    mUploadCompleted = true;

    if (mAbort) {
        return;
    }

    // After upload the pictures the contact etag get updated we need to retrieve
    // the new ones
    QString requestUrl =
            QString("%1?updated-min=%2&fields=entry(@gd:etag, id, link(@rel, @gd:etag))")
            .arg(GOOGLE_URL)
            .arg(startTime.toString(Qt::ISODate));
    QNetworkRequest request(requestUrl);
    request.setRawHeader("GData-Version", "3.0");
    request.setRawHeader(QString(QLatin1String("Authorization")).toUtf8(),
                         QString(QLatin1String("Bearer ") + mAuthToken).toUtf8());
    networkAccessManager->get(request);

    // wait for the reply to finish
    eventLoop.exec();
}

void GContactImageUploader::onRequestFinished(QNetworkReply *reply)
{
    if (mUploadCompleted) {
        if (reply->error() != QNetworkReply::NoError) {
            LOG_WARNING("Fail to retrieve new etags:" << reply->errorString());
        } else {
            QByteArray data = reply->readAll();
            QMap<QString, GContactImageUploader::UploaderReply> entries = parseEntryList(data);
            foreach(const QString &remoteId, entries.keys()) {
                if (mResults.contains(remoteId)) {
                    mResults[remoteId] = entries[remoteId];
                    emit uploadFinished(remoteId, entries[remoteId]);
                }
            }
        }
    } else {
        mResults.insert(mCurrentRemoteId, UploaderReply());
        if (reply->error() != QNetworkReply::NoError) {
            LOG_WARNING("Fail to upload avatar:" << reply->errorString());
            emit uploadError(mCurrentRemoteId, reply->errorString());
        } else {
            LOG_DEBUG("Avatar upload result" << reply->readAll());
        }
        mCurrentRemoteId.clear();
    }

    if (mEventLoop) {
        mEventLoop->quit();
    }
}

QMap<QString, GContactImageUploader::UploaderReply> GContactImageUploader::parseEntryList(const QByteArray &data) const
{
    QMap<QString, UploaderReply> result;

    QDomDocument doc;
    QString errorMsg;
    if (!doc.setContent(data, &errorMsg)) {
        LOG_WARNING("Fail to parse etag list" << errorMsg);
        return result;
    }

    QDomNodeList entries = doc.elementsByTagName("entry");
    for (int i = 0; i < entries.size(); i++) {
        UploaderReply reply;
        QDomElement entry = entries.item(i).toElement();
        QDomElement id = entry.firstChildElement("id");

        reply.newEtag = entry.attribute("gd:etag");
        if (!id.isNull()) {
            reply.remoteId = id.text().split('/').last();
        }

        QDomNodeList links = entry.elementsByTagName("link");
        for (int l = 0; l < links.size(); l++) {
            QDomElement link = links.item(l).toElement();

            if (link.attribute("rel") == "http://schemas.google.com/contacts/2008/rel#photo") {
                reply.newAvatarEtag = link.attribute("gd:etag");
                break;
            }
        }
        result.insert(reply.remoteId, reply);
    }

    return result;
}
