/*
 * This file is part of buteo-gcontact-plugin package
 *
 * Copyright (C) 2013 Jolla Ltd. and/or its subsidiary(-ies).
 *               2015 Canonical Ltd.
 *
 * Contributors: Sateesh Kavuri <sateesh.kavuri@gmail.com>
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

#ifndef GTRANSPORT_H
#define GTRANSPORT_H

#include <QObject>
#include <QList>
#include <QDateTime>
#include <QPair>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

class GTransportPrivate;
class GTransport : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(GTransport)
public:

    typedef enum {
        GET,
        POST,
        DELETE,
        PUT,
        HEAD
    } HTTP_REQUEST_TYPE;

    explicit GTransport(QObject *parent = 0);
    GTransport (QUrl url, QList<QPair<QByteArray, QByteArray> > headers);
    GTransport (QUrl url, QList<QPair<QByteArray, QByteArray> > headers, QByteArray data);

    virtual ~GTransport();

    void setUrl(const QString &url);
    void setData(QByteArray data);
    void setHeaders();
    void addHeader(const QByteArray first, const QByteArray second);
    void setAuthToken(const QString token);
    void setGDataVersionHeader();
    void setProxy(QString proxyHost, QString proxyPort);
    void request(const HTTP_REQUEST_TYPE type);
    bool hasReply() const;
    const QByteArray replyBody() const;
    void setUpdatedMin(const QDateTime datetime);
    void setMaxResults(unsigned int limit);
    void setShowDeleted();
    bool showDeleted() const;
    void setStartIndex(const int index);
    HTTP_REQUEST_TYPE requestType();
    void reset();

    typedef enum
    {
        HTTP_OK = 200,
        HTTP_CONTACT_CREATED = 201,
        HTTP_PRECONDITION_FAILED = 412
    } RESPONSE_CODE;

    static const QString GDATA_VERSION_HEADER;

signals:
    void finishedRequest();
    void error(int errorCode);

    // used by tests
    void requested(const QUrl &url, QByteArray *result);

private slots:
    virtual void finishedSlot(QNetworkReply* reply);
    virtual void readyRead();

private:
    QScopedPointer<GTransportPrivate> d_ptr;

};

#endif // GTRANSPORT_H
