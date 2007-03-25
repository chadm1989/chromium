/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
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
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
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

#import <WebCore/DocumentLoader.h>
#import <wtf/RetainPtr.h>
#import <wtf/HashSet.h>

@class WebDataSource;
@class WebView;

namespace WebCore {
    class ResourceRequest;
}

class WebDocumentLoaderMac : public WebCore::DocumentLoader
{
public:
    WebDocumentLoaderMac(const WebCore::ResourceRequest&, const WebCore::SubstituteData&);

    void setDataSource(WebDataSource *, WebView*);
    WebDataSource *dataSource() const;

    virtual void attachToFrame();
    virtual void detachFromFrame();

    void increaseLoadCount(unsigned long identifier);
    void decreaseLoadCount(unsigned long identifier);
private:
    WebDataSource *m_dataSource;
    RetainPtr<id> m_resourceLoadDelegate;
    RetainPtr<id> m_downloadDelegate;
    bool m_hasEverBeenDetached;
    HashSet<unsigned long> m_loadingResources;
};
