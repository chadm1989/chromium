/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebIDBDatabaseImpl.h"

#include "DOMStringList.h"
#include "IDBCallbacksProxy.h"
#include "IDBDatabaseBackendInterface.h"
#include "IDBTransactionBackendInterface.h"
#include "WebIDBCallbacks.h"
#include "WebIDBObjectStoreImpl.h"
#include "WebIDBTransactionImpl.h"

#if ENABLE(INDEXED_DATABASE)

using namespace WebCore;

namespace WebKit {

WebIDBDatabaseImpl::WebIDBDatabaseImpl(PassRefPtr<IDBDatabaseBackendInterface> databaseBackend)
    : m_databaseBackend(databaseBackend)
{
}

WebIDBDatabaseImpl::~WebIDBDatabaseImpl()
{
}

WebString WebIDBDatabaseImpl::name() const
{
    return m_databaseBackend->name();
}

WebString WebIDBDatabaseImpl::description() const
{
    return m_databaseBackend->description();
}

WebString WebIDBDatabaseImpl::version() const
{
    return m_databaseBackend->version();
}

WebDOMStringList WebIDBDatabaseImpl::objectStores() const
{
    return m_databaseBackend->objectStores();
}

void WebIDBDatabaseImpl::createObjectStore(const WebString& name, const WebString& keyPath, bool autoIncrement, WebIDBCallbacks* callbacks)
{
    m_databaseBackend->createObjectStore(name, keyPath, autoIncrement, IDBCallbacksProxy::create(callbacks));
}

WebIDBObjectStore* WebIDBDatabaseImpl::objectStore(const WebString& name, unsigned short mode)
{
    RefPtr<IDBObjectStoreBackendInterface> objectStore = m_databaseBackend->objectStore(name, mode);
    if (!objectStore)
        return 0;
    return new WebIDBObjectStoreImpl(objectStore);
}

void WebIDBDatabaseImpl::removeObjectStore(const WebString& name, WebIDBCallbacks* callbacks)
{
    m_databaseBackend->removeObjectStore(name, IDBCallbacksProxy::create(callbacks));
}

void WebIDBDatabaseImpl::setVersion(const WebString& version, WebIDBCallbacks* callbacks)
{
    m_databaseBackend->setVersion(version, IDBCallbacksProxy::create(callbacks));
}

WebIDBTransaction* WebIDBDatabaseImpl::transaction(const WebDOMStringList& names, unsigned short mode, unsigned long timeout)
{
    RefPtr<DOMStringList> nameList = PassRefPtr<DOMStringList>(names);
    RefPtr<IDBTransactionBackendInterface> transaction = m_databaseBackend->transaction(nameList.get(), mode, timeout);
    if (!transaction)
        return 0;
    return new WebIDBTransactionImpl(transaction);
}

} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)
