/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "WebKitDLL.h"
#include "WebError.h"
#include "WebKit.h"

#pragma warning(push, 0)
#include <WebCore/BString.h>
#pragma warning(pop)

using namespace WebCore;

// WebError ---------------------------------------------------------------------

WebError::WebError(const ResourceError& error, IPropertyBag* userInfo)
    : m_refCount(0)
    , m_error(error)
    , m_userInfo(userInfo)
{
    gClassCount++;
}

WebError::~WebError()
{
    gClassCount--;
}

WebError* WebError::createInstance(const ResourceError& error, IPropertyBag* userInfo)
{
    WebError* instance = new WebError(error, userInfo);
    instance->AddRef();
    return instance;
}

// IUnknown -------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebError::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualGUID(riid, IID_IUnknown))
        *ppvObject = static_cast<IUnknown*>(this);
    else if (IsEqualGUID(riid, CLSID_WebError))
        *ppvObject = static_cast<WebError*>(this);
    else if (IsEqualGUID(riid, IID_IWebError))
        *ppvObject = static_cast<IWebError*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE WebError::AddRef(void)
{
    return ++m_refCount;
}

ULONG STDMETHODCALLTYPE WebError::Release(void)
{
    ULONG newRef = --m_refCount;
    if (!newRef)
        delete(this);

    return newRef;
}

// IWebError ------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebError::init( 
    /* [in] */ BSTR domain,
    /* [in] */ int code,
    /* [in] */ BSTR url)
{
    m_error = ResourceError(String(domain, SysStringLen(domain)), code, String(url, SysStringLen(url)), String());
    return S_OK;
}
  
HRESULT STDMETHODCALLTYPE WebError::code( 
    /* [retval][out] */ int* result)
{
    *result = m_error.errorCode();
    return S_OK;
}
        
HRESULT STDMETHODCALLTYPE WebError::domain( 
    /* [retval][out] */ BSTR* result)
{
    if (!result)
        return E_POINTER;

    *result = BString(m_error.domain()).release();
    return S_OK;
}
               
HRESULT STDMETHODCALLTYPE WebError::localizedDescription( 
    /* [retval][out] */ BSTR* result)
{
    if (!result)
        return E_POINTER;

    *result = BString(m_error.localizedDescription()).release();
    return S_OK;
}

        
HRESULT STDMETHODCALLTYPE WebError::localizedFailureReason( 
    /* [retval][out] */ BSTR* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
        
HRESULT STDMETHODCALLTYPE WebError::localizedRecoveryOptions( 
    /* [retval][out] */ IEnumVARIANT** /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
        
HRESULT STDMETHODCALLTYPE WebError::localizedRecoverySuggestion( 
    /* [retval][out] */ BSTR* /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
       
HRESULT STDMETHODCALLTYPE WebError::recoverAttempter( 
    /* [retval][out] */ IUnknown** /*result*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}
        
HRESULT STDMETHODCALLTYPE WebError::userInfo( 
    /* [retval][out] */ IPropertyBag** result)
{
    if (!result)
        return E_POINTER;
    *result = 0;

    if (!m_userInfo)
        return E_FAIL;

    return m_userInfo.copyRefTo(result);
}

HRESULT STDMETHODCALLTYPE WebError::failingURL( 
    /* [retval][out] */ BSTR* result)
{
    if (!result)
        return E_POINTER;

    *result = BString(m_error.failingURL()).release();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebError::isPolicyChangeError( 
    /* [retval][out] */ BOOL *result)
{
    if (!result)
        return E_POINTER;

    *result = m_error.domain() == String(WebKitErrorDomain)
        && m_error.errorCode() == WebKitErrorFrameLoadInterruptedByPolicyChange;
    return S_OK;
}

const ResourceError& WebError::resourceError() const
{
    return m_error;
}
