/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
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

#ifndef WebCryptoKey_h
#define WebCryptoKey_h

#include "WebCommon.h"
#include "WebPrivatePtr.h"

namespace WebKit {

enum WebCryptoKeyType {
    WebCryptoKeyTypeSecret,
    WebCryptoKeyTypePublic,
    WebCryptoKeyTypePrivate,
};

enum WebCryptoKeyUsage {
    WebCryptoKeyUsageEncrypt = 1 << 0,
    WebCryptoKeyUsageDecrypt = 1 << 1,
    WebCryptoKeyUsageSign = 1 << 2,
    WebCryptoKeyUsageVerify = 1 << 3,
    WebCryptoKeyUsageDerive = 1 << 4,
    WebCryptoKeyUsageWrap = 1 << 5,
    WebCryptoKeyUsageUnwrap = 1 << 6,
#if WEBKIT_IMPLEMENTATION
    EndOfWebCryptoKeyUsage,
#endif
};

// A bitfield of WebCryptoKeyUsage
typedef int WebCryptoKeyUsageMask;

class WebCryptoAlgorithm;
class WebCryptoKeyPrivate;
class WebCryptoKeyHandle;

// The WebCryptoKey represents a key from the Web Crypto API:
//
// https://dvcs.w3.org/hg/webcrypto-api/raw-file/tip/spec/Overview.html#key-interface
//
// WebCryptoKey is just a reference-counted wrapper that manages the lifetime of
// a "WebCryptoKeyHandle*".
//
// WebCryptoKey is:
//   * Copiable (cheaply)
//   * Threadsafe if the embedder's WebCryptoKeyHandle is also threadsafe.
//
// The embedder is responsible for creating all WebCryptoKeys, and therefore can
// safely assume any details regarding the type of the wrapped
// WebCryptoKeyHandle*.
//
// FIXME: Define the interface to use for structured clone.
//        Cloning across a process boundary will need serialization,
//        however cloning for in-process workers could just share the same
//        (threadsafe) handle.
class WebCryptoKey {
public:
    ~WebCryptoKey() { reset(); }

    WebCryptoKey(const WebCryptoKey& other) { assign(other); }
    WebCryptoKey& operator=(const WebCryptoKey& other)
    {
        assign(other);
        return *this;
    }

    // For an explanation of these parameters see:
    // https://dvcs.w3.org/hg/webcrypto-api/raw-file/tip/spec/Overview.html#key-interface-members
    //
    // Note that the caller is passing ownership of the WebCryptoKeyHandle*.
    WEBKIT_EXPORT static WebCryptoKey create(WebCryptoKeyHandle*, WebCryptoKeyType, bool extractable, const WebCryptoAlgorithm&, WebCryptoKeyUsageMask);

    // Returns the opaque key handle that was set by the embedder.
    //   * Safe to downcast to known type (since embedder creates all the keys)
    //   * Returned pointer's lifetime is bound to |this|
    WEBKIT_EXPORT WebCryptoKeyHandle* handle() const;

    WEBKIT_EXPORT WebCryptoKeyType type() const;
    WEBKIT_EXPORT bool extractable() const;
    WEBKIT_EXPORT const WebCryptoAlgorithm& algorithm() const;
    WEBKIT_EXPORT WebCryptoKeyUsageMask keyUsage() const;

private:
    WebCryptoKey() { }
    WEBKIT_EXPORT void assign(const WebCryptoKey& other);
    WEBKIT_EXPORT void reset();

    WebPrivatePtr<WebCryptoKeyPrivate> m_private;
};

// Base class for the embedder to define its own opaque key handle. The lifetime
// of this object is controlled by WebCryptoKey using reference counting.
class WebCryptoKeyHandle {
public:
    virtual ~WebCryptoKeyHandle() { }
};

} // namespace WebKit

#endif
