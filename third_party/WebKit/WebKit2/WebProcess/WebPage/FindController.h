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

#ifndef FindController_h
#define FindController_h

#include "FindOptions.h"
#include <wtf/Forward.h>
#include <wtf/Noncopyable.h>

namespace WebCore {
    class Frame;
}

namespace WebKit {

class FindPageOverlay;
class WebPage;

class FindController {
    WTF_MAKE_NONCOPYABLE(FindController);

public:
    explicit FindController(WebPage*);

    void findString(const String&, FindDirection, FindOptions, unsigned maxNumMatches);
    void hideFindUI();

    void findPageOverlayDestroyed();

private:
    bool updateFindIndicator(WebCore::Frame* selectedFrame, bool isShowingOverlay);
    void resetFindIndicator();
    
    WebPage* m_webPage;
    FindPageOverlay* m_findPageOverlay;

    // Whether the UI process is showing the find indicator. Note that this can be true even if
    // the find indicator isn't showing, but it will never be false when it is showing.
    bool m_isShowingFindIndicator;
};

} // namespace WebKit

#endif // FindController_h
