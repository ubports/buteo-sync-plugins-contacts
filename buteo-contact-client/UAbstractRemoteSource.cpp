/*
 * This file is part of buteo-sync-plugins-goole package
 *
 * Copyright (C) 2015 Canonical Ltd
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

#include "UAbstractRemoteSource.h"
#include "UContactsBackend.h"

#include <QDebug>

QTCONTACTS_USE_NAMESPACE

class UContactsBackendBatchOperation
{
public:
    enum BatchOperation {
        BATCH_UPDATE = 0,
        BATCH_DELETE,
        BATCH_CREATE,
        BATCH_NONE,
    };

    UContactsBackendBatchOperation()
        : m_op(BATCH_NONE)
    {
    }

    UContactsBackendBatchOperation(BatchOperation op, const QContact &contact)
        : m_op(op),
          m_contact(contact)
    {
    }

    BatchOperation operation() const
    {
        return m_op;
    }

    QContact contact() const
    {
        return m_contact;
    }

    bool isValid() const
    {
        return (m_op != BATCH_NONE);
    }

private:
    BatchOperation m_op;
    QContact m_contact;
};

class UAbstractRemoteSourcePrivate
{
public:
    UAbstractRemoteSourcePrivate()
        : m_batchMode(false)
    {
    }

    bool m_batchMode;
    QList<UContactsBackendBatchOperation> m_operations;
};

UAbstractRemoteSource::UAbstractRemoteSource(QObject *parent)
    : QObject(parent),
      d_ptr(new UAbstractRemoteSourcePrivate)
{
}

UAbstractRemoteSource::~UAbstractRemoteSource()
{
}

void UAbstractRemoteSource::transaction()
{
    Q_D(UAbstractRemoteSource);

    d->m_batchMode = true;
}

bool UAbstractRemoteSource::commit()
{
    Q_D(UAbstractRemoteSource);

    if (d->m_operations.isEmpty()) {
        transactionCommited(QList<QContact>(),
                            QList<QContact>(),
                            QStringList(),
                            Sync::SYNC_DONE);
        return true;
    }

    QList<QContact> create;
    QList<QContact> update;
    QList<QContact> remove;

    foreach(const UContactsBackendBatchOperation &op, d->m_operations) {
        switch(op.operation()) {
        case UContactsBackendBatchOperation::BATCH_CREATE:
            create << op.contact();
            break;
        case UContactsBackendBatchOperation::BATCH_UPDATE:
            update << op.contact();
            break;
        case UContactsBackendBatchOperation::BATCH_DELETE:
            remove << op.contact();
            break;
        default:
            qWarning() << "Invalid operation";
        }
    }

    batch(create, update, remove);
    d->m_operations.clear();
    d->m_batchMode = false;
    return true;
}

bool UAbstractRemoteSource::rollback()
{
    Q_D(UAbstractRemoteSource);

    d->m_operations.clear();
    d->m_batchMode = false;
    return true;
}

void UAbstractRemoteSource::saveContacts(const QList<QContact> &contacts)
{
    Q_D(UAbstractRemoteSource);

    if (d->m_batchMode) {
        foreach(const QContact &contact, contacts) {
            UContactsBackendBatchOperation::BatchOperation op;
            QString remoteId = UContactsBackend::getRemoteId(contact);
            op = remoteId.isEmpty() ? UContactsBackendBatchOperation::BATCH_CREATE :
                                      UContactsBackendBatchOperation::BATCH_UPDATE;
            d->m_operations << UContactsBackendBatchOperation(op, contact);
        }
    } else {
        saveContactsNonBatch(contacts);
    }
}

void UAbstractRemoteSource::removeContacts(const QList<QContact> &contacts)
{
    Q_D(UAbstractRemoteSource);

    if (d->m_batchMode) {
        foreach(const QContact &contact, contacts) {
            d->m_operations << UContactsBackendBatchOperation(UContactsBackendBatchOperation::BATCH_DELETE,
                                                              contact);
        }
    } else {
        removeContactsNonBatch(contacts);
    }
}
