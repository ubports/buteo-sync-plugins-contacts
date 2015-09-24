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
#include <QUuid>

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

        // upload photo
        UploaderReply reply;
        reply.newAvatarEtag = QString("%1-avatar").arg(data.first);
        reply.newEtag = QString("%1-new").arg(data.first);
        reply.remoteId = data.first;
        mResults[data.first] = reply;
        emit uploadFinished(data.first, reply);
    }

    mUploadCompleted = true;

    if (mAbort) {
        return;
    }

}

void GContactImageUploader::onRequestFinished(QNetworkReply *reply)
{
}

QMap<QString, GContactImageUploader::UploaderReply> GContactImageUploader::parseEntryList(const QByteArray &data) const
{
    return QMap<QString, GContactImageUploader::UploaderReply>();
}
