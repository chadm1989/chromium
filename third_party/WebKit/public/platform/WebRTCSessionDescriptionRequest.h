/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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

#ifndef WebRTCSessionDescriptionRequest_h
#define WebRTCSessionDescriptionRequest_h

#include "WebCommon.h"
#include "WebNonCopyable.h"
#include "WebPrivatePtr.h"
#include "WebString.h"

namespace WebCore {
class RTCSessionDescriptionRequest;
}

namespace blink {
class WebRTCSessionDescription;

class WebRTCSessionDescriptionRequest {
public:
    class ExtraData {
    public:
        virtual ~ExtraData() { }
    };

    WebRTCSessionDescriptionRequest() { }
    WebRTCSessionDescriptionRequest(const WebRTCSessionDescriptionRequest& other) { assign(other); }
    ~WebRTCSessionDescriptionRequest() { reset(); }

    WebRTCSessionDescriptionRequest& operator=(const WebRTCSessionDescriptionRequest& other)
    {
        assign(other);
        return *this;
    }

    BLINK_PLATFORM_EXPORT void assign(const WebRTCSessionDescriptionRequest&);

    BLINK_PLATFORM_EXPORT void reset();
    bool isNull() const { return m_private.isNull(); }

    BLINK_PLATFORM_EXPORT void requestSucceeded(const WebRTCSessionDescription&) const;
    BLINK_PLATFORM_EXPORT void requestFailed(const WebString& error) const;

    // Extra data associated with this object.
    // If non-null, the extra data pointer will be deleted when the object is destroyed.
    // Setting the extra data pointer will cause any existing non-null
    // extra data pointer to be deleted.
    BLINK_PLATFORM_EXPORT ExtraData* extraData() const;
    BLINK_PLATFORM_EXPORT void setExtraData(ExtraData*);

#if INSIDE_BLINK
    BLINK_PLATFORM_EXPORT WebRTCSessionDescriptionRequest(const WTF::PassRefPtr<WebCore::RTCSessionDescriptionRequest>&);
#endif

private:
    WebPrivatePtr<WebCore::RTCSessionDescriptionRequest> m_private;
};

} // namespace blink

#endif // WebRTCSessionDescriptionRequest_h
