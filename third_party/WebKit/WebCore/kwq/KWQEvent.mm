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

#import <qevent.h>
#import <kwqdebug.h>

QEvent::~QEvent()
{
}


QMouseEvent::QMouseEvent( Type t, const QPoint &pos, int b, int s )
    : QEvent(t), _position(pos), _button((ButtonState)b), _state((ButtonState)s)
{
}

QMouseEvent::QMouseEvent(Type t, const QPoint &pos, const QPoint &, int b, int s)
    : QEvent(t), _position(pos), _button((ButtonState)b), _state((ButtonState)s)
{
}

Qt::ButtonState QMouseEvent::stateAfter()
{
    return _state;
}


QTimerEvent::QTimerEvent(int t)
    : QEvent(Timer)
{
    _timerId = t;
}


QKeyEvent::QKeyEvent(Type t, int, int, int, const QString &, bool, ushort)
    : QEvent(t)
{
    _logNotYetImplemented();
}

int QKeyEvent::key() const
{
    _logNotYetImplemented();
    return 0;
}

Qt::ButtonState QKeyEvent::state() const
{
    _logNotYetImplemented();
    return Qt::NoButton;
}

void QKeyEvent::accept()
{
    _logNotYetImplemented();
}

void QKeyEvent::ignore()
{
    _logNotYetImplemented();
}

bool QKeyEvent::isAutoRepeat() const
{
    _logNotYetImplemented();
    return false;
}

QString QKeyEvent::text(void) const
{
    _logNotYetImplemented();
    return QString();
}

int QKeyEvent::ascii(void) const
{
    _logNotYetImplemented();
    return 0;
}

int QKeyEvent::count(void) const
{
    _logNotYetImplemented();
    return 0;
}

bool QKeyEvent::isAccepted(void) const
{
    _logNotYetImplemented();
    return false;
}
