// -*- c-basic-offset: 2 -*-
/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003 Apple Computer, Inc.
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _KJS_DOM_H_
#define _KJS_DOM_H_

#include "kjs_binding.h"

#include <qvaluelist.h>
#include "misc/shared.h"

namespace DOM {
    class AttrImpl;
    class CharacterDataImpl;
    class DocumentTypeImpl;
    class DOMImplementationImpl;
    class ElementImpl;
    class EntityImpl;
    class NamedNodeMapImpl;
    class NodeImpl;
    class NodeListImpl;
    class NotationImpl;
    class ProcessingInstructionImpl;
    class TextImpl;
};

namespace KJS {

  class DOMNode : public DOMObject {
  public:
    DOMNode(ExecState *exec, DOM::NodeImpl *n);
    virtual ~DOMNode();
    virtual bool toBoolean(ExecState *) const;
    virtual Value tryGet(ExecState *exec, const Identifier &propertyName) const;
    Value getValueProperty(ExecState *exec, int token) const;
    virtual void mark();
    virtual void tryPut(ExecState *exec, const Identifier &propertyName, const Value& value, int attr = None);
    void putValue(ExecState *exec, int token, const Value& value, int attr);
    DOM::NodeImpl *impl() const { return m_impl.get(); }
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;

    virtual Value toPrimitive(ExecState *exec, Type preferred = UndefinedType) const;
    virtual UString toString(ExecState *exec) const;
    void setListener(ExecState *exec, int eventId, ValueImp *func) const;
    Value getListener(int eventId) const;
    virtual void pushEventHandlerScope(ExecState *exec, ScopeChain &scope) const;

    enum { NodeName, NodeValue, NodeType, ParentNode, ParentElement,
           ChildNodes, FirstChild, LastChild, PreviousSibling, NextSibling, Item,
           Attributes, NamespaceURI, Prefix, LocalName, OwnerDocument, InsertBefore,
           ReplaceChild, RemoveChild, AppendChild, HasAttributes, HasChildNodes,
           CloneNode, Normalize, IsSupported, AddEventListener, RemoveEventListener,
           DispatchEvent, Contains,
           OnAbort, OnBlur, OnChange, OnClick, OnContextMenu, OnDblClick, OnDragDrop, OnError,
           OnDragEnter, OnDragOver, OnDragLeave, OnDrop, OnDragStart, OnDrag, OnDragEnd,
           OnBeforeCut, OnCut, OnBeforeCopy, OnCopy, OnBeforePaste, OnPaste, OnSelectStart,
           OnFocus, OnInput, OnKeyDown, OnKeyPress, OnKeyUp, OnLoad, OnMouseDown,
           OnMouseMove, OnMouseOut, OnMouseOver, OnMouseUp, OnMouseWheel, OnMove, OnReset,
           OnResize, OnScroll, OnSearch, OnSelect, OnSubmit, OnUnload,
           OffsetLeft, OffsetTop, OffsetWidth, OffsetHeight, OffsetParent,
           ClientWidth, ClientHeight, ScrollLeft, ScrollTop, ScrollWidth, ScrollHeight };

  protected:
    // Constructor for inherited classes; doesn't set up a prototype.
    DOMNode(DOM::NodeImpl *n);
    khtml::SharedPtr<DOM::NodeImpl> m_impl;
  };

  DOM::NodeImpl *toNode(ValueImp *); // returns 0 if passed-in value is not a DOMNode object

  class DOMNodeList : public DOMObject {
  public:
    DOMNodeList(ExecState *, DOM::NodeListImpl *l) : m_impl(l) { }
    ~DOMNodeList();
    virtual bool hasOwnProperty(ExecState *exec, const Identifier &p) const;
    virtual Value tryGet(ExecState *exec, const Identifier &propertyName) const;
    virtual Value call(ExecState *exec, Object &thisObj, const List&args);
    virtual Value tryCall(ExecState *exec, Object &thisObj, const List&args);
    virtual bool implementsCall() const { return true; }
    // no put - all read-only
    virtual const ClassInfo* classInfo() const { return &info; }
    virtual bool toBoolean(ExecState *) const { return true; }
    static const ClassInfo info;
    DOM::NodeListImpl *impl() const { return m_impl.get(); }

    virtual Value toPrimitive(ExecState *exec, Type preferred = UndefinedType) const;

  private:
    khtml::SharedPtr<DOM::NodeListImpl> m_impl;
  };

