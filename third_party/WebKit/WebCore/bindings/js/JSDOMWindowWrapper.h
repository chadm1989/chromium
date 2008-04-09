/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef JSDOMWindowWrapper_h
#define JSDOMWindowWrapper_h

#include "JSDOMWindow.h"
#include <kjs/object.h>

namespace WebCore {

    class Frame;

    class JSDOMWindowWrapper : public KJS::JSObject {
        typedef KJS::JSObject Base;
    public:
        JSDOMWindowWrapper();
        virtual ~JSDOMWindowWrapper();

        JSDOMWindow* window() const { return m_window; }
        void setWindow(JSDOMWindow* window)
        {
            ASSERT_ARG(window, window);
            m_window = window;
            setPrototype(window->prototype());
        }

        static const KJS::ClassInfo s_info;
        virtual const KJS::ClassInfo* classInfo() const { return &s_info; }

        // Forwarded methods

        // JSObject methods
        virtual void mark();
        virtual KJS::UString className() const;
        virtual bool getOwnPropertySlot(KJS::ExecState*, const KJS::Identifier& propertyName, KJS::PropertySlot&);
        virtual void put(KJS::ExecState*, const KJS::Identifier& propertyName, KJS::JSValue*);
        virtual bool deleteProperty(KJS::ExecState*, const KJS::Identifier& propertyName);
        virtual void getPropertyNames(KJS::ExecState*, KJS::PropertyNameArray&);
        virtual bool getPropertyAttributes(const KJS::Identifier& propertyName, unsigned& attributes) const;
        virtual void defineGetter(KJS::ExecState*, const KJS::Identifier& propertyName, KJS::JSObject* getterFunction);
        virtual void defineSetter(KJS::ExecState*, const KJS::Identifier& propertyName, KJS::JSObject* setterFunction);
        virtual KJS::JSValue* lookupGetter(KJS::ExecState*, const KJS::Identifier& propertyName);
        virtual KJS::JSValue* lookupSetter(KJS::ExecState*, const KJS::Identifier& propertyName);
        virtual KJS::JSGlobalObject* toGlobalObject(KJS::ExecState*) const;

        // JSDOMWindow methods
        DOMWindow* impl() const;
        void disconnectFrame();
        void clear();

    private:
        JSDOMWindow* m_window;
    };

    KJS::JSValue* toJS(KJS::ExecState*, Frame*);
    JSDOMWindowWrapper* toJSDOMWindowWrapper(Frame*);

} // namespace WebCore

#endif // JSDOMWindowWrapper_h
