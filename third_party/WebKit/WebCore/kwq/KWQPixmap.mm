/*
 * Copyright (C) 2001, 2002 Apple Computer, Inc.  All rights reserved.
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

#import "KWQPixmap.h"

#import "WebCoreImageRenderer.h"
#import "WebCoreImageRendererFactory.h"

QPixmap::QPixmap()
{
    imageRenderer = nil;
    needCopyOnWrite = false;
}

QPixmap::QPixmap(const QSize &sz)
{
    imageRenderer = [[[WebCoreImageRendererFactory sharedFactory] imageRendererWithSize:NSMakeSize(sz.width(), sz.height())] retain];
    needCopyOnWrite = false;
}

QPixmap::QPixmap(const QByteArray &bytes)
{
    imageRenderer = [[[WebCoreImageRendererFactory sharedFactory] imageRendererWithBytes:bytes.data() length:bytes.size()] retain];
    needCopyOnWrite = false;
}

QPixmap::QPixmap(int w, int h)
{
    imageRenderer = [[[WebCoreImageRendererFactory sharedFactory] imageRendererWithSize:NSMakeSize(w, h)] retain];
    needCopyOnWrite = false;
}

QPixmap::QPixmap(const QPixmap &copyFrom) : QPaintDevice(copyFrom)
{
    imageRenderer = [copyFrom.imageRenderer retain];
    copyFrom.needCopyOnWrite = true;
    needCopyOnWrite = true;
}

QPixmap::~QPixmap()
{
    [imageRenderer release];
}

bool QPixmap::receivedData(const QByteArray &bytes, bool isComplete)
{
    if (imageRenderer == nil) {
        imageRenderer = [[[WebCoreImageRendererFactory sharedFactory] imageRenderer] retain];
    }
    return [imageRenderer incrementalLoadWithBytes:bytes.data() length:bytes.size() complete:isComplete];
}

bool QPixmap::mask() const
{
    return false;
}

bool QPixmap::isNull() const
{
    return imageRenderer == nil;
}

QSize QPixmap::size() const
{
    NSSize sz = [imageRenderer size];
    return QSize((int)sz.width, (int)sz.height);
}

QRect QPixmap::rect() const
{
    NSSize sz = [imageRenderer size];
    return QRect(0, 0, (int)sz.width, (int)sz.height);
}

int QPixmap::width() const
{
    return (int)[imageRenderer size].width;
}

int QPixmap::height() const
{
    return (int)[imageRenderer size].height;
}

void QPixmap::resize(const QSize &sz)
{
    resize(sz.width(), sz.height());
}

void QPixmap::resize(int w, int h)
{
    if (needCopyOnWrite) {
        id <WebCoreImageRenderer> newImageRenderer = [imageRenderer copyWithZone:NULL];
        [imageRenderer release];
        imageRenderer = newImageRenderer;
        needCopyOnWrite = false;
    }
    [imageRenderer resize:NSMakeSize(w, h)];
}

QPixmap QPixmap::xForm(const QWMatrix &xmatrix) const
{
    // This function is only called when an image needs to be scaled.  
    // We can depend on render_image.cpp to call resize AFTER
    // creating a copy of the image to be scaled. So, this
    // implementation simply returns a copy of the image. Note,
    // this assumption depends on the implementation of
    // RenderImage::printObject.   
    return *this;
}

QPixmap &QPixmap::operator=(const QPixmap &assignFrom)
{
    [assignFrom.imageRenderer retain];
    [imageRenderer release];
    imageRenderer = assignFrom.imageRenderer;
    assignFrom.needCopyOnWrite = true;
    needCopyOnWrite = true;
    return *this;
}

void QPixmap::stopAnimations()
{
    [imageRenderer stopAnimation];
}