  class DOMDocument : public DOMNode {
  public:
    DOMDocument(ExecState *exec, DOM::DocumentImpl *d);
    ~DOMDocument();
    virtual Value tryGet(ExecState *exec, const Identifier &propertyName) const;
    Value getValueProperty(ExecState *exec, int token) const;
    virtual void tryPut(ExecState *exec, const Identifier &propertyName, const Value& value, int attr = None);
    void putValue(ExecState *exec, int token, const Value& value, int attr);
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    enum { DocType, Implementation, DocumentElement,
           // Functions
           CreateElement, CreateDocumentFragment, CreateTextNode, CreateComment,
           CreateCDATASection, CreateProcessingInstruction, CreateAttribute,
           CreateEntityReference, GetElementsByTagName, ImportNode, CreateElementNS,
           CreateAttributeNS, GetElementsByTagNameNS, GetElementById,
           CreateRange, CreateNodeIterator, CreateTreeWalker, DefaultView,
           CreateEvent, ElementFromPoint, StyleSheets, PreferredStylesheetSet, 
           SelectedStylesheetSet, GetOverrideStyle, ReadyState, 
           ExecCommand, QueryCommandEnabled, QueryCommandIndeterm, QueryCommandState, 
           QueryCommandSupported, QueryCommandValue };

  protected:
    // Constructor for inherited classes; doesn't set up a prototype.
    DOMDocument(DOM::DocumentImpl *d);
  };

  class DOMAttr : public DOMNode {
  public:
    DOMAttr(ExecState *exec, DOM::AttrImpl *a);
    virtual Value tryGet(ExecState *exec, const Identifier &propertyName) const;
    virtual void tryPut(ExecState *exec, const Identifier &propertyName, const Value& value, int attr = None);
    Value getValueProperty(ExecState *exec, int token) const;
    void putValue(ExecState *exec, int token, const Value& value, int attr);
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    enum { Name, Specified, ValueProperty, OwnerElement };
  };

  DOM::AttrImpl *toAttr(ValueImp *); // returns 0 if passed-in value is not a DOMAttr object

  class DOMElement : public DOMNode {
  public:
    DOMElement(ExecState *exec, DOM::ElementImpl *e);
    virtual Value tryGet(ExecState *exec, const Identifier &propertyName) const;
    // no put - all read-only
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    enum { TagName, Style,
           GetAttribute, SetAttribute, RemoveAttribute, GetAttributeNode,
           SetAttributeNode, RemoveAttributeNode, GetElementsByTagName,
           GetAttributeNS, SetAttributeNS, RemoveAttributeNS, GetAttributeNodeNS,
           SetAttributeNodeNS, GetElementsByTagNameNS, HasAttribute, HasAttributeNS,
           ScrollByLines, ScrollByPages};
  protected:
    // Constructor for inherited classes; doesn't set up a prototype.
    DOMElement(DOM::ElementImpl *e);
  };

  DOM::ElementImpl *toElement(ValueImp *); // returns 0 if passed-in value is not a DOMElement object

  class DOMDOMImplementation : public DOMObject {
  public:
    // Build a DOMDOMImplementation
    DOMDOMImplementation(ExecState *, DOM::DOMImplementationImpl *i);
    ~DOMDOMImplementation();
    // no put - all functions
    virtual const ClassInfo* classInfo() const { return &info; }
    virtual bool toBoolean(ExecState *) const { return true; }
    static const ClassInfo info;
    enum { HasFeature, CreateDocumentType, CreateDocument, CreateCSSStyleSheet, CreateHTMLDocument };
    DOM::DOMImplementationImpl *impl() const { return m_impl.get(); }
  private:
    khtml::SharedPtr<DOM::DOMImplementationImpl> m_impl;
  };

  class DOMDocumentType : public DOMNode {
  public:
    DOMDocumentType(ExecState *exec, DOM::DocumentTypeImpl *dt);
    virtual Value tryGet(ExecState *exec, const Identifier &propertyName) const;
    Value getValueProperty(ExecState *exec, int token) const;
    // no put - all read-only
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    enum { Name, Entities, Notations, PublicId, SystemId, InternalSubset };
  };

  DOM::DocumentTypeImpl *toDocumentType(ValueImp *); // returns 0 if passed-in value is not a DOMDocumentType object

  class DOMNamedNodeMap : public DOMObject {
  public:
    DOMNamedNodeMap(ExecState *, DOM::NamedNodeMapImpl *m);
    ~DOMNamedNodeMap();
    virtual bool hasOwnProperty(ExecState *exec, const Identifier &p) const;
    virtual Value tryGet(ExecState *exec, const Identifier &propertyName) const;
    // no put - all read-only
    virtual const ClassInfo* classInfo() const { return &info; }
    virtual bool toBoolean(ExecState *) const { return true; }
    static const ClassInfo info;
    enum { GetNamedItem, SetNamedItem, RemoveNamedItem, Item,
           GetNamedItemNS, SetNamedItemNS, RemoveNamedItemNS };
    DOM::NamedNodeMapImpl *impl() const { return m_impl.get(); }
  private:
    khtml::SharedPtr<DOM::NamedNodeMapImpl> m_impl;
  };

