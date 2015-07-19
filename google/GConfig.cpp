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

#include "GConfig.h"

const int GConfig::MAX_RESULTS = 30;
const QString GConfig::SCOPE_URL = "https://www.google.com/m8/feeds/";
const QString GConfig::GCONTACT_URL = SCOPE_URL + "/contacts/default/";

const QString GConfig::GDATA_VERSION_TAG = "GData-Version";
const QString GConfig::GDATA_VERSION = "3.0";
const QString GConfig::G_DELETE_OVERRIDE_HEADER = "X-HTTP-Method-Override: DELETE";
const QString GConfig::G_ETAG_HEADER = "If-Match";
const QString GConfig::G_AUTH_HEADER = "Authorization";

/* Query parameters */
const QString GConfig::QUERY_TAG = "q";
const QString GConfig::MAX_RESULTS_TAG = "max-results";
const QString GConfig::START_INDEX_TAG = "start-index";
const QString GConfig::UPDATED_MIN_TAG = "updated-min";
const QString GConfig::ORDERBY_TAG = "orderby";
const QString GConfig::SHOW_DELETED_TAG = "showdeleted";
const QString GConfig::REQUIRE_ALL_DELETED = "requirealldeleted";
const QString GConfig::SORTORDER_TAG = "sortorder";

const QString GConfig::PHOTO_TAG = "photos";
const QString GConfig::MEDIA_TAG = "media";
const QString GConfig::BATCH_TAG = "batch";
