/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ThreadableLoaderClientWrapper_h
#define ThreadableLoaderClientWrapper_h

#include "ThreadableLoaderClient.h"
#include <wtf/Noncopyable.h>
#include <wtf/PassRefPtr.h>
#include <wtf/Threading.h>

namespace WebCore {

    class ThreadableLoaderClientWrapper : public ThreadSafeShared<ThreadableLoaderClientWrapper> {
    public:
        static PassRefPtr<ThreadableLoaderClientWrapper> create(ThreadableLoaderClient* client)
        {
            return adoptRef(new ThreadableLoaderClientWrapper(client));
        }

        void clearClient()
        {
            m_client = 0;
        }

        void didSendData(unsigned long long bytesSent, unsigned long long totalBytesToBeSent)
        {
            if (m_client)
                m_client->didSendData(bytesSent, totalBytesToBeSent);
        }

        void didReceiveResponse(const ResourceResponse& response)
        {
            if (m_client)
                m_client->didReceiveResponse(response);
        }

        void didReceiveData(const char* data, int lengthReceived)
        {
            if (m_client)
                m_client->didReceiveData(data, lengthReceived);
        }

        void didFinishLoading(unsigned long identifier)
        {
            if (m_client)
                m_client->didFinishLoading(identifier);
        }

        void didFail()
        {
            if (m_client)
                m_client->didFail();
        }

        void didGetCancelled()
        {
            if (m_client)
                m_client->didGetCancelled();
        }

        void didReceiveAuthenticationCancellation(const ResourceResponse& response)
        {
            if (m_client)
                m_client->didReceiveResponse(response);
        }

    protected:
        ThreadableLoaderClientWrapper(ThreadableLoaderClient* client)
            : m_client(client)
        {
        }

        ThreadableLoaderClient* m_client;
    };

} // namespace WebCore

#endif // ThreadableLoaderClientWrapper_h
