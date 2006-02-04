/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
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

#import "config.h"
#import "Screen.h"

#import "IntRect.h"
#import "KWQWidget.h"

namespace WebCore {

static NSScreen* screen(QWidget* widget)
{
    if (widget)
        if (NSScreen* screen = [[widget->getView() window] screen])
            return screen;
    return [NSScreen mainScreen];
}

static NSRect flipGlobalRect(NSRect rect)
{
    rect.origin.y = NSMaxY([[[NSScreen screens] objectAtIndex:0] frame]) - NSMaxY(rect);
    return rect;
}

int screenDepth(QWidget* widget)
{
    return [screen(widget) depth];
}

int screenDepthPerComponent(QWidget* widget)
{
   return NSBitsPerSampleFromDepth([screen(widget) depth]);
}

bool screenIsMonochrome(QWidget* widget)
{
    NSScreen* s = screen(widget);
    NSDictionary* dd = [s deviceDescription];
    NSString* colorSpaceName = [dd objectForKey:NSDeviceColorSpaceName];
    // XXX: can named colorspace or custom colorspace be monochrome?
    // XXX: will NS*BlackColorSpace or NS*WhiteColorSpace be always monochrome?
    return colorSpaceName == NSCalibratedWhiteColorSpace || colorSpaceName == NSCalibratedBlackColorSpace || 
        colorSpaceName == NSDeviceWhiteColorSpace || colorSpaceName == NSDeviceBlackColorSpace;
}

IntRect screenRect(QWidget* widget)
{
    return enclosingIntRect(flipGlobalRect([screen(widget) frame]));
}

IntRect usableScreenRect(QWidget* widget)
{
    return enclosingIntRect(flipGlobalRect([screen(widget) visibleFrame]));
}

}
