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

#include "UContactsCustomDetail.h"

// Ubuntu fields
const QString UContactsCustomDetail::FieldContactETag = "X-GOOGLE-ETAG";
const QString UContactsCustomDetail::FieldGroupMembershipInfo = "X-GROUP-ID";
const QString UContactsCustomDetail::FieldRemoteId = "X-REMOTE-ID";
const QString UContactsCustomDetail::FieldDeletedAt = "X-DELETED-AT";
const QString UContactsCustomDetail::FieldCreatedAt = "X-CREATED-AT";
const QString UContactsCustomDetail::FieldContactAvatarETag = "X-AVATAR-REV";

QContactExtendedDetail
UContactsCustomDetail::getCustomField(const QContact &contact, const QString &name)
{
    foreach (QContactExtendedDetail xd, contact.details<QContactExtendedDetail>()) {
        if (xd.name() == name) {
            return xd;
        }
    }
    QContactExtendedDetail xd;
    xd.setName(name);
    return xd;
}


void UContactsCustomDetail::setCustomField(QtContacts::QContact &contact, const QString &name, const QVariant &value)
{
    QContactExtendedDetail xd = getCustomField(contact, name);
    xd.setData(value);
    contact.saveDetail(&xd);
}
