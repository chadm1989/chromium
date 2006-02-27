/*
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 2004, 2005, 2006 Apple Computer, Inc.
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
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include "config.h"

#if AVOID_STATIC_CONSTRUCTORS
#define KHTML_ATOMICSTRING_HIDE_GLOBALS 1
#endif

#include "AtomicString.h"
#include "StaticConstructors.h"

#include <kxmlcore/HashSet.h>

namespace WebCore {
   
static HashSet<StringImpl*>* stringTable;

struct CStringTranslator 
{
    static unsigned hash(const char* c)
    {
        return StringImpl::computeHash(c);
    }

    static bool equal(StringImpl* r, const char* s)
    {
        int length = r->length();
        const QChar* d = r->unicode();
        for (int i = 0; i != length; ++i)
            if (d[i] != s[i])
                return false;
        return s[length] == 0;
    }

    static void translate(StringImpl*& location, const char* const& c, unsigned hash)
    {
        StringImpl* r = new StringImpl(c);
        r->_hash = hash;
        r->_inTable = true;
        location = r; 
    }
};

bool operator==(const AtomicString& a, const char* b)
{ 
    StringImpl* impl = a.impl();
    if ((!impl || !impl->s) && !b)
        return true;
    if ((!impl || !impl->s) || !b)
        return false;
    return CStringTranslator::equal(impl, b); 
}

StringImpl* AtomicString::add(const char* c)
{
    if (!c)
        return 0;
    int length = strlen(c);
    if (length == 0)
        return StringImpl::empty();
    
    return *stringTable->add<const char*, CStringTranslator>(c).first;
}

struct QCharBuffer {
    const QChar* s;
    uint length;
};

struct QCharBufferTranslator {
    static unsigned hash(const QCharBuffer& buf)
    {
        return StringImpl::computeHash(buf.s, buf.length);
    }

    static bool equal(StringImpl* const& str, const QCharBuffer& buf)
    {
        uint strLength = str->length();
        uint bufLength = buf.length;
        if (strLength != bufLength)
            return false;
        
        const uint32_t* strChars = reinterpret_cast<const uint32_t*>(str->unicode());
        const uint32_t* bufChars = reinterpret_cast<const uint32_t*>(buf.s);
        
        uint halfLength = strLength >> 1;
        for (uint i = 0; i != halfLength; ++i) {
            if (*strChars++ != *bufChars++)
                return false;
        }
        
        if (strLength & 1 && 
            *reinterpret_cast<const uint16_t *>(strChars) != *reinterpret_cast<const uint16_t *>(bufChars))
            return false;
        
        return true;
    }

    static void translate(StringImpl*& location, const QCharBuffer& buf, unsigned hash)
    {
        StringImpl *r = new StringImpl(buf.s, buf.length);
        r->_hash = hash;
        r->_inTable = true;
        
        location = r; 
    }
};

StringImpl* AtomicString::add(const QChar* s, int length)
{
    if (!s)
        return 0;

    if (length == 0)
        return StringImpl::empty();
    
    QCharBuffer buf = {s, length}; 
    return *stringTable->add<QCharBuffer, QCharBufferTranslator>(buf).first;
}

StringImpl* AtomicString::add(StringImpl* r)
{
    if (!r || r->_inTable)
        return r;

    if (r->length() == 0)
        return StringImpl::empty();
    
    StringImpl* result = *stringTable->add(r).first;
    if (result == r)
        r->_inTable = true;
    return result;
}

void AtomicString::remove(StringImpl* r)
{
    stringTable->remove(r);
}

#if AVOID_STATIC_CONSTRUCTORS
DEFINE_GLOBAL(AtomicString, nullAtom)
#else
const AtomicString nullAtom;
#endif
DEFINE_GLOBAL(AtomicString, emptyAtom, "")
DEFINE_GLOBAL(AtomicString, textAtom, "#text")
DEFINE_GLOBAL(AtomicString, commentAtom, "#comment")
DEFINE_GLOBAL(AtomicString, starAtom, "*")

void AtomicString::init()
{
#if AVOID_STATIC_CONSTRUCTORS
    static bool initialized;
    if (!initialized) {
        stringTable = new HashSet<StringImpl*>;

        // Use placement new to initialize the globals.
        new (&nullAtom) AtomicString;
        new (&emptyAtom) AtomicString("");
        new (&textAtom) AtomicString("#text");
        new (&commentAtom) AtomicString("#comment");
        new (&starAtom) AtomicString("*");

        initialized = true;
    }
#endif
}

}
