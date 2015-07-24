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

#ifndef CONTACTSCHANGENOTIFIER_H
#define CONTACTSCHANGENOTIFIER_H

#include <QObject>
#include <QList>

#include <QContactManager>
#include <QContactId>

QTCONTACTS_USE_NAMESPACE

class ContactsChangeNotifier : public QObject
{
    Q_OBJECT

public:
    /*! \brief constructor
     */
    ContactsChangeNotifier();

    /*! \brief destructor
     */
    ~ContactsChangeNotifier();

    /*! \brief start listening to changes from QContactManager
     */
    void enable();

    /*! \brief stop listening to changes from QContactManager
     */
    void disable();

Q_SIGNALS:
    /*! emit this signal to notify a change in contacts backend
     */
    void change();

private Q_SLOTS:
    void onContactsAdded(const QList<QContactId>& ids);
    void onContactsRemoved(const QList<QContactId>& ids);
    void onContactsChanged(const QList<QContactId>& ids);

private:
    QContactManager* iManager;
    bool iDisabled;
};

#endif
