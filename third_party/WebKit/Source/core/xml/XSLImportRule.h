/*
 * This file is part of the XSL implementation.
 *
 * Copyright (C) 2004, 2006, 2008 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef XSLImportRule_h
#define XSLImportRule_h

#include "RuntimeEnabledFeatures.h"
#include "core/fetch/ResourcePtr.h"
#include "core/fetch/StyleSheetResourceClient.h"
#include "core/xml/XSLStyleSheet.h"
#include "wtf/PassOwnPtr.h"

namespace WebCore {

class XSLStyleSheetResource;

class XSLImportRule FINAL : private StyleSheetResourceClient {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static PassOwnPtr<XSLImportRule> create(XSLStyleSheet* parentSheet, const String& href)
    {
        ASSERT(RuntimeEnabledFeatures::xsltEnabled());
        return adoptPtr(new XSLImportRule(parentSheet, href));
    }

    virtual ~XSLImportRule();

    const String& href() const { return m_strHref; }
    XSLStyleSheet* styleSheet() const { return m_styleSheet.get(); }

    XSLStyleSheet* parentStyleSheet() const { return m_parentStyleSheet; }
    void setParentStyleSheet(XSLStyleSheet* styleSheet) { m_parentStyleSheet = styleSheet; }

    bool isLoading();
    void loadSheet();

private:
    XSLImportRule(XSLStyleSheet* parentSheet, const String& href);

    virtual void setXSLStyleSheet(const String& href, const KURL& baseURL, const String& sheet) OVERRIDE;

    // FIXME: Oilpan: This raw pointer is safe and has to be kept raw at this
    // point to avoid cycles. When XSLImportRule is moved to the oilpan heap
    // it should be a Member and be traced.
    XSLStyleSheet* m_parentStyleSheet;
    String m_strHref;
    RefPtrWillBePersistent<XSLStyleSheet> m_styleSheet;
    ResourcePtr<XSLStyleSheetResource> m_resource;
    bool m_loading;
};

} // namespace WebCore

#endif // XSLImportRule_h
