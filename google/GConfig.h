/*
 * This file is part of buteo-gcontact-plugins package
 *
 * Copyright (C) 2013 Jolla Ltd. and/or its subsidiary(-ies).
 *               2015 Canonical Ltd
 *
 * Contributors: Sateesh Kavuri <sateesh.kavuri@gmail.com>
 *               Mani Chandrasekar <maninc@gmail.com>
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

#ifndef GCONFIG_H
#define GCONFIG_H

#include <QString>

class GConfig
{
public:
    static const int MAX_RESULTS;
    static const QString SCOPE_URL;
    static const QString GCONTACT_URL;

    static const QString GDATA_VERSION_TAG;
    static const QString GDATA_VERSION;
    static const QString G_DELETE_OVERRIDE_HEADER;
    static const QString G_ETAG_HEADER;
    static const QString G_AUTH_HEADER;

    /* Query parameters */
    static const QString QUERY_TAG;
    static const QString MAX_RESULTS_TAG;
    static const QString START_INDEX_TAG;
    static const QString UPDATED_MIN_TAG;
    static const QString ORDERBY_TAG;
    static const QString SHOW_DELETED_TAG;
    static const QString REQUIRE_ALL_DELETED;
    static const QString SORTORDER_TAG;

    static const QString PHOTO_TAG;
    static const QString MEDIA_TAG;
    static const QString BATCH_TAG;

    typedef enum
    {
        NONE = 0,
        ADD,
        UPDATE,
        DELETE
    } TRANSACTION_TYPE;
};
#endif // GCONFIG_H

