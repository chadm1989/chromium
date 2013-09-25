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

#ifndef WebURLRequest_h
#define WebURLRequest_h

#include "WebCommon.h"
#include "WebHTTPBody.h"

#if BLINK_IMPLEMENTATION
namespace WebCore { class ResourceRequest; }
#endif

namespace WebKit {

class WebCString;
class WebHTTPBody;
class WebHTTPHeaderVisitor;
class WebString;
class WebURL;
class WebURLRequestPrivate;

class WebURLRequest {
public:
    enum CachePolicy {
        UseProtocolCachePolicy, // normal load
        ReloadIgnoringCacheData, // reload
        ReturnCacheDataElseLoad, // back/forward or encoding change - allow stale data
        ReturnCacheDataDontLoad, // results of a post - allow stale data and only use cache
    };

    enum Priority {
        PriorityUnresolved = -1,
        PriorityVeryLow,
        PriorityLow,
        PriorityMedium,
        PriorityHigh,
        PriorityVeryHigh,
    };

    enum TargetType {
        TargetIsMainFrame = 0,
        TargetIsSubframe = 1,
        TargetIsSubresource = 2,
        TargetIsStyleSheet = 3,
        TargetIsScript = 4,
        TargetIsFontResource = 5,
        TargetIsImage = 6,
        TargetIsObject = 7,
        TargetIsMedia = 8,
        TargetIsWorker = 9,
        TargetIsSharedWorker = 10,
        TargetIsPrefetch = 11,
        TargetIsFavicon = 12,
        TargetIsXHR = 13,
        TargetIsTextTrack = 14,
        TargetIsUnspecified = 15,
    };

    class ExtraData {
    public:
        virtual ~ExtraData() { }
    };

    ~WebURLRequest() { reset(); }

    WebURLRequest() : m_private(0) { }
    WebURLRequest(const WebURLRequest& r) : m_private(0) { assign(r); }
    WebURLRequest& operator=(const WebURLRequest& r)
    {
        assign(r);
        return *this;
    }

    explicit WebURLRequest(const WebURL& url) : m_private(0)
    {
        initialize();
        setURL(url);
    }

    BLINK_EXPORT void initialize();
    BLINK_EXPORT void reset();
    BLINK_EXPORT void assign(const WebURLRequest&);

    BLINK_EXPORT bool isNull() const;

    BLINK_EXPORT WebURL url() const;
    BLINK_EXPORT void setURL(const WebURL&);

    // Used to implement third-party cookie blocking.
    BLINK_EXPORT WebURL firstPartyForCookies() const;
    BLINK_EXPORT void setFirstPartyForCookies(const WebURL&);

    BLINK_EXPORT bool allowCookies() const;
    BLINK_EXPORT void setAllowCookies(bool);

    // Controls whether user name, password, and cookies may be sent with the
    // request. (If false, this overrides allowCookies.)
    BLINK_EXPORT bool allowStoredCredentials() const;
    BLINK_EXPORT void setAllowStoredCredentials(bool);

    BLINK_EXPORT CachePolicy cachePolicy() const;
    BLINK_EXPORT void setCachePolicy(CachePolicy);

    BLINK_EXPORT WebString httpMethod() const;
    BLINK_EXPORT void setHTTPMethod(const WebString&);

    BLINK_EXPORT WebString httpHeaderField(const WebString& name) const;
    BLINK_EXPORT void setHTTPHeaderField(const WebString& name, const WebString& value);
    BLINK_EXPORT void addHTTPHeaderField(const WebString& name, const WebString& value);
    BLINK_EXPORT void clearHTTPHeaderField(const WebString& name);
    BLINK_EXPORT void visitHTTPHeaderFields(WebHTTPHeaderVisitor*) const;

    BLINK_EXPORT WebHTTPBody httpBody() const;
    BLINK_EXPORT void setHTTPBody(const WebHTTPBody&);

    // Controls whether upload progress events are generated when a request
    // has a body.
    BLINK_EXPORT bool reportUploadProgress() const;
    BLINK_EXPORT void setReportUploadProgress(bool);

    // Controls whether load timing info is collected for the request.
    BLINK_EXPORT bool reportLoadTiming() const;
    BLINK_EXPORT void setReportLoadTiming(bool);

    // Controls whether actual headers sent and received for request are
    // collected and reported.
    BLINK_EXPORT bool reportRawHeaders() const;
    BLINK_EXPORT void setReportRawHeaders(bool);

    BLINK_EXPORT TargetType targetType() const;
    BLINK_EXPORT void setTargetType(TargetType);

    // True if the request was user initiated.
    BLINK_EXPORT bool hasUserGesture() const;
    BLINK_EXPORT void setHasUserGesture(bool);

    // A consumer controlled value intended to be used to identify the
    // requestor.
    BLINK_EXPORT int requestorID() const;
    BLINK_EXPORT void setRequestorID(int);

    // A consumer controlled value intended to be used to identify the
    // process of the requestor.
    BLINK_EXPORT int requestorProcessID() const;
    BLINK_EXPORT void setRequestorProcessID(int);

    // Allows the request to be matched up with its app cache host.
    BLINK_EXPORT int appCacheHostID() const;
    BLINK_EXPORT void setAppCacheHostID(int);

    // If true, the response body will be downloaded to a file managed by the
    // WebURLLoader. See WebURLResponse::downloadedFilePath.
    BLINK_EXPORT bool downloadToFile() const;
    BLINK_EXPORT void setDownloadToFile(bool);

    // Extra data associated with the underlying resource request. Resource
    // requests can be copied. If non-null, each copy of a resource requests
    // holds a pointer to the extra data, and the extra data pointer will be
    // deleted when the last resource request is destroyed. Setting the extra
    // data pointer will cause the underlying resource request to be
    // dissociated from any existing non-null extra data pointer.
    BLINK_EXPORT ExtraData* extraData() const;
    BLINK_EXPORT void setExtraData(ExtraData*);

    BLINK_EXPORT Priority priority() const;

#if BLINK_IMPLEMENTATION
    WebCore::ResourceRequest& toMutableResourceRequest();
    const WebCore::ResourceRequest& toResourceRequest() const;
#endif

protected:
    void assign(WebURLRequestPrivate*);

private:
    WebURLRequestPrivate* m_private;
};

} // namespace WebKit

#endif
