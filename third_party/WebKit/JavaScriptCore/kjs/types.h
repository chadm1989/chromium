// -*- c-basic-offset: 2 -*-
/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
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
 *  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *  Boston, MA 02111-1307, USA.
 *
 */

#ifndef _KJS_TYPES_H_
#define _KJS_TYPES_H_

// internal data types

#include "value.h"
#include "reference.h"
#include "completion.h"

namespace KJS {

  class List;
  class ListIterator;
  class ListNode;
  class ListHookNode;

  /**
   * @short Iterator for @ref KJS::List objects.
   */
  class ListIterator {
    friend class List;
    ListIterator();
    ListIterator(ListNode *n);
  public:
    /**
     * Construct an iterator that points to the first element of the list.
     * @param l The list the iterator will operate on.
     */
    ListIterator(const List &l);
    /**
     * Dereference the iterator.
     * @return A pointer to the element the iterator operates on.
     */
    ValueImp* operator->() const;
    //    operator Value* () const { return node->member; }
    Value operator*() const;
    /**
     * Conversion to @ref KJS::Value*
     * @return A pointer to the element the iterator operates on.
     */
    //    operator Value*() const { return node->member; }
    /**
     * Postfix increment operator.
     * @return The element after the increment.
     */
    Value operator++();
    /**
     * Prefix increment operator.
     */
    Value operator++(int);
    /**
     * Postfix decrement operator.
     */
    Value operator--();
    /**
     * Prefix decrement operator.
     */
    Value operator--(int);
    /**
     * Compare the iterator with another one.
     * @return True if the two iterators operate on the same list element.
     * False otherwise.
     */
    bool operator==(const ListIterator &it) const;
    /**
     * Check for inequality with another iterator.
     * @return True if the two iterators operate on different list elements.
     */
    bool operator!=(const ListIterator &it) const;
  private:
    ListNode *node;
  };

  /**
   * @short Native list type.
   *
   * List is a native ECMAScript type. List values are only used for
   * intermediate results of expression evaluation and cannot be stored
   * as properties of objects.
   *
   * The list is explicitly shared. Note that while copy() returns a
   * copy of the list the referenced objects are still shared.
   */
  class List {
    friend class ListIterator;
  public:
    List(bool needsMarking = false);
    List(const List& l);
    List &operator=(const List& l);
      
    ~List();

    /**
     * Append an object to the end of the list.
     *
     * @param val Pointer to object.
     */
    void append(const Value& val);
    /**
     * Insert an object at the beginning of the list.
     *
     * @param val Pointer to object.
     */
    void prepend(const Value& val);
    /**
     * Appends the items of another list at the end of this one.
     */
    void appendList(const List& lst);
    /**
     * Prepend the items of another list to this one.
     * The first item of @p lst will become the first item of the list.
     */
    void prependList(const List& lst);
    /**
     * Remove the element at the beginning of the list.
     */
    void removeFirst();
    /**
     * Remove the element at the end of the list.
     */
    void removeLast();
    /*
     * Remove val from list.
     */
    void remove(const Value &val);
    /**
     * Remove all elements from the list.
     */
    void clear();
    /**
     * Returns a shallow copy of the list. Ownership is passed to the user
     * who is responsible for deleting the list then.
     */
    List copy() const;
    /**
     * @return A @ref KJS::ListIterator pointing to the first element.
     */
    ListIterator begin() const;
    /**
     * @return A @ref KJS::ListIterator pointing to the last element.
     */
    ListIterator end() const;
    /**
     * @return true if the list is empty. false otherwise.
     */
    bool isEmpty() const;
    /**
     * @return the current size of the list.
     */
    int size() const;
    /**
     * Retrieve an element at an indexed position. If you want to iterate
     * trough the whole list using @ref KJS::ListIterator will be faster.
     *
     * @param i List index.
     * @return Return the element at position i. @ref KJS::Undefined if the
     * index is out of range.
     */
    Value at(int i) const;
    /**
     * Equivalent to @ref at.
     */
    Value operator[](int i) const;

    /**
     * Returns a pointer to a static instance of an empty list. Useful if a
     * function has a @ref KJS::List parameter.
     */
    static const List empty();
#ifdef KJS_DEBUG_MEM
    static void globalClear();
#endif
    void mark();
  private:

    void erase(ListNode *n);
    void clearInternal();
    void refAll();
    void derefAll();
    void swap(List &other);

    ListHookNode *hook;
    bool m_needsMarking;
  };

}; // namespace

#endif // _KJS_TYPES_H_
