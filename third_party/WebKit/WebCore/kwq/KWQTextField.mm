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

#import <KWQNSTextField.h>

#import <qlineedit.h>
#import <KWQKHTMLPartImpl.h>

// KWQTextFieldCell is larger than a normal text field cell, so it includes
// the focus border as well as the rest of the text field.

@interface KWQTextFieldCell : NSTextFieldCell
{
}
@end

// KWQTextFieldFormatter enforces a maximum length.

@interface KWQTextFieldFormatter : NSFormatter
{
    int maxLength;
}

- (void)setMaximumLength:(int)len;
- (int)maximumLength;

@end

// KWQSecureTextField has two purposes.
// One is a workaround for bug 3024443.
// The other is hook up next and previous key views to KHTML.

@interface KWQSecureTextField : NSSecureTextField
{
    QWidget *widget;
    BOOL inSetFrameSize;
    BOOL inNextValidKeyView;
}

- initWithQWidget:(QWidget *)widget;

@end

@implementation KWQNSTextField

+ (void)initialize
{
    if (self == [KWQNSTextField class]) {
        [self setCellClass:[KWQTextFieldCell class]];
    }
}

- initWithFrame:(NSRect)frame
{
    [super initWithFrame:frame];
    
    formatter = [[KWQTextFieldFormatter alloc] init];
    [self setFormatter:formatter];

    [self setTarget:self];
    [self setAction:@selector(action:)];

    [self setDelegate:self];

    [[self cell] setScrollable:YES];
    
    return self;
}

- initWithQLineEdit:(QLineEdit *)w 
{
    widget = w;
    return [super init];
}

- (void)action:sender
{
    widget->returnPressed();
}

- (void)controlTextDidEndEditing:(NSNotification *)aNotification
{
    widget->textChanged();
}

- (void)dealloc
{
    [secureField release];
    [formatter release];
    [super dealloc];
}

- (KWQTextFieldFormatter *)formatter
{
    return formatter;
}

- (void)updateSecureFieldFrame
{
    [secureField setFrame:NSInsetRect([self bounds], FOCUS_BORDER_SIZE, FOCUS_BORDER_SIZE)];
}

- (void)setFrameSize:(NSSize)size
{
    [super setFrameSize:size];
    [self updateSecureFieldFrame];
}

- (void)setPasswordMode:(BOOL)flag
{
    if (!flag == ![secureField superview]) {
        return;
    }
    
    [self setStringValue:@""];
    if (!flag) {
        [secureField removeFromSuperview];
    } else {
        if (secureField == nil) {
            secureField = [[KWQSecureTextField alloc] initWithQWidget:widget];
            [secureField setDelegate:self];
            [secureField setFormatter:formatter];
            [secureField setFont:[self font]];
            [self updateSecureFieldFrame];
        }
        [self addSubview:secureField];
    }
    [self setStringValue:@""];
}

- (void)setEditable:(BOOL)flag
{
    [secureField setEditable:flag];
    [super setEditable:flag];
}

- (void)selectText:(id)sender
{
    if ([self passwordMode]) {
        [secureField selectText:sender];
        return;
    }
    [super selectText:sender];
}

- (BOOL)isEditable
{
    return [super isEditable];
}

- (BOOL)passwordMode
{
    return [secureField superview] != nil;
}

- (void)setMaximumLength:(int)len
{
    NSString *oldValue = [self stringValue];
    if ((int)[oldValue length] > len){
        [self setStringValue:[oldValue substringToIndex:len]];
    }
    [formatter setMaximumLength:len];
}

- (int)maximumLength
{
    return [formatter maximumLength];
}

- (BOOL)edited
{
    return edited;
}

- (void)setEdited:(BOOL)ed
{
    edited = ed;
}

- (void)textDidChange:(NSNotification *)aNotification
{
    edited = true;
}

- (NSString *)stringValue
{
    if ([secureField superview]) {
        return [secureField stringValue];
    }
    return [super stringValue];
}

- (void)setStringValue:(NSString *)string
{
    [secureField setStringValue:string];
    [super setStringValue:string];
}

- (void)setFont:(NSFont *)font
{
    [secureField setFont:font];
    [super setFont:font];
}

- (NSView *)nextKeyView
{
    return inNextValidKeyView
        ? KWQKHTMLPartImpl::nextKeyView(widget, KWQSelectingNext)
        : [super nextKeyView];
}

