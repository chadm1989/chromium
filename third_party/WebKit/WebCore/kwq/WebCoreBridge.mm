/*
 * Copyright (C) 2002 Apple Computer, Inc.  All rights reserved.
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

#import "WebCoreBridge.h"

#import "dom_node.h"
#import "dom_docimpl.h"
#import "dom_nodeimpl.h"
#import "khtml_part.h"
#import "khtmlview.h"
#import "render_frames.h"
#import "render_image.h"
#import "render_object.h"
#import "render_root.h"
#import "render_style.h"

#import "KWQFont.h"

#import "KWQAssertions.h"
#import "html_documentimpl.h"
#import "dom_nodeimpl.h"
#import "htmlattrs.h"
#import "htmltags.h"
#import "csshelper.h"
#import "KWQDOMNode.h"
#import "WebCoreImageRenderer.h"
#import "WebCoreTextRendererFactory.h"
#import "KWQCharsets.h"
#import "KWQFrame.h"

#import "WebCoreDOMPrivate.h"

#import <kjs/property_map.h>

using KParts::URLArgs;

using DOM::DocumentImpl;

using khtml::parseURL;
using khtml::RenderImage;
using khtml::RenderObject;
using khtml::RenderPart;

using KJS::SavedProperties;

NSString *WebCoreElementFrameKey = 		@"WebElementFrame";
NSString *WebCoreElementImageAltStringKey = 	@"WebElementImageAltString";
NSString *WebCoreElementImageKey = 		@"WebElementImage";
NSString *WebCoreElementImageLocationKey = 	@"WebElementImageLocation";
NSString *WebCoreElementImageURLKey = 		@"WebElementImageURL";
NSString *WebCoreElementLinkURLKey = 		@"WebElementLinkURL";
NSString *WebCoreElementLinkTargetFrameKey =	@"WebElementTargetFrame";
NSString *WebCoreElementLinkLabelKey = 		@"WebElementLinkLabel";
NSString *WebCoreElementLinkTitleKey = 		@"WebElementLinkTitle";
NSString *WebCoreElementNameKey = 		@"WebElementName";
NSString *WebCoreElementStringKey = 		@"WebElementString";

@interface KWQPageState : NSObject
{
    DOM::DocumentImpl *document;
    KURL *URL;
    KJS::SavedProperties *windowProperties;
    KJS::SavedProperties *locationProperties;
}
- initWithDocument: (DOM::DocumentImpl *)doc URL: (KURL)u windowProperties: (KJS::SavedProperties *)wp locationProperties: (KJS::SavedProperties *)lp;
- (DOM::DocumentImpl *)document;
- (KURL *)URL;
- (KJS::SavedProperties *)windowProperties;
- (KJS::SavedProperties *)locationProperties;
@end

@implementation WebCoreBridge

- init
{
    [super init];
    
    _part = new KHTMLPart;
    _part->kwq->setBridge(self);
    
    return self;
}

- (void)dealloc
{
    [self removeFromFrame];
    
    if (_renderPart) {
        _renderPart->deref();
    }
    _part->kwq->setBridge(nil);
    _part->deref();
    
    [super dealloc];
}

- (KHTMLPart *)part
{
    return _part;
}

- (void)setRenderPart:(KHTMLRenderPart *)newPart;
{
    newPart->ref();
    if (_renderPart) {
        _renderPart->deref();
    }
    _renderPart = newPart;
}

- (KHTMLRenderPart *)renderPart
{
    return _renderPart;
}

- (void)setParent:(WebCoreBridge *)parent
{
    _part->setParent([parent part]);
}

- (void)openURL:(NSString *)URL reload:(BOOL)reload headers:(NSDictionary *)headers lastModified:(NSDate *)lastModified pageCache:(NSDictionary *)pageCache
{
    if (pageCache) {
        KWQPageState *state = [pageCache objectForKey:@"WebCorePageState"];
        _part->kwq->openURLFromPageCache([state document], [state URL], [state windowProperties], [state locationProperties]);
        return;
    }

    // arguments
    URLArgs args(_part->browserExtension()->urlArgs());
    args.reload = reload;
    NSString *contentType = [headers objectForKey:@"Content-Type"];
    if (contentType) {
        args.serviceType = QString::fromNSString(contentType);
    }
    _part->browserExtension()->setURLArgs(args);

    // opening the URL
    _part->didOpenURL([URL cString]);

    // things we have to set up after calling didOpenURL
    NSString *refreshHeader = [headers objectForKey:@"Refresh"];
    if (refreshHeader) {
        _part->kwq->addMetaData("http-refresh", QString::fromNSString(refreshHeader));
    }
    if (lastModified) {
	_part->kwq->addMetaData("modified", QString::fromNSString([lastModified description]));
    }
}

- (void)addData:(NSData *)data withEncoding:(NSString *)encoding
{
    _part->kwq->slotData(encoding, NO, (const char *)[data bytes], [data length], NO);
}

- (void)addData:(NSData *)data withOverrideEncoding:(NSString *)encoding
{
    _part->kwq->slotData(encoding, YES, (const char *)[data bytes], [data length], NO);
}

- (void)closeURL
{
    _part->closeURL();
}

- (void)saveDocumentState
{
    DocumentImpl *doc = _part->kwq->document();
    if (doc != 0){
        QStringList list = doc->docState();
        NSMutableArray *documentState = [[[NSMutableArray alloc] init] autorelease];
        
        for (uint i = 0; i < list.count(); i++){
            QString s = list[i];
            [documentState addObject: [NSString stringWithCharacters: (const unichar *)s.unicode() length: s.length()]];
        }
        [self saveDocumentState: documentState];
    }
}

- (void)restoreDocumentState
{
    DocumentImpl *doc = _part->kwq->document();
    
    if (doc != 0){
        NSArray *documentState = [self documentState];
        
        QStringList s;
        for (uint i = 0; i < [documentState count]; i++){
            NSString *string = [documentState objectAtIndex: i];
            s.append(QString::fromNSString(string));
        }
            
        doc->setRestoreState(s);
    }
}

- (void)scrollToAnchorWithURL:(NSString *)URL
{
    _part->kwq->scrollToAnchor([URL cString]);
}

- (BOOL)saveDocumentToPageCache
{
    DocumentImpl *doc = _part->kwq->document();
    if (doc != 0){
        KJS::SavedProperties *windowProperties = new KJS::SavedProperties();
        KJS::SavedProperties *locationProperties = new KJS::SavedProperties();

        _part->kwq->saveWindowProperties(windowProperties);
        _part->kwq->saveLocationProperties(locationProperties);
        KWQPageState *pageState = [[[KWQPageState alloc] initWithDocument: doc URL:_part->m_url windowProperties:windowProperties locationProperties:locationProperties] autorelease];
        return [self saveDocumentToPageCache: pageState];
    }
    return false;
}

- (BOOL)canCachePage
{
    return _part->kwq->canCachePage();
}

- (void)end
{
    _part->end();
}

- (void)createKHTMLViewWithNSView:(NSView *)view marginWidth:(int)mw marginHeight:(int)mh
{
    // If we own the view, delete the old one - otherwise the render _part will take care of deleting the view.
    [self removeFromFrame];

    KHTMLView *kview = new KHTMLView(_part, 0);
    _part->kwq->setView(kview, true);

    kview->setView(view);
    if (mw >= 0)
        kview->setMarginWidth(mw);
    if (mh >= 0)
        kview->setMarginHeight(mh);
}

- (void)scrollToAnchor:(NSString *)a
{
    _part->gotoAnchor(QString::fromNSString(a));
}

- (NSString *)selectedString
{
    return [[_part->selectedText().getNSString() copy] autorelease];
}

- (void)selectAll
{
    _part->selectAll();
}

- (void)deselectAll
{
    _part->kwq->document()->clearSelection();
}

- (BOOL)isFrameSet
{
    return _part->kwq->isFrameSet();
}

- (void)reapplyStyles
{
    return _part->reparseConfiguration();
}

- (void)forceLayout
{
    _part->kwq->forceLayout();
}

- (void)drawRect:(NSRect)rect withPainter:(QPainter *)p
{
    _part->kwq->paint(p, QRect(rect));
}

- (void)drawRect:(NSRect)rect
{
    QPainter p;
    [self drawRect:rect withPainter:&p];
}

- (void)adjustFrames:(NSRect)rect
{
    // Ick!  khtml sets the frame size during layout and
    // the frame origins during drawing!  So we have to 
    // layout and do a draw with rendering disabled to
    // correctly adjust the frames.
    [self forceLayout];
    QPainter p;
    p.setPaintingDisabled(YES);
    [self drawRect:rect withPainter:&p];
}

- (NSObject *)copyDOMNode:(NodeImpl *)node copier:(id <WebCoreDOMTreeCopier>)copier
{
    NSMutableArray *children = [[NSMutableArray alloc] init];
    for (NodeImpl *child = node->firstChild(); child; child = child->nextSibling()) {
        [children addObject:[self copyDOMNode:child copier:copier]];
    }
    NSObject *copiedNode = [copier nodeWithName:node->nodeName().string().getNSString()
                                          value:node->nodeValue().string().getNSString()
                                         source:node->recursive_toHTML(1).getNSString()
                                       children:children];
    [children release];
    return copiedNode;
}

- (NSObject *)copyDOMTree:(id <WebCoreDOMTreeCopier>)copier
{
    DocumentImpl *doc = _part->kwq->document();
    if (!doc) {
        return nil;
    }
    return [self copyDOMNode:doc copier:copier];
}

- (NSObject *)copyRenderNode:(RenderObject *)node copier:(id <WebCoreRenderTreeCopier>)copier
{
    NSMutableArray *children = [[NSMutableArray alloc] init];
    for (RenderObject *child = node->firstChild(); child; child = child->nextSibling()) {
        [children addObject:[self copyRenderNode:child copier:copier]];
    }
          
    NSString *name = [[NSString alloc] initWithUTF8String:node->renderName()];
    
    RenderPart *nodeRenderPart = dynamic_cast<RenderPart *>(node);
    QWidget *widget = nodeRenderPart ? nodeRenderPart->widget() : 0;
    NSView *view = widget ? widget->getView() : nil;
    
    int nx, ny;
    node->absolutePosition(nx,ny);
    NSObject *copiedNode = [copier nodeWithName:name
                                       position:NSMakePoint(nx,ny)
                                           rect:NSMakeRect(node->xPos(), node->yPos(), node->width(), node->height())
                                           view:view
                                       children:children];
    
    [name release];
    [children release];
    
    return copiedNode;
}

- (NSObject *)copyRenderTree:(id <WebCoreRenderTreeCopier>)copier
{
    RenderObject *renderer = _part->kwq->renderer();
    if (!renderer) {
        return nil;
    }
    return [self copyRenderNode:renderer copier:copier];
}

- (void)removeFromFrame
{
    _part->kwq->setView(0, false);
}

- (void)installInFrame:(NSView *)view
{
    // If this isn't the main frame, it must have a render _part set, or it
    // won't ever get installed in the view hierarchy.
    ASSERT(self == [self mainFrame] || _renderPart != nil);

    _part->kwq->view()->setView(view);
    if (_renderPart) {
        _renderPart->setWidget(_part->kwq->view());
        // Now the render part owns the view, so we don't any more.
        _part->kwq->setOwnsView(false);
    }
}

- (int)stateForEvent:(NSEvent *)event
{
    unsigned modifiers = [event modifierFlags];
    int state = 0;
    
    if (modifiers & NSControlKeyMask)
        state |= Qt::ControlButton;
    if (modifiers & NSShiftKeyMask)
        state |= Qt::ShiftButton;
    if (modifiers & NSAlternateKeyMask)
        state |= Qt::AltButton;
    
    // Mapping command to meta is slightly questionable, but it works for now.
    if (modifiers & NSCommandKeyMask)
        state |= Qt::MetaButton;
    
    return state;
}

- (void)mouseUp:(NSEvent *)event
{
    if (!_part->kwq->view()) {
        return;
    }
    
    NSPoint p = [event locationInWindow];

    int button, state;
    switch ([event type]) {
    case NSRightMouseUp:
        button = Qt::RightButton;
        state = Qt::RightButton;
        break;
    case NSOtherMouseUp:
        button = Qt::MidButton;
        state = Qt::MidButton;
        break;
    default:
        button = Qt::LeftButton;
        state = Qt::LeftButton;
        break;
    }
    state |= [self stateForEvent:event];
    
    int clickCount = [event clickCount];

    // Our behavior here is a little different that Qt.  Qt always sends
    // a mouse release event, even for a double click.  To correct problems
    // in khtml's DOM click event handling we do not send a release here
    // for a double click.  Instead we send that event from khtmlview's
    // viewportMouseDoubleClickEvent.
    if (clickCount > 0 && clickCount % 2 == 0) {
        QMouseEvent doubleClickEvent(QEvent::MouseButtonDblClick, QPoint(p), button, state, clickCount);
        _part->kwq->view()->viewportMouseDoubleClickEvent(&doubleClickEvent);
    }
    else {
        QMouseEvent releaseEvent(QEvent::MouseButtonRelease, QPoint(p), button, state, clickCount);
        _part->kwq->view()->viewportMouseReleaseEvent(&releaseEvent);
    }
}

- (void)mouseDown:(NSEvent *)event
{
    if (!_part->kwq->view()) {
        return;
    }
    
    NSPoint p = [event locationInWindow];
    
    int button, state;     
    switch ([event type]) {
    case NSRightMouseDown:
        button = Qt::RightButton;
        state = Qt::RightButton;
        break;
    case NSOtherMouseDown:
        button = Qt::MidButton;
        state = Qt::MidButton;
        break;
    default:
        button = Qt::LeftButton;
        state = Qt::LeftButton;
        break;
    }
    state |= [self stateForEvent:event];
    
    QMouseEvent kEvent(QEvent::MouseButtonPress, QPoint(p), button, state, [event clickCount]);
    _part->kwq->view()->viewportMousePressEvent(&kEvent);
}

- (void)mouseMoved:(NSEvent *)event
{
    if (!_part->kwq->view()) {
        return;
    }
    
    NSPoint p = [event locationInWindow];
    
    QMouseEvent kEvent(QEvent::MouseMove, QPoint(p), 0, [self stateForEvent:event]);
    _part->kwq->view()->viewportMouseMoveEvent(&kEvent);
}

- (void)mouseDragged:(NSEvent *)event
{
    if (!_part->kwq->view()) {
        return;
    }
    
    NSPoint p = [event locationInWindow];
    
    QMouseEvent kEvent(QEvent::MouseMove, QPoint(p), Qt::LeftButton, Qt::LeftButton);
    _part->kwq->view()->viewportMouseMoveEvent(&kEvent);
}

- (NSDictionary *)elementAtPoint:(NSPoint)point
{
    RenderObject *renderer = _part->kwq->renderer();
    if (!renderer) {
        return nil;
    }
    RenderObject::NodeInfo nodeInfo(true, true);
    renderer->layer()->nodeAtPoint(nodeInfo, (int)point.x, (int)point.y);
    
    NSMutableDictionary *element = [NSMutableDictionary dictionary];
    
    NodeImpl *URLNode = nodeInfo.URLElement();
    if (URLNode) {
        ElementImpl* e = static_cast<ElementImpl*>(URLNode);
        NSString *title = e->getAttribute(ATTR_TITLE).string().getNSString();
        if(title){
            [element setObject:title forKey:WebCoreElementLinkTitleKey];
        }
        
        NSString *URLString = _part->kwq->document()->completeURL(e->getAttribute(ATTR_HREF).string()).getNSString();
        
        if (URLString) {
            // Look for the first #text node to use as a label.
            NodeImpl *labelParent = e;
            while (labelParent->hasChildNodes()){
                NodeImpl *childNode = labelParent->firstChild();
                unsigned short type = childNode->nodeType();
                if (type == Node::TEXT_NODE){
                    DOMStringImpl *dv = childNode->nodeValue().implementation();
                    if (dv){
                        NSString *value = [NSString stringWithCharacters: (const unichar *)dv->s length: dv->l];
                        [element setObject:value forKey:WebCoreElementLinkLabelKey];
                        break;
                    }
                }
                labelParent = childNode;
            }
            [element setObject:URLString forKey:WebCoreElementLinkURLKey];
        }
        
        DOMString target = e->getAttribute(ATTR_TARGET);
        if (target.isEmpty() && _part->kwq->document()) {
            target = _part->kwq->document()->baseTarget();
        }
        if (!target.isEmpty()) {
            [element setObject:target.string().getNSString() forKey:WebCoreElementLinkTargetFrameKey];
        }
    }

    NodeImpl *node = nodeInfo.innerNonSharedNode();
    if (node && isImage(node)){

        ElementImpl* i =  static_cast<ElementImpl*>(node);
        DOMString attr = i->getAttribute(ATTR_SRC);
        if(attr.isEmpty()){
            // Look for the URL in the DATA attribute of the OBJECT tag.
            attr = i->getAttribute(ATTR_DATA);
        }

        NSString *URLString = _part->kwq->document()->completeURL(attr.string()).getNSString();        
        if (URLString) {
            [element setObject:URLString forKey:WebCoreElementImageURLKey];
            
            NSString *altString = i->getAttribute(ATTR_ALT).string().getNSString();
            if(altString){
                [element setObject:altString forKey:WebCoreElementImageAltStringKey];
            }
            
            RenderImage *r = (RenderImage *)node->renderer();
            id <WebCoreImageRenderer> image = r->pixmap().image();
            if (image) {
                [element setObject:image forKey:WebCoreElementImageKey];
                int x, y;
                if(r->absolutePosition(x, y)){
                    [element setObject:[NSValue valueWithPoint:NSMakePoint(x,y)] forKey:WebCoreElementImageLocationKey];
                }
            }
        }
    }

    if (_part->hasSelection()) {
        [element setObject:[self selectedString] forKey:WebCoreElementStringKey];
    }
    
    return element;
}

- (BOOL)searchFor:(NSString *)string direction:(BOOL)forward caseSensitive:(BOOL)caseFlag
{
    return _part->findTextNext(QString::fromNSString(string), forward, caseFlag, FALSE);
}

- (void)jumpToSelection
{
    _part->kwq->jumpToSelection();
}

- (void)setTextSizeMultiplier:(float)multiplier
{
    int newZoomFactor = (int)rint(multiplier * 100);
    if (_part->zoomFactor() == newZoomFactor) {
        return;
    }
    _part->setZoomFactor(newZoomFactor);
    // setZoomFactor will trigger a timed layout, but we want to do the layout before
    // we do any drawing. This takes care of that. Without this we redraw twice.
    [self setNeedsLayout];
}

- (CFStringEncoding)textEncoding
{
    return KWQCFStringEncodingFromIANACharsetName(_part->encoding().getCFString());
}

- (NSView *)nextKeyView
{
    return _part->kwq->nextKeyView(0, KWQSelectingNext);
}

- (NSView *)previousKeyView
{
    return _part->kwq->nextKeyView(0, KWQSelectingPrevious);
}

- (NSView *)nextKeyViewInsideWebViews
{
    return _part->kwq->nextKeyViewInFrameHierarchy(0, KWQSelectingNext);
}

- (NSView *)previousKeyViewInsideWebViews
{
    return _part->kwq->nextKeyViewInFrameHierarchy(0, KWQSelectingPrevious);
}

- (NSString *)stringByEvaluatingJavaScriptFromString:(NSString *)string
{
    _part->kwq->createDummyDocument();
    return _part->executeScript(QString::fromNSString(string)).asString().getNSString();
}

- (id<WebDOMDocument>)DOMDocument
{
    return [WebCoreDOMDocument documentWithImpl:_part->kwq->document()];
}

- (void)setSelectionFrom:(id<WebDOMNode>)start startOffset:(int)startOffset to:(id<WebDOMNode>)end endOffset:(int) endOffset
{
    _part->kwq->document()->setSelection([(WebCoreDOMNode *)start impl], startOffset, [(WebCoreDOMNode *)end impl], endOffset);
}

static NSAttributedString *attributedString(DOM::NodeImpl *_startNode, int startOffset, DOM::NodeImpl *endNode, int endOffset)
{
    bool hasNewLine = true;
    bool hasParagraphBreak = true;
    DOM::Node n = _startNode;
    khtml::RenderObject *renderer;
    NSFont *font;
    NSMutableAttributedString *result = [[[NSMutableAttributedString alloc] init] autorelease];
    NSAttributedString *partialString;

    while(!n.isNull()) {
        renderer = n.handle()->renderer();
        if (n.nodeType() == DOM::Node::TEXT_NODE && renderer) {
            QString text;
            QString str = n.nodeValue().string();
khtml::RenderStyle *style = 0;

            font = nil;
            style = renderer->style();
            if (style) {
                font = style->font().getNSFont();
            }

            if(n == _startNode && n == endNode && startOffset >=0 && endOffset >= 0)
                text = str.mid(startOffset, endOffset - startOffset);
            else if(n == _startNode && startOffset >= 0)
                text = str.mid(startOffset);
            else if(n == endNode && endOffset >= 0)
                text = str.left(endOffset);
            else
                text = str;

            text = text.stripWhiteSpace();
            if (text.length() > 1)
                text += ' ';

            if (text.length() > 0){
                hasNewLine = false;
                hasParagraphBreak = false;
                if (font){
                    NSMutableDictionary *attrs = [[[NSMutableDictionary alloc] init] autorelease];
                    [attrs setObject:font forKey:NSFontAttributeName];
                    if (style && style->color().isValid())
                        [attrs setObject:style->color().getNSColor() forKey:NSForegroundColorAttributeName];
                    if (style && style->backgroundColor().isValid())
                        [attrs setObject:style->backgroundColor().getNSColor() forKey:NSBackgroundColorAttributeName];
                    partialString = [[[NSAttributedString alloc] initWithString: text.getNSString() attributes: attrs] autorelease];
                }
                else
                    partialString = [[[NSAttributedString alloc] initWithString: text.getNSString() attributes: nil] autorelease];

                [result appendAttributedString: partialString];
            }
        }
        else if (renderer != 0){
            // This is our simple HTML -> ASCII transformation:
            QString text;
            unsigned short _id = n.elementId();
            switch(_id) {
                case ID_BR:
                    text += "\n";
                    hasNewLine = true;
                    break;

                case ID_TD:
                case ID_TH:
                case ID_HR:
                case ID_OL:
                case ID_UL:
                case ID_LI:
                case ID_DD:
                case ID_DL:
                case ID_DT:
                case ID_PRE:
                case ID_BLOCKQUOTE:
                case ID_DIV:
                    if (!hasNewLine)
                        text += "\n";
                    hasNewLine = true;
                    break;
                case ID_P:
                case ID_TR:
                case ID_H1:
                case ID_H2:
                case ID_H3:
                case ID_H4:
                case ID_H5:
                case ID_H6:
                    if (!hasNewLine)
                        text += "\n";
                    if (!hasParagraphBreak)
                        text += "\n";
                        hasParagraphBreak = true;
                    hasNewLine = true;
                    break;
            }
            partialString = [[[NSAttributedString alloc] initWithString: text.getNSString() attributes: nil] autorelease];
            [result appendAttributedString: partialString];
        }

        if(n == endNode)
            break;

DOM::Node next = n.firstChild();
        if(next.isNull())
            next = n.nextSibling();

        while( next.isNull() && !n.parentNode().isNull() ) {
            QString text;
            n = n.parentNode();
            next = n.nextSibling();

            unsigned short _id = n.elementId();
            switch(_id) {
                case ID_TD:
                case ID_TH:
                case ID_HR:
                case ID_OL:
                case ID_UL:
                case ID_LI:
                case ID_DD:
                case ID_DL:
                case ID_DT:
                case ID_PRE:
                case ID_BLOCKQUOTE:
                case ID_DIV:
                    if (!hasNewLine)
                        text += "\n";
                    hasNewLine = true;
                    break;
                case ID_P:
                case ID_TR:
                case ID_H1:
                case ID_H2:
                case ID_H3:
                case ID_H4:
                case ID_H5:
                case ID_H6:
                    if (!hasNewLine)
                        text += "\n";
                    // An extra newline is needed at the start, not the end, of these types of tags,
                    // so don't add another here.
                    hasNewLine = true;
                    break;
            }
            partialString = [[[NSAttributedString alloc] initWithString: text.getNSString() attributes: nil] autorelease];
            [result appendAttributedString: partialString];
        }

        n = next;
    }

    return result;
}

- (NSAttributedString *)selectedAttributedString
{
    return attributedString (_part->kwq->selectionStart(), _part->kwq->selectionStartOffset(), _part->kwq->selectionEnd(), _part->kwq->selectionEndOffset());
}

- (NSAttributedString *)attributedStringFrom: (id<WebDOMNode>)startNode startOffset: (int)startOffset to: (id<WebDOMNode>)endNode endOffset: (int)endOffset
{
    return attributedString ([(WebCoreDOMNode *)startNode impl], startOffset, [(WebCoreDOMNode *)endNode impl], endOffset);
}


- (id<WebDOMNode>)selectionStart
{
    return [WebCoreDOMNode nodeWithImpl: _part->kwq->selectionStart()];
}

- (int)selectionStartOffset
{
    return _part->kwq->selectionStartOffset();
}

- (id<WebDOMNode>)selectionEnd
{
    return [WebCoreDOMNode nodeWithImpl: _part->kwq->selectionEnd()];
}

- (int)selectionEndOffset
{
    return _part->kwq->selectionEndOffset();
}

- (void)setName:(NSString *)name
{
    _part->setName(QString::fromNSString(name));
}

- (NSString *)name
{
    return _part->name().getNSString();
}

- (NSString *)URL
{
    return _part->url().url().getNSString();
}

- (NSString *)referrer
{
    return _part->kwq->referrer().getNSString();
}

- (int)frameBorderStyle
{
    if (_part->kwq->view()->frameStyle() & QFrame::Sunken)
        return SunkenFrameBorder;
    if (_part->kwq->view()->frameStyle() & QFrame::Plain)
        return PlainFrameBorder;
    return NoFrameBorder;
}

+ (NSString *)stringWithData:(NSData *)data textEncoding:(CFStringEncoding)textEncoding
{
    QString string = QString::fromStringWithEncoding((const char*)[data bytes], [data length], textEncoding);
    return string.getNSString();
}

+ (NSString *)stringWithData:(NSData *)data textEncodingName:(NSString *)textEncodingName
{
    CFStringEncoding textEncoding = KWQCFStringEncodingFromIANACharsetName((CFStringRef)textEncodingName);
    return [WebCoreBridge stringWithData:data textEncoding:textEncoding];
}

- (BOOL)needsLayout
{
    RenderObject *renderer = _part->kwq->renderer();
    return renderer ? !renderer->layouted() : false;
}

- (BOOL)interceptKeyEvent:(NSEvent *)event toView:(NSView *)view
{
    return _part->kwq->keyEvent(event);
}

@end

@implementation KWQPageState
- initWithDocument: (DOM::DocumentImpl *)doc URL: (KURL)u windowProperties: (KJS::SavedProperties *)wp locationProperties: (KJS::SavedProperties *)lp
{
    [super init];
    doc->ref();
    document = doc;
    document->setInPageCache(YES);
    URL = new KURL(u);
    windowProperties = wp;
    locationProperties =lp;
    return self;
}

- (void)dealloc
{
    KHTMLView *view = document->view();
    document->setInPageCache(NO);
    document->detach();
    document->deref();
    document = 0;
    delete view;
    delete URL;
    URL = 0;
    delete windowProperties;
    delete locationProperties;
    [super dealloc];
}

- (DOM::DocumentImpl *)document
{
    return document;
}

- (KURL *)URL
{
    return URL;
}

- (KJS::SavedProperties *)windowProperties
{
    return windowProperties;
}

- (KJS::SavedProperties *)locationProperties
{
    return locationProperties;
}


@end

