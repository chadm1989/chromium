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

#include <kwqdebug.h>

unsigned int KWQ_LOG_LEVEL = KWQ_LOG_ALL;

void KWQSetLogLevel(int mask) {
    KWQ_LOG_LEVEL = mask;    
}

bool checkedDefault = 0;

unsigned int KWQGetLogLevel(){
    if (!checkedDefault){
        NSString *logLevelString = [[NSUserDefaults standardUserDefaults] objectForKey:@"WebKitLogLevel"];
        if (logLevelString != nil){
            if (![[NSScanner scannerWithString: logLevelString] scanHexInt: &KWQ_LOG_LEVEL]){
                NSLog (@"Unable to scan hex value for WebKitLogLevel, default to value of %d", KWQ_LOG_LEVEL);
            }
        }
        checkedDefault = 1; 
    }
    return KWQ_LOG_LEVEL;
}


void KWQLog(NSString *format, ...) {    
    if (KWQGetLogLevel() & KWQ_LOG_ERROR){
        va_list args;
        va_start(args, format); 
        NSLogv(format, args);
        va_end(args);
    }
}


void KWQLogAtLevel(unsigned int level, NSString *format, ...) {    
    if (KWQGetLogLevel() & level){
        va_list args;
        va_start(args, format); 
        NSLogv(format, args);
        va_end(args);
    }
}


void KWQDebug(const char *format, ...) {    
    if (KWQGetLogLevel() & KWQ_LOG_DEBUG){
        va_list args;
        va_start(args, format); 
        vfprintf(stderr, format, args);
        va_end(args);
    }
}


void KWQDebugAtLevel(unsigned int level, const char *format, ...) {    
    if (KWQGetLogLevel() & level){
        va_list args;
        va_start(args, format); 
        vfprintf(stderr, format, args);
        va_end(args);
    }
}

