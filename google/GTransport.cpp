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
        : mNetworkRequest(0),
          mNetworkReply(0),
          mNetworkMgr(new QNetworkAccessManager(parent))
    {
    }

    void
    construct(const QUrl& url)
    {
        mUrl = url;
        QUrlQuery urlQuery(mUrl);

        QList<QPair<QString, QString> > queryList = urlQuery.queryItems();
        for (int i=0; i < queryList.size(); i++)
        {
            QPair<QString, QString> pair = queryList.at(i);
            QByteArray leftEncode = QUrl::toPercentEncoding(pair.first);
            QByteArray rightEncode = QUrl::toPercentEncoding(pair.second);
            urlQuery.removeQueryItem(pair.first);
            urlQuery.addQueryItem(leftEncode, rightEncode);
        }
        mUrl.setQuery(urlQuery);
    }

    QNetworkRequest *mNetworkRequest;
    QNetworkReply *mNetworkReply;
    QScopedPointer<QNetworkAccessManager> mNetworkMgr;

    QUrl mUrl;
    QList<QPair<QByteArray, QByteArray> > mHeaders;

    QByteArray mPostData;
    QByteArray mNetworkReplyBody;
    QNetworkReply::NetworkError mNetworkError;
    int mResponseCode;
    QString mAuthToken;
    QDateTime mUpdatedMin;
    GTransport::HTTP_REQUEST_TYPE mRequestType;
};

GTransport::GTransport(QObject *parent)
    : QObject(parent),
      d_ptr(new GTransportPrivate(this))
{
    FUNCTION_CALL_TRACE;
    Q_D(GTransport);

    connect(d->mNetworkMgr.data(),
            SIGNAL(finished(QNetworkReply*)),
            SLOT(finishedSlot(QNetworkReply*)), Qt::QueuedConnection);
}

GTransport::GTransport (QUrl url, QList<QPair<QByteArray, QByteArray> > headers)
    : d_ptr(new GTransportPrivate(this))
{
    FUNCTION_CALL_TRACE;
    Q_D(GTransport);

    d->mHeaders = headers;
    d->construct(url);
    connect(d->mNetworkMgr.data(),
            SIGNAL(finished(QNetworkReply*)),
            SLOT(finishedSlot(QNetworkReply*)), Qt::QueuedConnection);

}

GTransport::GTransport (QUrl url, QList<QPair<QByteArray, QByteArray> > headers, QByteArray data)
    : d_ptr(new GTransportPrivate(this))
{
    FUNCTION_CALL_TRACE;
    Q_D(GTransport);

    d->mHeaders = headers;
    d->mPostData = data;
    d->construct(url);
    connect(d->mNetworkMgr.data(),
            SIGNAL(finished(QNetworkReply*)),
            SLOT(finishedSlot(QNetworkReply*)), Qt::QueuedConnection);
}

GTransport::~GTransport()
{
    FUNCTION_CALL_TRACE;
    Q_D(GTransport);

    delete d->mNetworkRequest;
    d->mNetworkRequest = 0;
    // TODO: Should I delete network reply??
    d->mNetworkReply = 0;
}

void
GTransport::setUrl(const QString &url)
{
    FUNCTION_CALL_TRACE;
    Q_D(GTransport);

    d->mUrl.setUrl(url, QUrl::StrictMode);
    d->construct(d->mUrl);
}

void
GTransport::setData(QByteArray data)
{
    FUNCTION_CALL_TRACE;
    Q_D(GTransport);

    //FIXME: this is really necessary??
    if (d->mPostData.isEmpty()) {
        d->mPostData.clear();
    }
    d->mPostData = data;
}

void GTransport::setHeaders()
{
    FUNCTION_CALL_TRACE;
    Q_D(GTransport);

    /*
     * A laughable bug in Google (bug#3397). If the "GDataVersion:3.0" tag is not
     * the first header in the list, then google does not consider
     * it as a 3.0 version, and just returns the default format
     * So, the headers order is very sensitive
     */
    for (int i=0; i < d->mHeaders.count (); i++)
    {
        QPair<QByteArray, QByteArray> headerPair = d->mHeaders[i];
        d->mNetworkRequest->setRawHeader(headerPair.first, headerPair.second);
    }
}

void
GTransport::addHeader(const QByteArray first, const QByteArray second)
{
    FUNCTION_CALL_TRACE;
    Q_D(GTransport);

    d->mHeaders.append(QPair<QByteArray, QByteArray> (first, second));
}

void
GTransport::setAuthToken(const QString token)
{
    FUNCTION_CALL_TRACE;
    Q_D(GTransport);

    d->mAuthToken = token;

    QByteArray header1 = QString(G_AUTH_HEADER).toUtf8();
    addHeader(header1, ("Bearer " + token).toUtf8());
}

void
GTransport::setGDataVersionHeader()
{
    Q_D(GTransport);
    d->mHeaders.append(QPair<QByteArray, QByteArray> (QByteArray ("GData-Version"), QByteArray ("3.0")));
}

void
GTransport::setProxy (QString proxyHost, QString proxyPort)
{
    FUNCTION_CALL_TRACE;
    Q_D(GTransport);

    QNetworkProxy proxy = d->mNetworkMgr->proxy();
    proxy.setType (QNetworkProxy::HttpProxy);
    proxy.setHostName (proxyHost);
    proxy.setPort (proxyPort.toInt ());

    d->mNetworkMgr->setProxy(proxy);
}



