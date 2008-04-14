/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef JSInspectedObjectWrapper_h
#define JSInspectedObjectWrapper_h

#include "JSQuarantinedObjectWrapper.h"

namespace WebCore {

    class JSInspectedObjectWrapper : public JSQuarantinedObjectWrapper {
    public:
        static KJS::JSValue* wrap(KJS::ExecState* unwrappedExec, KJS::JSValue* unwrappedValue);

        virtual ~JSInspectedObjectWrapper();

        virtual const KJS::ClassInfo* classInfo() const { return &s_info; }
        static const KJS::ClassInfo s_info;

    protected:
        JSInspectedObjectWrapper(KJS::ExecState* unwrappedExec, KJS::JSObject* unwrappedObject, KJS::JSValue* wrappedPrototype);

        virtual bool allowsGetProperty() const { return true; }
        virtual bool allowsSetProperty() const { return true; }
        virtual bool allowsDeleteProperty() const { return true; }
        virtual bool allowsConstruct() const { return true; }
        virtual bool allowsHasInstance() const { return true; }
        virtual bool allowsCallAsFunction() const { return true; }
        virtual bool allowsGetPropertyNames() const { return true; }

        virtual KJS::JSValue* prepareIncomingValue(KJS::ExecState* unwrappedExec, KJS::JSValue* unwrappedValue) const;
        virtual KJS::JSValue* wrapOutgoingValue(KJS::ExecState* unwrappedExec, KJS::JSValue* unwrappedValue) const { return wrap(unwrappedExec, unwrappedValue); }
    };

} // namespace WebCore

#endif // JSInspectedObjectWrapper_h
