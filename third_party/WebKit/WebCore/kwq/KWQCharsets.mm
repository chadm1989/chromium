/*
 * Copyright (C) 2003 Apple Computer, Inc.  All rights reserved.
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

#import "KWQCharsets.h"

struct CharsetEntry {
    const char *name;
    CFStringEncoding encoding;
    KWQEncodingFlags flags;
};

/* The following autogenerated file includes the charset data. */
#include "KWQCharsetData.c"

static Boolean encodingNamesEqual(const void *value1, const void *value2);
static CFHashCode encodingNameHash(const void *value);

static const CFDictionaryKeyCallBacks encodingNameKeyCallbacks = { 0, NULL, NULL, NULL, encodingNamesEqual, encodingNameHash };

static CFMutableDictionaryRef nameToTable = NULL;
static CFMutableDictionaryRef encodingToTable = NULL;

static void buildDictionaries()
{
    nameToTable = CFDictionaryCreateMutable(NULL, 0, &encodingNameKeyCallbacks, NULL);
    encodingToTable = CFDictionaryCreateMutable(NULL, 0, NULL, NULL);

    for (int i = 0; table[i].name != NULL; i++) {
        CFDictionaryAddValue(nameToTable, table[i].name, &table[i]);
        CFDictionaryAddValue(encodingToTable, reinterpret_cast<void *>(table[i].encoding), &table[i]);
    }
}

CFStringEncoding KWQCFStringEncodingFromIANACharsetName(const char *name, KWQEncodingFlags *flags)
{
    if (nameToTable == NULL) {
        buildDictionaries();
    }

    const void *value;
    if (!CFDictionaryGetValueIfPresent(nameToTable, name, &value)) {
        if (flags) {
            *flags = NoEncodingFlags;
        }
        return kCFStringEncodingInvalidId;
    }
    if (flags) {
        *flags = static_cast<const CharsetEntry *>(value)->flags;
    }
    return static_cast<const CharsetEntry *>(value)->encoding;
}

const char *KWQCFStringEncodingToIANACharsetName(CFStringEncoding encoding)
{
    if (encodingToTable == NULL) {
        buildDictionaries();
    }
    
    const void *value;
    if (!CFDictionaryGetValueIfPresent(encodingToTable, reinterpret_cast<void *>(encoding), &value)) {
        return NULL;
    }
    return static_cast<const CharsetEntry *>(value)->name;
}

static Boolean encodingNamesEqual(const void *value1, const void *value2)
{
    const char *s1 = static_cast<const char *>(value1);
    const char *s2 = static_cast<const char *>(value2);
    
    while (1) {
        char c1;
        do {
            c1 = *s1++;
        } while (c1 && !isalnum(c1));
        char c2;
        do {
            c2 = *s2++;
        } while (c2 && !isalnum(c2));
        
        if (tolower(c1) != tolower(c2)) {
            return false;
        }
        
        if (!c1 || !c2) {
            return !c1 && !c2;
        }
    }
}

// Golden ratio - arbitrary start value to avoid mapping all 0's to all 0's
// or anything like that.
const unsigned PHI = 0x9e3779b9U;

// This hash algorithm comes from:
// http://burtleburtle.net/bob/hash/hashfaq.html
// http://burtleburtle.net/bob/hash/doobs.html
static CFHashCode encodingNameHash(const void *value)
{
    const char *s = static_cast<const char *>(value);
    
    CFHashCode h = PHI;

    for (int i = 0; i != 16; ++i) {
        char c;
        do {
            c = *s++;
        } while (c && !isalnum(c));
        if (!c) {
            break;
        }
        h += tolower(c);
	h += (h << 10); 
	h ^= (h << 6); 
    }

    h += (h << 3);
    h ^= (h >> 11);
    h += (h << 15);
 
    return h;
}