void
GTransport::request(const HTTP_REQUEST_TYPE type)
{
    FUNCTION_CALL_TRACE;
    Q_D(GTransport);

    LOG_DEBUG ("Request type:" << type);
    if (d->mNetworkRequest) {
        delete d->mNetworkRequest;
        d->mNetworkRequest = 0;
    }

    d->mNetworkReplyBody = "";
    d->mNetworkRequest = new QNetworkRequest();
    d->mNetworkRequest->setUrl(d->mUrl);
    setHeaders();

    d->mRequestType = type;
    LOG_DEBUG("++URL:" << d->mNetworkRequest->url().toString ());
    switch (type) {
    case GET:
        d->mNetworkReply = d->mNetworkMgr->get(*(d->mNetworkRequest));
        LOG_DEBUG ("--- FINISHED GET REQUEST ---");
        break;
    case POST:
        d->mNetworkRequest->setHeader (QNetworkRequest::ContentLengthHeader, d->mPostData.size ());
        d->mNetworkReply = d->mNetworkMgr->post(*(d->mNetworkRequest), d->mPostData);
        LOG_DEBUG ("--- FINISHED POST REQUEST ---");
        break;
    case PUT:
        d->mNetworkRequest->setHeader (QNetworkRequest::ContentLengthHeader, d->mPostData.size ());
        d->mNetworkReply = d->mNetworkMgr->put(*(d->mNetworkRequest), d->mPostData);
        LOG_DEBUG ("--- FINISHED PUT REQUEST ---");
        break;
    case DELETE:
        d->mNetworkReply = d->mNetworkMgr->deleteResource(*(d->mNetworkRequest));
        break;
    case HEAD:
        // Nothing
        break;
    default:
        // FIXME: signal the error
        break;
    }

    d->mNetworkRequest->setRawHeader(QStringLiteral("If-Match").toUtf8(),
                                     QStringLiteral("*").toUtf8());

    QList<QByteArray> headerList = d->mNetworkRequest->rawHeaderList();
    for (int i=0; i<headerList.size (); i++) {
        LOG_DEBUG ("Header " << i << ":" << headerList.at (i)
                                  << ":" << d->mNetworkRequest->rawHeader(headerList.at (i)));
    }
    connect(d->mNetworkReply, SIGNAL(readyRead()), SLOT(readyRead()));
}

bool
GTransport::hasReply() const
{
    FUNCTION_CALL_TRACE;
    const Q_D(GTransport);

    return (d->mNetworkReply);
}

const
QByteArray GTransport::replyBody() const
{
    FUNCTION_CALL_TRACE;
    const Q_D(GTransport);

    return d->mNetworkReplyBody;
}

void
GTransport::readyRead()
{
    FUNCTION_CALL_TRACE;
    Q_D(GTransport);

    d->mResponseCode = d->mNetworkReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    LOG_DEBUG ("++RESPONSE CODE:" << d->mResponseCode);
    QByteArray bytes = d->mNetworkReply->readAll();
    if (d->mResponseCode >= 200 && d->mResponseCode <= 300) {
        d->mNetworkReplyBody += bytes;
    } else {
        LOG_DEBUG ("SERVER ERROR:" << bytes);
        emit error(d->mResponseCode);
    }
}

void
GTransport::finishedSlot(QNetworkReply *reply)
{
    FUNCTION_CALL_TRACE;
    Q_D(GTransport);

//    QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
//    QVariant redirectionUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);

    d->mNetworkError = reply->error();

    if (d->mNetworkError != QNetworkReply::NoError) {
        emit error(d->mNetworkError);
    }

    emit finishedRequest();
}

void
GTransport::setUpdatedMin (const QDateTime datetime)
{
    FUNCTION_CALL_TRACE;
    Q_D(GTransport);

    d->mUpdatedMin = datetime;
    QUrlQuery urlQuery(d->mUrl);

    if (!urlQuery.hasQueryItem(UPDATED_MIN_TAG)) {
        urlQuery.addQueryItem(UPDATED_MIN_TAG,
                              d->mUpdatedMin.toString(Qt::ISODate));
        d->mUrl.setQuery(urlQuery);
    }
}

void
GTransport::setMaxResults (unsigned int limit)
{
    FUNCTION_CALL_TRACE;
    Q_D(GTransport);

    QUrlQuery urlQuery(d->mUrl);
    if (!urlQuery.hasQueryItem(MAX_RESULTS_TAG)) {
        urlQuery.addQueryItem(MAX_RESULTS_TAG, QString::number(limit));
        d->mUrl.setQuery(urlQuery);
    }
}

void
GTransport::setShowDeleted()
{
    FUNCTION_CALL_TRACE;
    Q_D(GTransport);

    QUrlQuery urlQuery(d->mUrl);
    if (!urlQuery.hasQueryItem(SHOW_DELETED_TAG)) {
        urlQuery.addQueryItem(SHOW_DELETED_TAG, "true");
        d->mUrl.setQuery(urlQuery);
    }
}

bool GTransport::showDeleted() const
{
    const Q_D(GTransport);

    QUrlQuery urlQuery(d->mUrl);
    return urlQuery.hasQueryItem(SHOW_DELETED_TAG);
}

void
GTransport::setStartIndex(const int index)
{
    FUNCTION_CALL_TRACE;
    Q_D(GTransport);

    QUrlQuery urlQuery(d->mUrl);
    if (urlQuery.hasQueryItem ("start-index")) {
        urlQuery.removeQueryItem ("start-index");
    }

    urlQuery.addQueryItem ("start-index", QString::number (index));
    d->mUrl.setQuery(urlQuery);
}

GTransport::HTTP_REQUEST_TYPE
GTransport::requestType ()
{
    Q_D(GTransport);

    return d->mRequestType;
}

void
GTransport::reset()
{
    Q_D(GTransport);

    d->mUrl.clear ();
    d->mHeaders.clear ();
    d->mPostData.clear ();
    d->mNetworkReplyBody.clear();
}
