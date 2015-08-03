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

#include "GTransport.h"

#include <QBuffer>
#include <QDebug>
#include <QNetworkProxy>
#include <QDateTime>
#include <QUrlQuery>

#include <LogMacros.h>

const int MAX_RESULTS = 10;
const QString SCOPE_URL("https://www.google.com/m8/feeds/");
const QString GCONTACT_URL(SCOPE_URL + "/contacts/default/");

const QString GDATA_VERSION_TAG = "GData-Version";
const QString GDATA_VERSION = "3.1";
const QString G_DELETE_OVERRIDE_HEADER("X-HTTP-Method-Override: DELETE");
const QString G_ETAG_HEADER("If-Match ");
const QString G_AUTH_HEADER ("Authorization");
const QString G_CONTENT_TYPE_HEADER = "application/atom+xml";

/* Query parameters */
const QString QUERY_TAG("q");
const QString MAX_RESULTS_TAG("max-results");
const QString START_INDEX_TAG("start-index");
const QString UPDATED_MIN_TAG("updated-min");
const QString ORDERBY_TAG("orderby");
const QString SHOW_DELETED_TAG("showdeleted");
const QString REQUIRE_ALL_DELETED("requirealldeleted");
const QString SORTORDER_TAG("sortorder");

const QString PHOTO_TAG("photos");
const QString MEDIA_TAG("media");
const QString BATCH_TAG("batch");

class GTransportPrivate
{
public:
    GTransportPrivate(QObject *parent)
    {
    }
    QList<QPair<QByteArray, QByteArray> > mHeaders;
};

GTransport::GTransport(QObject *parent)
    : QObject(parent),
      d_ptr(new GTransportPrivate(this))
{
    FUNCTION_CALL_TRACE;
    Q_D(GTransport);
}

GTransport::GTransport(QUrl url, QList<QPair<QByteArray, QByteArray> > headers)
    : d_ptr(new GTransportPrivate(this))
{
    FUNCTION_CALL_TRACE;
    Q_D(GTransport);
}

GTransport::GTransport(QUrl url, QList<QPair<QByteArray, QByteArray> > headers, QByteArray data)
    : d_ptr(new GTransportPrivate(this))
{
    FUNCTION_CALL_TRACE;
    Q_D(GTransport);
}

GTransport::~GTransport()
{
    FUNCTION_CALL_TRACE;
    Q_D(GTransport);
}

void
GTransport::setUrl(const QString &url)
{
    FUNCTION_CALL_TRACE;
    Q_D(GTransport);
    setProperty("URL", url);
}

void
GTransport::setData(QByteArray data)
{
    FUNCTION_CALL_TRACE;
    Q_D(GTransport);
    setProperty("DATA", data);
}

void GTransport::setHeaders()
{
    FUNCTION_CALL_TRACE;
    Q_D(GTransport);
}

void
GTransport::addHeader(const QByteArray first, const QByteArray second)
{
    FUNCTION_CALL_TRACE;
    Q_D(GTransport);
    QVariantMap headers = property("Headers").value<QVariantMap>();
    headers.insert(first, second);
    setProperty("Headers", headers);
}

void
GTransport::setAuthToken(const QString token)
{
    FUNCTION_CALL_TRACE;
    Q_D(GTransport);
}

void
GTransport::setGDataVersionHeader()
{
    Q_D(GTransport);
    QVariantMap headers = property("Headers").value<QVariantMap>();
    headers.insert(QStringLiteral("GData-Version"), QStringLiteral("3.0"));
    setProperty("Headers", headers);
}

void
GTransport::setProxy (QString proxyHost, QString proxyPort)
{
    FUNCTION_CALL_TRACE;
    Q_D(GTransport);
    setProperty("ProxyHost", proxyHost);
    setProperty("ProxyPort", proxyPort);
}

void
GTransport::request(const HTTP_REQUEST_TYPE type)
{
    FUNCTION_CALL_TRACE;
    Q_D(GTransport);
    QUrl url(property("URL").toString());
    QByteArray data;
    emit requested(url, &data);
    setProperty("RequestType", (int) type);
    setProperty("ReplyBody", data);
    emit finishedRequest();
}

bool
GTransport::hasReply() const
{
    FUNCTION_CALL_TRACE;
    return true;
}

const
QByteArray GTransport::replyBody() const
{
    FUNCTION_CALL_TRACE;
    return property("ReplyBody").toByteArray();
}

void
GTransport::readyRead()
{
    FUNCTION_CALL_TRACE;
    Q_D(GTransport);
}

void
GTransport::finishedSlot(QNetworkReply *reply)
{
    FUNCTION_CALL_TRACE;
    Q_D(GTransport);

    emit finishedRequest();
}

void
GTransport::setUpdatedMin(const QDateTime datetime)
{
    FUNCTION_CALL_TRACE;
    Q_D(GTransport);
    setProperty("UpdatedMin", datetime);
}

void
GTransport::setMaxResults(unsigned int limit)
{
    FUNCTION_CALL_TRACE;
    Q_D(GTransport);
    setProperty("MaxResults", limit);
}

void
GTransport::setShowDeleted()
{
    FUNCTION_CALL_TRACE;
    Q_D(GTransport);
    setProperty("ShowDeleted", true);
}

bool GTransport::showDeleted() const
{
    const Q_D(GTransport);
    return property("ShowDeleted").toBool();
}

void
GTransport::setStartIndex(const int index)
{
    FUNCTION_CALL_TRACE;
    Q_D(GTransport);
    setProperty("StartIndex", index);
}

GTransport::HTTP_REQUEST_TYPE
GTransport::requestType()
{
    return GTransport::HTTP_REQUEST_TYPE(property("RequestType").toInt());
}

void
GTransport::reset()
{
    Q_D(GTransport);
}

void GTransport::setGroupFilter(const QString &account, const QString &groupId)
{
    setProperty("GroupFilter", QString("%1@%2").arg(account).arg(groupId));
}
