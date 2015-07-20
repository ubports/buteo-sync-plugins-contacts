/*
 * This file is part of buteo-sync-plugins-goole package
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

#ifndef UCONTACTSCUSTOMDETAIL_H
#define UCONTACTSCUSTOMDETAIL_H

#include <QString>

#include <QContact>
#include <QContactExtendedDetail>

QTCONTACTS_USE_NAMESPACE

class UContactsCustomDetail
{
public:
    // Ubuntu fields
    static const QString FieldContactETag;
    static const QString FieldRemoteId;
    static const QString FieldGroupMembershipInfo;
    static const QString FieldDeletedAt;
    static const QString FieldCreatedAt;
    static const QString FieldContactAvatarETag;

    static QContactExtendedDetail getCustomField(const QContact &contact, const QString &name);
    static void setCustomField(QContact &contact, const QString &name, const QVariant &value);
};

#endif // GCONTACTCUSTOMDETAIL_H
