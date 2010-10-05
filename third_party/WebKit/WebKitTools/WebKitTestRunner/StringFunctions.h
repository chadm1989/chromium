/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef StringFunctions_h
#define StringFunctions_h

#include <JavaScriptCore/JSRetainPtr.h>
#include <JavaScriptCore/JavaScript.h>
#include <WebKit2/WKRetainPtr.h>
#include <WebKit2/WKString.h>
#include <WebKit2/WKStringCF.h>
#include <WebKit2/WKStringPrivate.h>
#include <WebKit2/WKURL.h>
#include <WebKit2/WKURLCF.h>
#include <sstream>
#include <string>
#include <wtf/OwnArrayPtr.h>
#include <wtf/PassOwnArrayPtr.h>
#include <wtf/Platform.h>
#include <wtf/RetainPtr.h>
#include <wtf/Vector.h>

namespace WTR {

// Conversion functions

inline WKRetainPtr<WKStringRef> toWK(JSStringRef string)
{
    return WKRetainPtr<WKStringRef>(AdoptWK, WKStringCreateWithJSString(string));
}

inline WKRetainPtr<WKStringRef> toWK(JSRetainPtr<JSStringRef> string)
{
    return toWK(string.get());
}

inline JSRetainPtr<JSStringRef> toJS(WKStringRef string)
{
    return JSRetainPtr<JSStringRef>(Adopt, WKStringCopyJSString(string));
}

inline JSRetainPtr<JSStringRef> toJS(const WKRetainPtr<WKStringRef>& string)
{
    return toJS(string.get());
}

inline std::string toSTD(WKStringRef string)
{
    size_t bufferSize = WKStringGetMaximumUTF8CStringSize(string);
    OwnArrayPtr<char> buffer = adoptArrayPtr(new char[bufferSize]);
    size_t stringLength = WKStringGetUTF8CString(string, buffer.get(), bufferSize);
    return std::string(buffer.get(), stringLength - 1);
}

inline std::string toSTD(const WKRetainPtr<WKStringRef>& string)
{
    return toSTD(string.get());
}

// Streaming functions

inline std::ostream& operator<<(std::ostream& out, WKStringRef stringRef)
{
    if (!stringRef)
        return out;

    return out << toSTD(stringRef);
}

inline std::ostream& operator<<(std::ostream& out, const WKRetainPtr<WKStringRef>& stringRef)
{
    return out << stringRef.get();
}

// URL Functions

inline WKURLRef createWKURL(const char* pathOrURL)
{
    RetainPtr<CFStringRef> pathOrURLCFString(AdoptCF, CFStringCreateWithCString(0, pathOrURL, kCFStringEncodingUTF8));
    RetainPtr<CFURLRef> cfURL;
    if (CFStringHasPrefix(pathOrURLCFString.get(), CFSTR("http://")) || CFStringHasPrefix(pathOrURLCFString.get(), CFSTR("https://")))
        cfURL.adoptCF(CFURLCreateWithString(0, pathOrURLCFString.get(), 0));
    else
#if PLATFORM(WIN)
        cfURL.adoptCF(CFURLCreateWithFileSystemPath(0, pathOrURLCFString.get(), kCFURLWindowsPathStyle, false));
#else
        cfURL.adoptCF(CFURLCreateWithFileSystemPath(0, pathOrURLCFString.get(), kCFURLPOSIXPathStyle, false));
#endif
    return WKURLCreateWithCFURL(cfURL.get());
}

inline WKStringRef copyURLString(WKURLRef url)
{
    RetainPtr<CFURLRef> cfURL(AdoptCF, WKURLCopyCFURL(0, url));
    return WKStringCreateWithCFString(CFURLGetString(cfURL.get()));
}

} // namespace WTR

#endif // StringFunctions_h
