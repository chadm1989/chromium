/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef Lookup_h
#define Lookup_h

#include "CallFrame.h"
#include "Identifier.h"
#include "JSGlobalObject.h"
#include "JSObject.h"
#include "PropertySlot.h"
#include <stdio.h>
#include <wtf/Assertions.h>

namespace JSC {

    // Hash table generated by the create_hash_table script.
    struct HashTableValue {
        const char* key; // property name
        unsigned char attributes; // JSObject attributes
        intptr_t value1;
        intptr_t value2;
    };

    // FIXME: There is no reason this get function can't be simpler.
    // ie. typedef JSValue (*GetFunction)(ExecState*, JSObject* baseObject)
    typedef PropertySlot::GetValueFunc GetFunction;
    typedef void (*PutFunction)(ExecState*, JSObject* baseObject, JSValue value);

    class HashEntry {
    public:
        void initialize(UString::Rep* key, unsigned char attributes, intptr_t v1, intptr_t v2)
        {
            m_key = key;
            m_attributes = attributes;
            m_u.store.value1 = v1;
            m_u.store.value2 = v2;
            m_next = 0;
        }

        void setKey(UString::Rep* key) { m_key = key; }
        UString::Rep* key() const { return m_key; }

        unsigned char attributes() const { return m_attributes; }

        NativeFunction function() const { ASSERT(m_attributes & Function); return m_u.function.functionValue; }
        unsigned char functionLength() const { ASSERT(m_attributes & Function); return static_cast<unsigned char>(m_u.function.length); }

        GetFunction propertyGetter() const { ASSERT(!(m_attributes & Function)); return m_u.property.get; }
        PutFunction propertyPutter() const { ASSERT(!(m_attributes & Function)); return m_u.property.put; }

        intptr_t lexerValue() const { ASSERT(!m_attributes); return m_u.lexer.value; }

        void setNext(HashEntry *next) { m_next = next; }
        HashEntry* next() const { return m_next; }

    private:
        UString::Rep* m_key;
        unsigned char m_attributes; // JSObject attributes

        union {
            struct {
                intptr_t value1;
                intptr_t value2;
            } store;
            struct {
                NativeFunction functionValue;
                intptr_t length; // number of arguments for function
            } function;
            struct {
                GetFunction get;
                PutFunction put;
            } property;
            struct {
                intptr_t value;
                intptr_t unused;
            } lexer;
        } m_u;

        HashEntry* m_next;
    };

    struct HashTable {

        int compactSize;
        int compactHashSizeMask;

        const HashTableValue* values; // Fixed values generated by script.
        mutable const HashEntry* table; // Table allocated at runtime.

        ALWAYS_INLINE void initializeIfNeeded(JSGlobalData* globalData) const
        {
            if (!table)
                createTable(globalData);
        }

        ALWAYS_INLINE void initializeIfNeeded(ExecState* exec) const
        {
            if (!table)
                createTable(&exec->globalData());
        }

        void deleteTable() const;

        // Find an entry in the table, and return the entry.
        ALWAYS_INLINE const HashEntry* entry(JSGlobalData* globalData, const Identifier& identifier) const
        {
            initializeIfNeeded(globalData);
            return entry(identifier);
        }

        ALWAYS_INLINE const HashEntry* entry(ExecState* exec, const Identifier& identifier) const
        {
            initializeIfNeeded(exec);
            return entry(identifier);
        }

    private:
        ALWAYS_INLINE const HashEntry* entry(const Identifier& identifier) const
        {
            ASSERT(table);

            const HashEntry* entry = &table[identifier.ustring().rep()->computedHash() & compactHashSizeMask];

            if (!entry->key())
                return 0;

            do {
                if (entry->key() == identifier.ustring().rep())
                    return entry;
                entry = entry->next();
            } while (entry);

            return 0;
        }

        // Convert the hash table keys to identifiers.
        void createTable(JSGlobalData*) const;
    };

