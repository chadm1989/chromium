// -*- c-basic-offset: 4 -*-
/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2006, 2007 Apple Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#ifndef Parser_h
#define Parser_h

#include <wtf/Forward.h>
#include <wtf/Noncopyable.h>
#include <wtf/RefPtr.h>
#include "nodes.h"

namespace KJS {

    class FunctionBodyNode;
    class ProgramNode;
    class UString;

    struct UChar;

    class Parser : Noncopyable {
    public:
        PassRefPtr<ProgramNode> parseProgram(const UString& sourceURL, int startingLineNumber,
            const UChar* code, unsigned length,
            int* sourceId = 0, int* errLine = 0, UString* errMsg = 0);

        PassRefPtr<FunctionBodyNode> parseFunctionBody(const UString& sourceURL, int startingLineNumber,
            const UChar* code, unsigned length,
            int* sourceId = 0, int* errLine = 0, UString* errMsg = 0);

        UString sourceURL() const { return m_sourceURL; }
        int sourceId() const { return m_sourceId; }

        void didFinishParsing(SourceElements* sourceElements, int lastLine)
        {
            m_sourceElements.set(sourceElements);
            m_lastLine = lastLine;
        }

    private:
        friend Parser& parser();

        Parser(); // Use parser() instead.
        void parse(int startingLineNumber, const UChar* code, unsigned length,
            int* sourceId, int* errLine, UString* errMsg);

        UString m_sourceURL;
        int m_sourceId;
        OwnPtr<SourceElements> m_sourceElements;
        int m_lastLine;
    };
    
    Parser& parser(); // Returns the singleton JavaScript parser.

} // namespace KJS

#endif // Parser_h
