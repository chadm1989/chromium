/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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
#include "IDBTransactionBackendProxy.h"

#if ENABLE(INDEXED_DATABASE)

#include "IDBObjectStoreBackendProxy.h"
#include "IDBTransactionCallbacks.h"
#include "WebIDBDatabaseError.h"
#include "WebIDBObjectStore.h"
#include "WebIDBTransaction.h"
#include "WebIDBTransactionCallbacksImpl.h"

using namespace WebCore;

namespace WebKit {

PassRefPtr<IDBTransactionBackendInterface> IDBTransactionBackendProxy::create(PassOwnPtr<WebIDBTransaction> transaction)
{
    return adoptRef(new IDBTransactionBackendProxy(transaction));
}

IDBTransactionBackendProxy::IDBTransactionBackendProxy(PassOwnPtr<WebIDBTransaction> transaction)
    : m_webIDBTransaction(transaction)
{
    ASSERT(m_webIDBTransaction);
}

IDBTransactionBackendProxy::~IDBTransactionBackendProxy()
{
}

PassRefPtr<IDBObjectStoreBackendInterface> IDBTransactionBackendProxy::objectStore(const String& name, ExceptionCode& ec)
{
    OwnPtr<WebIDBObjectStore> objectStore = adoptPtr(m_webIDBTransaction->objectStore(name, ec));
    if (!objectStore)
        return 0;
    return IDBObjectStoreBackendProxy::create(objectStore.release());
}

unsigned short IDBTransactionBackendProxy::mode() const
{
    return m_webIDBTransaction->mode();
}

void IDBTransactionBackendProxy::abort()
{
    m_webIDBTransaction->abort();
}

bool IDBTransactionBackendProxy::scheduleTask(PassOwnPtr<ScriptExecutionContext::Task>, PassOwnPtr<ScriptExecutionContext::Task>)
{
    // This should never be reached as it's the impl objects who get to
    // execute tasks in the browser process.
    ASSERT_NOT_REACHED();
    return false;
}

void IDBTransactionBackendProxy::didCompleteTaskEvents()
{
    m_webIDBTransaction->didCompleteTaskEvents();
}

void IDBTransactionBackendProxy::setCallbacks(IDBTransactionCallbacks* callbacks)
{
    m_webIDBTransaction->setCallbacks(new WebIDBTransactionCallbacksImpl(callbacks));
}

} // namespace WebKit

#endif // ENABLE(INDEXED_DATABASE)
