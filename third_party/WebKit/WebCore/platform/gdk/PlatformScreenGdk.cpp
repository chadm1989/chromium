/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2006 Michael Emmel mike.emmel@gmail.com 
 * All rights reserved.
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
#include "PlatformScreen.h"

#include "NotImplemented.h"
#include "Widget.h"
#include <gdk/gdk.h>
#include <stdio.h>
#include <wtf/Assertions.h>

namespace WebCore {

int screenDepth(Widget* widget) 
{
    ASSERT(widget->gdkDrawable());

    gint dummy, depth;
    gdk_window_get_geometry(widget->gdkDrawable(), &dummy, &dummy, &dummy, &dummy, &depth);
    return depth;
}

int screenDepthPerComponent(Widget*)
{
    notImplemented();
    return 8;
}

bool screenIsMonochrome(Widget*) 
{ 
    notImplemented(); 
    return false; 
}

FloatRect screenRect(Widget*) 
{ 
    notImplemented();
    return FloatRect(); 
}

FloatRect screenAvailableRect(Widget*) 
{ 
    notImplemented(); 
    return FloatRect(); 
}

} // namespace WebCore
