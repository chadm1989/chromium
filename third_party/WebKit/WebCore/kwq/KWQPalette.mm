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

#include <qpalette.h>

#include <kwqdebug.h>

QPalette::QPalette()
{
    _logNotYetImplemented();
}


QPalette::QPalette(const QPalette &)
{
    _logNotYetImplemented();
}


QPalette::~QPalette()
{
    _logNotYetImplemented();
}


void QPalette::setColor(ColorGroup, QColorGroup::ColorRole role, const QColor &color)
{
    _logNotYetImplemented();
}


const QColorGroup &QPalette::active() const
{
    _logNotYetImplemented();
    return data->active;
}


const QColorGroup &QPalette::inactive() const
{
    _logNotYetImplemented();
    return data->inactive;
}


const QColorGroup &QPalette::disabled() const
{
    _logNotYetImplemented();
    return data->disabled;
}


const QColorGroup &QPalette::normal() const
{
    _logNotYetImplemented();
    return data->normal;
}


QPalette &QPalette::operator=(const QPalette &p)
{
    _logNotYetImplemented();
    //p.data->ref();
    //if ( data->deref() )
    //    delete data;
    data = p.data;
    return *this;
}

