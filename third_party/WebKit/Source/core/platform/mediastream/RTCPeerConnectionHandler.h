/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Google Inc. nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
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

#ifndef RTCPeerConnectionHandler_h
#define RTCPeerConnectionHandler_h

#include "public/platform/WebMediaStream.h"
#include <wtf/PassOwnPtr.h>
#include <wtf/PassRefPtr.h>

namespace WebKit {
class WebRTCICECandidate;
class WebRTCSessionDescription;
struct WebRTCDataChannelInit;
}

namespace WebCore {

class MediaConstraints;
class MediaStreamComponent;
class RTCConfiguration;
class RTCDTMFSenderHandler;
class RTCDataChannelHandler;
class RTCPeerConnectionHandlerClient;
class RTCSessionDescriptionRequest;
class RTCStatsRequest;
class RTCVoidRequest;

class RTCPeerConnectionHandler {
public:
    static PassOwnPtr<RTCPeerConnectionHandler> create(RTCPeerConnectionHandlerClient*);
    virtual ~RTCPeerConnectionHandler() { }

    virtual bool initialize(PassRefPtr<RTCConfiguration>, PassRefPtr<MediaConstraints>) = 0;

    virtual void createOffer(PassRefPtr<RTCSessionDescriptionRequest>, PassRefPtr<MediaConstraints>) = 0;
    virtual void createAnswer(PassRefPtr<RTCSessionDescriptionRequest>, PassRefPtr<MediaConstraints>) = 0;
    virtual void setLocalDescription(PassRefPtr<RTCVoidRequest>, WebKit::WebRTCSessionDescription) = 0;
    virtual void setRemoteDescription(PassRefPtr<RTCVoidRequest>, WebKit::WebRTCSessionDescription) = 0;
    virtual WebKit::WebRTCSessionDescription localDescription() = 0;
    virtual WebKit::WebRTCSessionDescription remoteDescription() = 0;
    virtual bool updateIce(PassRefPtr<RTCConfiguration>, PassRefPtr<MediaConstraints>) = 0;
    virtual bool addIceCandidate(WebKit::WebRTCICECandidate) = 0;
    virtual bool addStream(WebKit::WebMediaStream, PassRefPtr<MediaConstraints>) = 0;
    virtual void removeStream(WebKit::WebMediaStream) = 0;
    virtual void getStats(PassRefPtr<RTCStatsRequest>) = 0;
    virtual PassOwnPtr<RTCDataChannelHandler> createDataChannel(const String& label, const WebKit::WebRTCDataChannelInit&) = 0;
    virtual PassOwnPtr<RTCDTMFSenderHandler> createDTMFSender(PassRefPtr<MediaStreamComponent>) = 0;
    virtual void stop() = 0;

protected:
    RTCPeerConnectionHandler() { }
};

} // namespace WebCore

#endif // RTCPeerConnectionHandler_h
