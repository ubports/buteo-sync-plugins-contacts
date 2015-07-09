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

#include "GContactImageDownloader.h"
#include "GTransport.h"

#include <LogMacros.h>

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QTemporaryFile>

GContactImageDownloader::GContactImageDownloader(const QString &authToken, QObject *parent)
    : QObject(parent),
      mEventLoop(0),
      mAuthToken(authToken),
      mAbort(false)
{
}

GContactImageDownloader::~GContactImageDownloader()
{
    foreach(const QString &file, mTempFiles) {
        QFile::remove(file);
    }
}

void GContactImageDownloader::push(const QUrl &imgUrl)
{
    mQueue.push_back(imgUrl);
}

QMap<QUrl, QUrl> GContactImageDownloader::donwloaded()
{
    return mResults;
}

void GContactImageDownloader::exec()
{
    QNetworkAccessManager *networkAccessManager = new QNetworkAccessManager;
    connect(networkAccessManager,
            SIGNAL(finished(QNetworkReply*)),
            SLOT(onRequestFinished(QNetworkReply*)),
            Qt::QueuedConnection);

    QEventLoop eventLoop;
    mEventLoop = &eventLoop;

    while(!mQueue.isEmpty()) {
        QNetworkRequest request(mQueue.takeFirst());
        request.setRawHeader("GData-Version", "3.0");
        request.setRawHeader(QString(QLatin1String("Authorization")).toUtf8(),
                             QString(QLatin1String("Bearer ") + mAuthToken).toUtf8());
        QNetworkReply *reply = networkAccessManager->get(request);

        // wait for the download to finish
        eventLoop.exec();

        // should we abort?
        if (mAbort) {
            break;
        }
    }
    delete networkAccessManager;
}

void GContactImageDownloader::onRequestFinished(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        LOG_WARNING("Fail to download avatar:" << reply->errorString());
        emit donwloadError(reply->url(), reply->errorString());
    } else {
        QUrl localFile(saveImage(reply->url(), reply->readAll()));
        mResults.insert(reply->url(), localFile);
        emit downloadFinished(reply->url(), localFile);
    }

    if (mEventLoop) {
        mEventLoop->quit();
    }
}

QUrl GContactImageDownloader::saveImage(const QUrl &remoteFile, const QByteArray &imgData)
{
    Q_UNUSED(remoteFile);

    QTemporaryFile tmp;
    if (tmp.open()) {
        tmp.write(imgData);
        tmp.setAutoRemove(false);
        tmp.close();
        mTempFiles << tmp.fileName();
        return QUrl::fromLocalFile(tmp.fileName());
    }
    return QUrl();
}