- (NSView *)previousKeyView
{
   return inNextValidKeyView
        ? KWQKHTMLPartImpl::nextKeyView(widget, KWQSelectingPrevious)
        : [super previousKeyView];
}

- (NSView *)nextValidKeyView
{
    inNextValidKeyView = YES;
    NSView *view = [super nextValidKeyView];
    inNextValidKeyView = NO;
    return view;
}

- (NSView *)previousValidKeyView
{
    inNextValidKeyView = YES;
    NSView *view = [super previousValidKeyView];
    inNextValidKeyView = NO;
    return view;
}

@end

// This cell is used so that our frame includes the place where the focus rectangle is drawn.
// We account for this at the QWidget level so the placement of the text field is still correct,
// just as we account for the margins in NSButton and other AppKit controls.

@implementation KWQTextFieldCell

- (BOOL)isOpaque
{
    return NO;
}

- (NSSize)cellSizeForBounds:(NSRect)bounds
{
    NSSize size = [super cellSizeForBounds:bounds];
    size.width += FOCUS_BORDER_SIZE * 2;
    size.height += FOCUS_BORDER_SIZE * 2;
    return size;
}

- (void)drawWithFrame:(NSRect)frame inView:(NSView *)view
{
    [super drawWithFrame:NSInsetRect(frame, FOCUS_BORDER_SIZE, FOCUS_BORDER_SIZE) inView:view];
}

- (void)editWithFrame:(NSRect)frame inView:(NSView *)view editor:(NSText *)editor delegate:(id)delegate event:(NSEvent *)event
{
    [super editWithFrame:NSInsetRect(frame, FOCUS_BORDER_SIZE, FOCUS_BORDER_SIZE) inView:view editor:editor delegate:delegate event:event];
}

- (void)selectWithFrame:(NSRect)frame inView:(NSView *)view editor:(NSText *)editor delegate:(id)delegate start:(int)start length:(int)length
{
    [super selectWithFrame:NSInsetRect(frame, FOCUS_BORDER_SIZE, FOCUS_BORDER_SIZE) inView:view editor:editor delegate:delegate start:start length:length];
}

@end

@implementation KWQTextFieldFormatter

- init
{
    [super init];
    maxLength = INT_MAX;
    return self;
}

- (void)setMaximumLength:(int)len
{
    maxLength = len;
}

- (int)maximumLength
{
    return maxLength;
}

- (NSString *)stringForObjectValue:(id)object
{
    return (NSString *)object;
}

- (BOOL)getObjectValue:(id *)object forString:(NSString *)string errorDescription:(NSString **)error
{
    *object = string;
    return YES;
}

- (BOOL)isPartialStringValid:(NSString *)partialString newEditingString:(NSString **)newString errorDescription:(NSString **)error
{
    if ((int)[partialString length] > maxLength) {
        *newString = nil;
        return NO;
    }

    return YES;
}

- (NSAttributedString *)attributedStringForObjectValue:(id)anObject withDefaultAttributes:(NSDictionary *)attributes
{
    return nil;
}

@end

@implementation KWQSecureTextField

- initWithQWidget:(QWidget *)w
{
    widget = w;
    return [super init];
}

- (NSView *)nextKeyView
{
    return inNextValidKeyView
        ? KWQKHTMLPartImpl::nextKeyView(widget, KWQSelectingNext)
        : [super nextKeyView];
}

- (NSView *)previousKeyView
{
   return inNextValidKeyView
        ? KWQKHTMLPartImpl::nextKeyView(widget, KWQSelectingPrevious)
        : [super previousKeyView];
}

- (NSView *)nextValidKeyView
{
    inNextValidKeyView = YES;
    NSView *view = [super nextValidKeyView];
    inNextValidKeyView = NO;
    return view;
}

- (NSView *)previousValidKeyView
{
    inNextValidKeyView = YES;
    NSView *view = [super previousValidKeyView];
    inNextValidKeyView = NO;
    return view;
}

// These next two methods are the workaround for bug 3024443.
// Basically, setFrameSize ends up calling an inappropriate selectText, so we just ignore
// calls to selectText while setFrameSize is running.

- (void)selectText:(id)sender
{
    if (sender == self && inSetFrameSize) {
        return;
    }
    [super selectText:sender];
}

- (void)setFrameSize:(NSSize)size
{
    inSetFrameSize = YES;
    [super setFrameSize:size];
    inSetFrameSize = NO;
}

@end
