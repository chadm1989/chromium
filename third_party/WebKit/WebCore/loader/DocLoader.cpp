/*
    This file is part of the KDE libraries

    Copyright (C) 1998 Lars Knoll (knoll@mpi-hd.mpg.de)
    Copyright (C) 2001 Dirk Mueller (mueller@kde.org)
    Copyright (C) 2002 Waldo Bastian (bastian@kde.org)
    Copyright (C) 2004, 2005, 2006 Apple Computer, Inc.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.

    This class provides all functionality needed for loading images, style sheets and html
    pages from the web. It has a memory cache for these objects.
*/

#include "config.h"
#include "DocLoader.h"

#include "Cache.h"
#include "CachedCSSStyleSheet.h"
#include "CachedImage.h"
#include "CachedScript.h"
#include "CachedXSLStyleSheet.h"
#include "Document.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "loader.h"

namespace WebCore {

DocLoader::DocLoader(Frame *frame, Document* doc)
: m_cache(cache())
{
    m_cachePolicy = CachePolicyVerify;
    m_expireDate = 0;
    m_autoLoadImages = true;
    m_frame = frame;
    m_doc = doc;
    m_loadInProgress = false;

    m_cache->addDocLoader(this);
}

DocLoader::~DocLoader()
{
    m_cache->removeDocLoader(this);
}

void DocLoader::setExpireDate(time_t _expireDate)
{
    m_expireDate = _expireDate;
}

void DocLoader::checkForReload(const KURL& fullURL)
{
    if (m_cachePolicy == CachePolicyVerify) {
       if (!m_reloadedURLs.contains(fullURL.url())) {
          CachedResource* existing = cache()->resourceForURL(fullURL.url());
          if (existing && existing->isExpired()) {
             cache()->remove(existing);
             m_reloadedURLs.add(fullURL.url());
          }
       }
    } else if ((m_cachePolicy == CachePolicyReload) || (m_cachePolicy == CachePolicyRefresh)) {
       if (!m_reloadedURLs.contains(fullURL.url())) {
          CachedResource* existing = cache()->resourceForURL(fullURL.url());
          if (existing)
             cache()->remove(existing);
          m_reloadedURLs.add(fullURL.url());
       }
    }
}

CachedImage* DocLoader::requestImage(const String& url)
{
    CachedImage* resource = static_cast<CachedImage*>(requestResource(CachedResource::ImageResource, url));
    if (autoLoadImages() && resource && resource->stillNeedsLoad()) {
        resource->setLoading(true);
        cache()->loader()->load(this, resource, true);
    }
    return resource;
}

CachedCSSStyleSheet* DocLoader::requestCSSStyleSheet(const String& url, const String& charset, bool isUserStyleSheet)
{
    return static_cast<CachedCSSStyleSheet*>(requestResource(CachedResource::CSSStyleSheet, url, &charset, isUserStyleSheet));
}

CachedCSSStyleSheet* DocLoader::requestUserCSSStyleSheet(const String& url, const String& charset)
{
    return requestCSSStyleSheet(url, charset, true);
}

CachedScript* DocLoader::requestScript(const String& url, const String& charset)
{
    return static_cast<CachedScript*>(requestResource(CachedResource::Script, url, &charset));
}

#ifdef XSLT_SUPPORT
CachedXSLStyleSheet* DocLoader::requestXSLStyleSheet(const String& url)
{
    return static_cast<CachedXSLStyleSheet*>(requestResource(CachedResource::XSLStyleSheet, url));
}
#endif

#ifdef XBL_SUPPORT
CachedXBLDocument* DocLoader::requestXBLDocument(const String& url)
{
    return static_cast<CachedXSLStyleSheet*>(requestResource(CachedResource::XBL, url));
}
#endif

CachedResource* DocLoader::requestResource(CachedResource::Type type, const String& url, const String* charset, bool skipCanLoadCheck)
{
    KURL fullURL = m_doc->completeURL(url.deprecatedString());

    FrameLoader* fl = 0;
    if (m_frame)
        fl = m_frame->loader();
    if (fl && fl->isReloading())
        setCachePolicy(CachePolicyReload);

    checkForReload(fullURL);

    bool hideReferrer;
    CachedResource* resource = 0;
    //If you are not allowed to load you may not get from the cache either.
    if (skipCanLoadCheck || (fl && fl->canLoad(fullURL, fl->outgoingReferrer(), hideReferrer))) {
        resource = cache()->requestResource(this, type, fullURL, m_expireDate, charset);
        if (resource) {
            m_docResources.set(resource->url(), resource);
            checkCacheObjectStatus(resource);
        }
    }

    return resource;
}

void DocLoader::setAutoLoadImages(bool enable)
{
    if (enable == m_autoLoadImages)
        return;

    m_autoLoadImages = enable;

    if (!m_autoLoadImages)
        return;

    HashMap<String, CachedResource*>::iterator end = m_docResources.end();
    for (HashMap<String, CachedResource*>::iterator it = m_docResources.begin(); it != end; ++it) {
        CachedResource* resource = it->second;
        if (resource->type() == CachedResource::ImageResource) {
            CachedImage* image = const_cast<CachedImage*>(static_cast<const CachedImage *>(resource));

            CachedResource::Status status = image->status();
            if (status != CachedResource::Unknown)
                continue;

            cache()->loader()->load(this, image, true);
        }
    }
}

void DocLoader::setCachePolicy(CachePolicy cachePolicy)
{
    m_cachePolicy = cachePolicy;
}

void DocLoader::removeCachedResource(CachedResource* resource) const
{
    m_docResources.remove(resource->url());
}

void DocLoader::setLoadInProgress(bool load)
{
    m_loadInProgress = load;
    if (!load && m_frame)
        m_frame->loader()->loadDone();
}

void DocLoader::checkCacheObjectStatus(CachedResource* resource)
{
        // Return from the function for objects that we didn't load from the cache.
        if (!resource)
            return;
        switch (resource->status()) {
            case CachedResource::Cached:
                break;
            case CachedResource::NotCached:
            case CachedResource::Unknown:
            case CachedResource::New:
            case CachedResource::Pending:
                return;
        }
        
        // Notify the caller that we "loaded".
        if (!m_frame || m_frame->loader()->haveToldBridgeAboutLoad(resource->url()))
            return;
        
        ResourceRequest request(resource->url());
        const ResourceResponse& response = resource->response();
        SharedBuffer* data = resource->allData();
        
        // FIXME: If the WebKit client changes or cancels the request, WebCore does not respect this and continues the load.
        m_frame->loader()->loadedResourceFromMemoryCache(request, response, data ? data->size() : 0);
        m_frame->loader()->didTellBridgeAboutLoad(resource->url());
}

}
