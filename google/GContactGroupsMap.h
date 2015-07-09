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

#ifndef GOOGLECONTACTGRUOPMAP_H
#define GOOGLECONTACTGRUOPMAP_H

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

class GContactGroupMap: public QObject
{
    Q_OBJECT

public:
    explicit GContactGroupMap(const QString &authToken, QObject *parent = 0);
    ~GContactGroupMap();

    void reload();

signals:
    void updated();

private slots:
    void onRequestFinished(QNetworkReply *reply);

private:
    QScopedPointer<QNetworkAccessManager> mNetworkAccessManager;
    QString mAuthToken;
    QMap<QString, QString> mGroups;
};

#endif // GOOGLECONTACTGRUOPMAP_H
