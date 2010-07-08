/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 *           (C) 2009 Brent Fulgham <bfulgham@webkit.org>
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

#include "config.h"
#include "PixelDumpSupportCairo.h"

#include "DumpRenderTree.h"
#include "PixelDumpSupport.h"
#include <algorithm>
#include <ctype.h>
#include <wtf/Assertions.h>
#include <wtf/RefPtr.h>
#include <wtf/RetainPtr.h>
#include <wtf/StringExtras.h>

#if PLATFORM(WIN)
#include "MD5.h"
#endif

using namespace std;

static cairo_status_t writeFunction(void* closure, const unsigned char* data, unsigned int length)
{
    Vector<unsigned char>* in = reinterpret_cast<Vector<unsigned char>*>(closure);
    in->append(data, length);
    return CAIRO_STATUS_SUCCESS;
}

static void printPNG(cairo_surface_t* image)
{
    Vector<unsigned char> pixelData;
    // Only PNG output is supported for now.
    cairo_surface_write_to_png_stream(image, writeFunction, &pixelData);

    const size_t dataLength = pixelData.size();
    const unsigned char* data = pixelData.data();

    printPNG(data, dataLength);
}

void computeMD5HashStringForBitmapContext(BitmapContext* context, char hashString[33])
{
    cairo_t* bitmapContext = context->cairoContext();
    cairo_surface_t* surface = cairo_get_target(bitmapContext);

    ASSERT(cairo_image_surface_get_format(surface) == CAIRO_FORMAT_ARGB32); // ImageDiff assumes 32 bit RGBA, we must as well.

    size_t pixelsHigh = cairo_image_surface_get_height(surface);
    size_t pixelsWide = cairo_image_surface_get_width(surface);
    size_t bytesPerRow = pixelsWide * cairo_image_surface_get_stride(surface);

    MD5_CTX md5Context;
    MD5_Init(&md5Context);
    unsigned char* bitmapData = static_cast<unsigned char*>(cairo_image_surface_get_data(surface));
    for (unsigned row = 0; row < pixelsHigh; row++) {
        MD5_Update(&md5Context, bitmapData, 4 * pixelsWide);
        bitmapData += bytesPerRow;
    }
    unsigned char hash[16];
    MD5_Final(hash, &md5Context);

    hashString[0] = '\0';
    for (int i = 0; i < 16; i++)
        snprintf(hashString, 33, "%s%02x", hashString, hash[i]);
}

void dumpBitmap(BitmapContext* context)
{
    cairo_surface_t* surface = cairo_get_target(context->cairoContext());
    printPNG(surface);
}
