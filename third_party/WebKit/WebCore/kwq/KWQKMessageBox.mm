/*
 * Copyright (C) 2001 Apple Computer, Inc.  All rights reserved.
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
#import <Cocoa/Cocoa.h>

#include <kmessagebox.h>

void KMessageBox::error(QWidget *, const QString &message, 
    const QString &caption=QString::null, bool notify=true)
{
    if (caption.isNull())
        NSRunAlertPanel (nil, QSTRING_TO_NSSTRING(message), nil, nil, nil);
    else
        NSRunAlertPanel (QSTRING_TO_NSSTRING(caption), QSTRING_TO_NSSTRING(message), nil, nil, nil);
}


int KMessageBox::warningYesNo(QWidget *, const QString &message, 
    const QString &caption=QString::null, 
    const QString &buttonYes=QString::null, 
    const QString &buttonNo=QString::null, 
    bool notify=true)
{
    NSString *yes = buttonYes.isNull() ? nil : @"OK";
    NSString *no = buttonNo.isNull() ? nil : @"Cancel";
    int result;
    
    if (caption.isNull())
        result = NSRunAlertPanel (nil, QSTRING_TO_NSSTRING(message), yes, no, nil);
    else
        result = NSRunAlertPanel (QSTRING_TO_NSSTRING(caption), QSTRING_TO_NSSTRING(message), yes, no, nil);

    if (result == NSAlertDefaultReturn)
        return 1;
    return 0;
}


int KMessageBox::questionYesNo(QWidget *, const QString &message, 
    const QString &caption=QString::null, 
    const QString &buttonYes=QString::null, 
    const QString &buttonNo=QString::null, bool notify=true)
{
    NSString *yes = buttonYes.isNull() ? nil : @"OK";
    NSString *no = buttonNo.isNull() ? nil : @"Cancel";
    int result;
    
    if (caption.isNull())
        result = NSRunAlertPanel (nil, QSTRING_TO_NSSTRING(message), yes, no, nil);
    else
        result = NSRunAlertPanel (QSTRING_TO_NSSTRING(caption), QSTRING_TO_NSSTRING(message), yes, no, nil);

    if (result == NSAlertDefaultReturn)
        return 1;
    return 0;
}


void KMessageBox::sorry(QWidget *, const QString &message, 
    const QString &caption=QString::null, bool notify=true)
{
    if (caption.isNull())
        NSRunAlertPanel (nil, QSTRING_TO_NSSTRING(message), nil, nil, nil);
    else
        NSRunAlertPanel (QSTRING_TO_NSSTRING(caption), QSTRING_TO_NSSTRING(message), nil, nil, nil);
}




