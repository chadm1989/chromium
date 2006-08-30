/*
    Copyright (C) 2004, 2005 Nikolas Zimmermann <wildfox@kde.org>
                  2004, 2005 Rob Buis <buis@kde.org>
                  2005 Eric Seidel <eric.seidel@kdemail.net>
                  2006 Apple Computer, Inc

    This file is part of the KDE project

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    aint with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#ifndef KCanvasItem_H
#define KCanvasItem_H
#ifdef SVG_SUPPORT

#include "DeprecatedValueList.h"
#include "FloatRect.h"

#include "RenderObject.h"

namespace WebCore {

class FloatPoint;
class SVGStyledElement;

class Path;
class RenderSVGContainer;

class RenderPath : public RenderObject
{
public:
    RenderPath(RenderStyle *style, SVGStyledElement *node);
    virtual ~RenderPath();

    // Hit-detection seperated for the fill and the stroke
    virtual bool fillContains(const FloatPoint&, bool requiresFill = true) const;
    virtual bool strokeContains(const FloatPoint&, bool requiresStroke = true) const = 0;

    // Returns an unscaled bounding box (not even including localTransform()) for this vector path
    virtual FloatRect relativeBBox(bool includeStroke = true) const;

    void setPath(const Path& newPath);
    const Path& path() const;

    virtual bool isRenderPath() const { return true; }
    virtual const char *renderName() const { return "KCanvasItem"; }
    
    virtual AffineTransform localTransform() const;
    virtual void setLocalTransform(const AffineTransform &matrix);
    
    virtual void layout();
    virtual IntRect getAbsoluteRepaintRect();
    virtual bool requiresLayer();
    virtual short lineHeight(bool b, bool isRootLineBox = false) const;
    virtual short baselinePosition(bool b, bool isRootLineBox = false) const;
    virtual void paint(PaintInfo&, int parentX, int parentY);
 
    virtual void absoluteRects(DeprecatedValueList<IntRect>& rects, int tx, int ty);

    virtual bool nodeAtPoint(NodeInfo&, int x, int y, int tx, int ty, HitTestAction);
    
    // FIXME: When the other SVG classes get pointer-events support this should be moved elsewhere
    struct PointerEventsHitRules {
        PointerEventsHitRules()
        : requireVisible(false)
        , requireFill(false)
        , requireStroke(false)
        , canHitStroke(false)
        , canHitFill(false)
    {}
        
        bool requireVisible;
        bool requireFill;
        bool requireStroke;
        bool canHitStroke;
        bool canHitFill;  
    };

protected:
    virtual void drawMarkersIfNeeded(GraphicsContext*, const FloatRect&, const Path&) const = 0;
    virtual FloatRect strokeBBox() const = 0;

private:
    FloatPoint mapAbsolutePointToLocal(const FloatPoint&) const;
    
    PointerEventsHitRules pointerEventsHitRules();

    class Private;
    Private *d;
};

}

#endif // SVG_SUPPORT
#endif

// vim:ts=4:noet