    void setUpStaticFunctionSlot(ExecState*, const HashEntry*, JSObject* thisObject, const Identifier& propertyName, PropertySlot&);

    /**
     * This method does it all (looking in the hashtable, checking for function
     * overrides, creating the function or retrieving from cache, calling
     * getValueProperty in case of a non-function property, forwarding to parent if
     * unknown property).
     */
    template <class ThisImp, class ParentImp>
    inline bool getStaticPropertySlot(ExecState* exec, const HashTable* table, ThisImp* thisObj, const Identifier& propertyName, PropertySlot& slot)
    {
        const HashEntry* entry = table->entry(exec, propertyName);

        if (!entry) // not found, forward to parent
            return thisObj->ParentImp::getOwnPropertySlot(exec, propertyName, slot);

        if (entry->attributes() & Function)
            setUpStaticFunctionSlot(exec, entry, thisObj, propertyName, slot);
        else
            slot.setCustom(thisObj, entry->propertyGetter());

        return true;
    }

    /**
     * Simplified version of getStaticPropertySlot in case there are only functions.
     * Using this instead of getStaticPropertySlot allows 'this' to avoid implementing
     * a dummy getValueProperty.
     */
    template <class ParentImp>
    inline bool getStaticFunctionSlot(ExecState* exec, const HashTable* table, JSObject* thisObj, const Identifier& propertyName, PropertySlot& slot)
    {
        if (static_cast<ParentImp*>(thisObj)->ParentImp::getOwnPropertySlot(exec, propertyName, slot))
            return true;

        const HashEntry* entry = table->entry(exec, propertyName);
        if (!entry)
            return false;

        setUpStaticFunctionSlot(exec, entry, thisObj, propertyName, slot);
        return true;
    }

    /**
     * Simplified version of getStaticPropertySlot in case there are no functions, only "values".
     * Using this instead of getStaticPropertySlot removes the need for a FuncImp class.
     */
    template <class ThisImp, class ParentImp>
    inline bool getStaticValueSlot(ExecState* exec, const HashTable* table, ThisImp* thisObj, const Identifier& propertyName, PropertySlot& slot)
    {
        const HashEntry* entry = table->entry(exec, propertyName);

        if (!entry) // not found, forward to parent
            return thisObj->ParentImp::getOwnPropertySlot(exec, propertyName, slot);

        ASSERT(!(entry->attributes() & Function));

        slot.setCustom(thisObj, entry->propertyGetter());
        return true;
    }

    /**
     * This one is for "put".
     * It looks up a hash entry for the property to be set.  If an entry
     * is found it sets the value and returns true, else it returns false.
     */
    template <class ThisImp>
    inline bool lookupPut(ExecState* exec, const Identifier& propertyName, JSValue value, const HashTable* table, ThisImp* thisObj)
    {
        const HashEntry* entry = table->entry(exec, propertyName);

        if (!entry)
            return false;

        if (entry->attributes() & Function) { // function: put as override property
            if (LIKELY(value.isCell()))
                thisObj->putDirectFunction(propertyName, value.asCell());
            else
                thisObj->putDirect(propertyName, value);
        } else if (!(entry->attributes() & ReadOnly))
            entry->propertyPutter()(exec, thisObj, value);

        return true;
    }

    /**
     * This one is for "put".
     * It calls lookupPut<ThisImp>() to set the value.  If that call
     * returns false (meaning no entry in the hash table was found),
     * then it calls put() on the ParentImp class.
     */
    template <class ThisImp, class ParentImp>
    inline void lookupPut(ExecState* exec, const Identifier& propertyName, JSValue value, const HashTable* table, ThisImp* thisObj, PutPropertySlot& slot)
    {
        if (!lookupPut<ThisImp>(exec, propertyName, value, table, thisObj))
            thisObj->ParentImp::put(exec, propertyName, value, slot); // not found: forward to parent
    }

} // namespace JSC

#endif // Lookup_h
