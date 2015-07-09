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

#ifndef GOOGLECONTACTIMAGEUPLOADER_H
#define GOOGLECONTACTIMAGEUPLOADER_H

#include <QObject>
#include <QString>
#include <QUrl>
#include <QQueue>
#include <QMap>
#include <QMutex>
#include <QEventLoop>
#include <QNetworkReply>



class GContactImageUploader: public QObject
{
    Q_OBJECT

public:
    class UploaderReply
    {
    public:
        QString remoteId;
        QString newEtag;
        QString newAvatarEtag;
    };

    explicit GContactImageUploader(const QString &authToken,
                                   const QString &accountId,
                                   QObject *parent = 0);

    void push(const QString &remoteId, const QUrl &imgUrl);
    QMap<QString, UploaderReply> result();
    void exec();

signals:
    void uploadFinished(const QUrl &remoteId, const GContactImageUploader::UploaderReply &eTag);
    void uploadError(const QUrl &remoteId, const QString &error);

private slots:
    void onRequestFinished(QNetworkReply *reply);

private:
    QEventLoop *mEventLoop;
    QQueue<QPair<QString, QUrl> > mQueue;
    QString mAuthToken;
    QString mAccountId;
    QString mCurrentRemoteId;
    QMap<QString, UploaderReply> mResults;
    bool mAbort;
    bool mUploadCompleted;

     QMap<QString, UploaderReply> parseEntryList(const QByteArray &data) const;
};

#endif // GOOGLECONTACTIMAGEUPLOADER_H
