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

#include "ContactsChangeNotifier.h"
#include "LogMacros.h"
#include <QList>

const QString DEFAULT_CONTACTS_MANAGER("galera");

ContactsChangeNotifier::ContactsChangeNotifier()
    : iDisabled(true)
{
    FUNCTION_CALL_TRACE;
    iManager = new QContactManager(DEFAULT_CONTACTS_MANAGER);
}

ContactsChangeNotifier::~ContactsChangeNotifier()
{
    disable();
    delete iManager;
}

void ContactsChangeNotifier::enable()
{
    if(iManager && iDisabled) {
        connect(iManager, &QContactManager::contactsAdded,
                this, &ContactsChangeNotifier::onContactsAdded);

        connect(iManager, &QContactManager::contactsRemoved,
                this, &ContactsChangeNotifier::onContactsRemoved);

        connect(iManager, &QContactManager::contactsChanged,
                this, &ContactsChangeNotifier::onContactsChanged);
        iDisabled = false;
    }
}

void ContactsChangeNotifier::onContactsAdded(const QList<QContactId>& ids)
{
    FUNCTION_CALL_TRACE;
    if(ids.count()) {
        foreach(const QContactId &id, ids) {
            LOG_DEBUG("Added contact with id" << id);
        }
        emit change();
    }
}

void ContactsChangeNotifier::onContactsRemoved(const QList<QContactId>& ids)
{
    FUNCTION_CALL_TRACE;
    if(ids.count()) {
        foreach(const QContactId &id, ids) {
            LOG_DEBUG("Removed contact with id" << id);
        }
        emit change();
    }
}

void ContactsChangeNotifier::onContactsChanged(const QList<QContactId>& ids)
{
    FUNCTION_CALL_TRACE;
    if(ids.count()) {
        foreach(const QContactId &id, ids) {
            LOG_DEBUG("Changed contact with id" << id);
        }
        emit change();
    }
}

void ContactsChangeNotifier::disable()
{
    FUNCTION_CALL_TRACE;
    iDisabled = true;
    QObject::disconnect(iManager, 0, this, 0);
}
