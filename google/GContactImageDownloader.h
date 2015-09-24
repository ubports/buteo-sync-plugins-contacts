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

#ifndef GOOGLECONTACTIMAGEDOWNLOADER_H
#define GOOGLECONTACTIMAGEDOWNLOADER_H

#include <QObject>
#include <QString>
#include <QUrl>
#include <QQueue>
#include <QThread>
#include <QMap>
#include <QMutex>
#include <QEventLoop>
#include <QtNetwork/QNetworkAccessManager>

class GTransport;

class GContactImageDownloader: public QObject
{
    Q_OBJECT

public:
    explicit GContactImageDownloader(const QString &authToken, QObject *parent = 0);
    ~GContactImageDownloader();

    void push(const QUrl &imgUrl);
    QMap<QUrl, QUrl> donwloaded();
    void exec();

signals:
    void downloadFinished(const QUrl &imgUrl, const QUrl &localFile);
    void donwloadError(const QUrl &imgUrl, const QString &error);

private slots:
    void onRequestFinished(QNetworkReply *reply);

private:
    QEventLoop *mEventLoop;
    QQueue<QUrl> mQueue;
    QString mAuthToken;
    QMap<QUrl, QUrl> mResults;
    bool mAbort;
    QStringList mTempFiles;

    QUrl saveImage(const QUrl &remoteFile, const QByteArray &imgData);
};

#endif // GOOGLECONTACTIMAGEDOWNLOADER_H
