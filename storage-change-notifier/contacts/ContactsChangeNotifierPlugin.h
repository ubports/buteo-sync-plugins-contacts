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

#ifndef CONTACTSCHANGENOTIFIERPLUGIN_H
#define CONTACTSCHANGENOTIFIERPLUGIN_H

#include "StorageChangeNotifierPlugin.h"

class ContactsChangeNotifier;

class ContactsChangeNotifierPlugin : public Buteo::StorageChangeNotifierPlugin
{
    Q_OBJECT

public:
    /*! \brief constructor
     * see StorageChangeNotifierPlugin
     */
    ContactsChangeNotifierPlugin(const QString& aStorageName);

    /*! \brief destructor
     */
    ~ContactsChangeNotifierPlugin();

    /*! \brief see StorageChangeNotifierPlugin::name
     */
    QString name() const;

    /*! \brief see StorageChangeNotifierPlugin::hasChanges
     */
    bool hasChanges() const;

    /*! \brief see StorageChangeNotifierPlugin::changesReceived
     */
    void changesReceived();

    /*! \brief see StorageChangeNotifierPlugin::enable
     */
    void enable();

    /*! \brief see StorageChangeNotifierPlugin::disable
     */
    void disable(bool disableAfterNextChange = false);

private Q_SLOTS:
    /*! \brief handles a change notification from contacts notifier
     */
    void onChange();

private:
    ContactsChangeNotifier* icontactsChangeNotifier;
    bool ihasChanges;
    bool iDisableLater;
};

#endif
