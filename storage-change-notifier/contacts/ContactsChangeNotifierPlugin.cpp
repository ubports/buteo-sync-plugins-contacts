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

#include "ContactsChangeNotifierPlugin.h"
#include "ContactsChangeNotifier.h"
#include "LogMacros.h"
#include <QTimer>

using namespace Buteo;

extern "C" StorageChangeNotifierPlugin* createPlugin(const QString& aStorageName)
{
    return new ContactsChangeNotifierPlugin(aStorageName);
}

extern "C" void destroyPlugin(StorageChangeNotifierPlugin* plugin)
{
    delete plugin;
}

ContactsChangeNotifierPlugin::ContactsChangeNotifierPlugin(const QString& aStorageName)
    : StorageChangeNotifierPlugin(aStorageName),
      ihasChanges(false),
      iDisableLater(false)
{
    FUNCTION_CALL_TRACE;
    icontactsChangeNotifier = new ContactsChangeNotifier;
    connect(icontactsChangeNotifier, SIGNAL(change()),
                                     SLOT(onChange()));
}

ContactsChangeNotifierPlugin::~ContactsChangeNotifierPlugin()
{
    FUNCTION_CALL_TRACE;
    delete icontactsChangeNotifier;
}

QString ContactsChangeNotifierPlugin::name() const
{
    FUNCTION_CALL_TRACE;
    return iStorageName;
}

bool ContactsChangeNotifierPlugin::hasChanges() const
{
    FUNCTION_CALL_TRACE;
    return ihasChanges;
}

void ContactsChangeNotifierPlugin::changesReceived()
{
    FUNCTION_CALL_TRACE;
    ihasChanges = false;
}

void ContactsChangeNotifierPlugin::onChange()
{
    FUNCTION_CALL_TRACE;
    ihasChanges = true;
    if(iDisableLater) {
        icontactsChangeNotifier->disable();
    } else {
        emit storageChange();
    }
}

void ContactsChangeNotifierPlugin::enable()
{
    FUNCTION_CALL_TRACE;
    icontactsChangeNotifier->enable();
    iDisableLater = false;
}

void ContactsChangeNotifierPlugin::disable(bool disableAfterNextChange)
{
    FUNCTION_CALL_TRACE;
    if(disableAfterNextChange) {
        iDisableLater = true;
    } else {
        icontactsChangeNotifier->disable();
    }
}