  class DOMProcessingInstruction : public DOMNode {
  public:
    DOMProcessingInstruction(ExecState *exec, DOM::ProcessingInstructionImpl *pi);
    virtual Value tryGet(ExecState *exec, const Identifier &propertyName) const;
    Value getValueProperty(ExecState *exec, int token) const;
    virtual void tryPut(ExecState *exec, const Identifier &propertyName, const Value& value, int attr = None);
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    enum { Target, Data, Sheet };
  };

  class DOMNotation : public DOMNode {
  public:
    DOMNotation(ExecState *exec, DOM::NotationImpl *n);
    virtual Value tryGet(ExecState *exec, const Identifier &propertyName) const;
    Value getValueProperty(ExecState *exec, int token) const;
    // no put - all read-only
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    enum { PublicId, SystemId };
  };

  class DOMEntity : public DOMNode {
  public:
    DOMEntity(ExecState *exec, DOM::EntityImpl *e);
    virtual Value tryGet(ExecState *exec, const Identifier &propertyName) const;
    Value getValueProperty(ExecState *exec, int token) const;
    // no put - all read-only
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    enum { PublicId, SystemId, NotationName };
  };

  // Constructor for Node - constructor stuff not implemented yet
  class NodeConstructor : public DOMObject {
  public:
    NodeConstructor(ExecState *) { }
    virtual Value tryGet(ExecState *exec, const Identifier &propertyName) const;
    Value getValueProperty(ExecState *exec, int token) const;
    // no put - all read-only
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
  };

  // Constructor for DOMException - constructor stuff not implemented yet
  class DOMExceptionConstructor : public DOMObject {
  public:
    DOMExceptionConstructor(ExecState *) { }
    virtual Value tryGet(ExecState *exec, const Identifier &propertyName) const;
    Value getValueProperty(ExecState *exec, int token) const;
    // no put - all read-only
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
  };

  ValueImp *getDOMDocumentNode(ExecState *exec, DOM::DocumentImpl *n);
  bool checkNodeSecurity(ExecState *exec, DOM::NodeImpl *n);
  ValueImp *getRuntimeObject(ExecState *exec, DOM::NodeImpl *n);
  ValueImp *getDOMNode(ExecState *exec, DOM::NodeImpl *n);
  ValueImp *getDOMNamedNodeMap(ExecState *exec, DOM::NamedNodeMapImpl *m);
  ValueImp *getDOMNodeList(ExecState *exec, DOM::NodeListImpl *l);
  ValueImp *getDOMDOMImplementation(ExecState *exec, DOM::DOMImplementationImpl *i);
  ObjectImp *getNodeConstructor(ExecState *exec);
  ObjectImp *getDOMExceptionConstructor(ExecState *exec);

  // Internal class, used for the collection return by e.g. document.forms.myinput
  // when multiple nodes have the same name.
  class DOMNamedNodesCollection : public DOMObject {
  public:
    DOMNamedNodesCollection(ExecState *exec, const QValueList< khtml::SharedPtr<DOM::NodeImpl> >& nodes );
    virtual Value tryGet(ExecState *exec, const Identifier &propertyName) const;
  private:
    QValueList< khtml::SharedPtr<DOM::NodeImpl> > m_nodes;
  };

  class DOMCharacterData : public DOMNode {
  public:
    DOMCharacterData(ExecState *exec, DOM::CharacterDataImpl *d);
    virtual Value tryGet(ExecState *exec,const Identifier &propertyName) const;
    Value getValueProperty(ExecState *, int token) const;
    virtual void tryPut(ExecState *exec, const Identifier &propertyName, const Value& value, int attr = None);
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    DOM::CharacterDataImpl *toData() const;
    enum { Data, Length,
           SubstringData, AppendData, InsertData, DeleteData, ReplaceData };
  protected:
    // Constructor for inherited classes; doesn't set up a prototype.
    DOMCharacterData(DOM::CharacterDataImpl *d);
  };

  class DOMText : public DOMCharacterData {
  public:
    DOMText(ExecState *exec, DOM::TextImpl *t);
    virtual Value tryGet(ExecState *exec,const Identifier &propertyName) const;
    Value getValueProperty(ExecState *, int token) const;
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    DOM::TextImpl *toText() const;
    enum { SplitText };
  };

} // namespace

#endif
