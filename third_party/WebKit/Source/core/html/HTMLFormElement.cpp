/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 *           (C) 2006 Alexey Proskuryakov (ap@nypop.com)
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
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "core/html/HTMLFormElement.h"

#include <limits>
#include "HTMLNames.h"
#include "bindings/v8/ScriptController.h"
#include "bindings/v8/ScriptEventListener.h"
#include "core/dom/Attribute.h"
#include "core/dom/Document.h"
#include "core/dom/ElementTraversal.h"
#include "core/events/AutocompleteErrorEvent.h"
#include "core/events/Event.h"
#include "core/events/ScopedEventQueue.h"
#include "core/events/ThreadLocalEventNames.h"
#include "core/html/HTMLCollection.h"
#include "core/html/HTMLDialogElement.h"
#include "core/html/HTMLImageElement.h"
#include "core/html/HTMLInputElement.h"
#include "core/html/HTMLObjectElement.h"
#include "core/html/RadioNodeList.h"
#include "core/html/forms/FormController.h"
#include "core/loader/FrameLoader.h"
#include "core/loader/FrameLoaderClient.h"
#include "core/frame/ContentSecurityPolicy.h"
#include "core/frame/DOMWindow.h"
#include "core/frame/Frame.h"
#include "core/frame/UseCounter.h"
#include "core/rendering/RenderTextControl.h"
#include "platform/UserGestureIndicator.h"

using namespace std;

namespace WebCore {

using namespace HTMLNames;

HTMLFormElement::HTMLFormElement(Document& document)
    : HTMLElement(formTag, document)
    , m_associatedElementsBeforeIndex(0)
    , m_associatedElementsAfterIndex(0)
    , m_wasUserSubmitted(false)
    , m_isSubmittingOrPreparingForSubmission(false)
    , m_shouldSubmit(false)
    , m_isInResetFunction(false)
    , m_wasDemoted(false)
    , m_requestAutocompleteTimer(this, &HTMLFormElement::requestAutocompleteTimerFired)
{
    ScriptWrappable::init(this);
}

PassRefPtr<HTMLFormElement> HTMLFormElement::create(Document& document)
{
    UseCounter::count(document, UseCounter::FormElement);
    return adoptRef(new HTMLFormElement(document));
}

HTMLFormElement::~HTMLFormElement()
{
    document().formController()->willDeleteForm(this);

    for (unsigned i = 0; i < m_associatedElements.size(); ++i)
        m_associatedElements[i]->formWillBeDestroyed();
    for (unsigned i = 0; i < m_imageElements.size(); ++i)
        m_imageElements[i]->m_form = 0;
}

bool HTMLFormElement::formWouldHaveSecureSubmission(const String& url)
{
    return treeScope().completeURL(url).protocolIs("https");
}

bool HTMLFormElement::rendererIsNeeded(const RenderStyle& style)
{
    if (!m_wasDemoted)
        return HTMLElement::rendererIsNeeded(style);

    ContainerNode* node = parentNode();
    RenderObject* parentRenderer = node->renderer();
    // FIXME: Shouldn't we also check for table caption (see |formIsTablePart| below).
    // FIXME: This check is not correct for Shadow DOM.
    bool parentIsTableElementPart = (parentRenderer->isTable() && node->hasTagName(tableTag))
        || (parentRenderer->isTableRow() && node->hasTagName(trTag))
        || (parentRenderer->isTableSection() && node->hasTagName(tbodyTag))
        || (parentRenderer->isRenderTableCol() && node->hasTagName(colTag))
        || (parentRenderer->isTableCell() && node->hasTagName(trTag));

    if (!parentIsTableElementPart)
        return true;

    EDisplay display = style.display();
    bool formIsTablePart = display == TABLE || display == INLINE_TABLE || display == TABLE_ROW_GROUP
        || display == TABLE_HEADER_GROUP || display == TABLE_FOOTER_GROUP || display == TABLE_ROW
        || display == TABLE_COLUMN_GROUP || display == TABLE_COLUMN || display == TABLE_CELL
        || display == TABLE_CAPTION;

    return formIsTablePart;
}

Node::InsertionNotificationRequest HTMLFormElement::insertedInto(ContainerNode* insertionPoint)
{
    HTMLElement::insertedInto(insertionPoint);
    if (insertionPoint->inDocument())
        this->document().didAssociateFormControl(this);
    return InsertionDone;
}

static inline Node* findRoot(Node* n)
{
    Node* root = n;
    for (; n; n = n->parentNode())
        root = n;
    return root;
}

void HTMLFormElement::removedFrom(ContainerNode* insertionPoint)
{
    Node* root = findRoot(this);
    Vector<FormAssociatedElement*> associatedElements(m_associatedElements);
    for (unsigned i = 0; i < associatedElements.size(); ++i)
        associatedElements[i]->formRemovedFromTree(root);
    HTMLElement::removedFrom(insertionPoint);
}

void HTMLFormElement::handleLocalEvents(Event* event)
{
    Node* targetNode = event->target()->toNode();
    if (event->eventPhase() != Event::CAPTURING_PHASE && targetNode && targetNode != this && (event->type() == EventTypeNames::submit || event->type() == EventTypeNames::reset)) {
        event->stopPropagation();
        return;
    }
    HTMLElement::handleLocalEvents(event);
}

unsigned HTMLFormElement::length() const
{
    unsigned len = 0;
    for (unsigned i = 0; i < m_associatedElements.size(); ++i)
        if (m_associatedElements[i]->isEnumeratable())
            ++len;
    return len;
}

Node* HTMLFormElement::item(unsigned index)
{
    return elements()->item(index);
}

void HTMLFormElement::submitImplicitly(Event* event, bool fromImplicitSubmissionTrigger)
{
    int submissionTriggerCount = 0;
    bool seenDefaultButton = false;
    for (unsigned i = 0; i < m_associatedElements.size(); ++i) {
        FormAssociatedElement* formAssociatedElement = m_associatedElements[i];
        if (!formAssociatedElement->isFormControlElement())
            continue;
        HTMLFormControlElement* control = toHTMLFormControlElement(formAssociatedElement);
        if (!seenDefaultButton && control->canBeSuccessfulSubmitButton()) {
            if (fromImplicitSubmissionTrigger)
                seenDefaultButton = true;
            if (control->isSuccessfulSubmitButton()) {
                if (control->renderer()) {
                    control->dispatchSimulatedClick(event);
                    return;
                }
            } else if (fromImplicitSubmissionTrigger) {
                // Default (submit) button is not activated; no implicit submission.
                return;
            }
        } else if (control->canTriggerImplicitSubmission()) {
            ++submissionTriggerCount;
        }
    }
    if (fromImplicitSubmissionTrigger && submissionTriggerCount == 1)
        prepareForSubmission(event);
}

// FIXME: Consolidate this and similar code in FormSubmission.cpp.
static inline HTMLFormControlElement* submitElementFromEvent(const Event* event)
{
    for (Node* node = event->target()->toNode(); node; node = node->parentOrShadowHostNode()) {
        if (node->isElementNode() && toElement(node)->isFormControlElement())
            return toHTMLFormControlElement(node);
    }
    return 0;
}

bool HTMLFormElement::validateInteractively(Event* event)
{
    ASSERT(event);
    if (!document().page() || noValidate())
        return true;

    HTMLFormControlElement* submitElement = submitElementFromEvent(event);
    if (submitElement && submitElement->formNoValidate())
        return true;

    for (unsigned i = 0; i < m_associatedElements.size(); ++i) {
        if (m_associatedElements[i]->isFormControlElement())
            toHTMLFormControlElement(m_associatedElements[i])->hideVisibleValidationMessage();
    }

    Vector<RefPtr<FormAssociatedElement> > unhandledInvalidControls;
    if (!checkInvalidControlsAndCollectUnhandled(&unhandledInvalidControls))
        return true;
    // Because the form has invalid controls, we abort the form submission and
    // show a validation message on a focusable form control.

    // Needs to update layout now because we'd like to call isFocusable(), which
    // has !renderer()->needsLayout() assertion.
    document().updateLayoutIgnorePendingStylesheets();

    RefPtr<HTMLFormElement> protector(this);
    // Focus on the first focusable control and show a validation message.
    for (unsigned i = 0; i < unhandledInvalidControls.size(); ++i) {
        FormAssociatedElement* unhandledAssociatedElement = unhandledInvalidControls[i].get();
        HTMLElement* unhandled = toHTMLElement(unhandledAssociatedElement);
        if (unhandled->isFocusable() && unhandled->inDocument()) {
            unhandled->scrollIntoViewIfNeeded(false);
            unhandled->focus();
            if (unhandled->isFormControlElement())
                toHTMLFormControlElement(unhandled)->updateVisibleValidationMessage();
            break;
        }
    }
    // Warn about all of unfocusable controls.
    if (document().frame()) {
        for (unsigned i = 0; i < unhandledInvalidControls.size(); ++i) {
            FormAssociatedElement* unhandledAssociatedElement = unhandledInvalidControls[i].get();
            HTMLElement* unhandled = toHTMLElement(unhandledAssociatedElement);
            if (unhandled->isFocusable() && unhandled->inDocument())
                continue;
            String message("An invalid form control with name='%name' is not focusable.");
            message.replace("%name", unhandledAssociatedElement->name());
            document().addConsoleMessage(RenderingMessageSource, ErrorMessageLevel, message);
        }
    }
    return false;
}

bool HTMLFormElement::prepareForSubmission(Event* event)
{
    RefPtr<HTMLFormElement> protector(this);
    Frame* frame = document().frame();
    if (m_isSubmittingOrPreparingForSubmission || !frame)
        return m_isSubmittingOrPreparingForSubmission;

    m_isSubmittingOrPreparingForSubmission = true;
    m_shouldSubmit = false;

    // Interactive validation must be done before dispatching the submit event.
    if (!validateInteractively(event)) {
        m_isSubmittingOrPreparingForSubmission = false;
        return false;
    }

    StringPairVector controlNamesAndValues;
    getTextFieldValues(controlNamesAndValues);
    RefPtr<FormState> formState = FormState::create(this, controlNamesAndValues, &document(), NotSubmittedByJavaScript);
    frame->loader().client()->dispatchWillSendSubmitEvent(formState.release());

    if (dispatchEvent(Event::createCancelableBubble(EventTypeNames::submit)))
        m_shouldSubmit = true;

    m_isSubmittingOrPreparingForSubmission = false;

    if (m_shouldSubmit)
        submit(event, true, true, NotSubmittedByJavaScript);

    return m_shouldSubmit;
}

void HTMLFormElement::submit()
{
    submit(0, false, true, NotSubmittedByJavaScript);
}

void HTMLFormElement::submitFromJavaScript()
{
    submit(0, false, UserGestureIndicator::processingUserGesture(), SubmittedByJavaScript);
}

void HTMLFormElement::getTextFieldValues(StringPairVector& fieldNamesAndValues) const
{
    ASSERT_ARG(fieldNamesAndValues, fieldNamesAndValues.isEmpty());

    fieldNamesAndValues.reserveCapacity(m_associatedElements.size());
    for (unsigned i = 0; i < m_associatedElements.size(); ++i) {
        FormAssociatedElement* control = m_associatedElements[i];
        HTMLElement* element = toHTMLElement(control);
        if (!element->hasTagName(inputTag))
            continue;

        HTMLInputElement* input = toHTMLInputElement(element);
        if (!input->isTextField())
            continue;

        fieldNamesAndValues.append(make_pair(input->name().string(), input->value()));
    }
}

void HTMLFormElement::submitDialog(PassRefPtr<FormSubmission> formSubmission)
{
    for (Node* node = this; node; node = node->parentOrShadowHostNode()) {
        if (node->hasTagName(dialogTag)) {
            toHTMLDialogElement(node)->closeDialog(formSubmission->result());
            return;
        }
    }
}

void HTMLFormElement::submit(Event* event, bool activateSubmitButton, bool processingUserGesture, FormSubmissionTrigger formSubmissionTrigger)
{
    FrameView* view = document().view();
    Frame* frame = document().frame();
    if (!view || !frame || !frame->page())
        return;

    if (m_isSubmittingOrPreparingForSubmission) {
        m_shouldSubmit = true;
        return;
    }

    m_isSubmittingOrPreparingForSubmission = true;
    m_wasUserSubmitted = processingUserGesture;

    RefPtr<HTMLFormControlElement> firstSuccessfulSubmitButton;
    bool needButtonActivation = activateSubmitButton; // do we need to activate a submit button?

    for (unsigned i = 0; i < m_associatedElements.size(); ++i) {
        FormAssociatedElement* associatedElement = m_associatedElements[i];
        if (!associatedElement->isFormControlElement())
            continue;
        if (needButtonActivation) {
            HTMLFormControlElement* control = toHTMLFormControlElement(associatedElement);
            if (control->isActivatedSubmit())
                needButtonActivation = false;
            else if (firstSuccessfulSubmitButton == 0 && control->isSuccessfulSubmitButton())
                firstSuccessfulSubmitButton = control;
        }
    }

    if (needButtonActivation && firstSuccessfulSubmitButton)
        firstSuccessfulSubmitButton->setActivatedSubmit(true);

    RefPtr<FormSubmission> formSubmission = FormSubmission::create(this, m_attributes, event, formSubmissionTrigger);
    EventQueueScope scopeForDialogClose; // Delay dispatching 'close' to dialog until done submitting.
    if (formSubmission->method() == FormSubmission::DialogMethod)
        submitDialog(formSubmission.release());
    else
        scheduleFormSubmission(formSubmission.release());

    if (needButtonActivation && firstSuccessfulSubmitButton)
        firstSuccessfulSubmitButton->setActivatedSubmit(false);

    m_shouldSubmit = false;
    m_isSubmittingOrPreparingForSubmission = false;
}

void HTMLFormElement::scheduleFormSubmission(PassRefPtr<FormSubmission> submission)
{
    ASSERT(submission->method() == FormSubmission::PostMethod || submission->method() == FormSubmission::GetMethod);
    ASSERT(submission->data());
    ASSERT(submission->state());
    if (submission->action().isEmpty())
        return;
    if (document().isSandboxed(SandboxForms)) {
        // FIXME: This message should be moved off the console once a solution to https://bugs.webkit.org/show_bug.cgi?id=103274 exists.
        document().addConsoleMessage(SecurityMessageSource, ErrorMessageLevel, "Blocked form submission to '" + submission->action().elidedString() + "' because the form's frame is sandboxed and the 'allow-forms' permission is not set.");
        return;
    }

    if (protocolIsJavaScript(submission->action())) {
        if (!document().contentSecurityPolicy()->allowFormAction(KURL(submission->action())))
            return;
        document().frame()->script().executeScriptIfJavaScriptURL(submission->action());
        return;
    }

    Frame* targetFrame = document().frame()->loader().findFrameForNavigation(submission->target(), submission->state()->sourceDocument());
    if (!targetFrame) {
        if (!DOMWindow::allowPopUp(document().frame()) && !UserGestureIndicator::processingUserGesture())
            return;
        targetFrame = document().frame();
    } else {
        submission->clearTarget();
    }
    if (!targetFrame->page())
        return;

    submission->setReferrer(document().outgoingReferrer());
    submission->setOrigin(document().outgoingOrigin());

    targetFrame->navigationScheduler().scheduleFormSubmission(submission);
}

void HTMLFormElement::reset()
{
    Frame* frame = document().frame();
    if (m_isInResetFunction || !frame)
        return;

    m_isInResetFunction = true;

    if (!dispatchEvent(Event::createCancelableBubble(EventTypeNames::reset))) {
        m_isInResetFunction = false;
        return;
    }

    for (unsigned i = 0; i < m_associatedElements.size(); ++i) {
        if (m_associatedElements[i]->isFormControlElement())
            toHTMLFormControlElement(m_associatedElements[i])->reset();
    }

    m_isInResetFunction = false;
}

void HTMLFormElement::requestAutocomplete()
{
    Frame* frame = document().frame();
    if (!frame)
        return;

    if (!shouldAutocomplete() || !UserGestureIndicator::processingUserGesture()) {
        finishRequestAutocomplete(AutocompleteResultErrorDisabled);
        return;
    }

    StringPairVector controlNamesAndValues;
    getTextFieldValues(controlNamesAndValues);
    RefPtr<FormState> formState = FormState::create(this, controlNamesAndValues, &document(), SubmittedByJavaScript);
    frame->loader().client()->didRequestAutocomplete(formState.release());
}

void HTMLFormElement::finishRequestAutocomplete(AutocompleteResult result)
{
    RefPtr<Event> event;
    if (result == AutocompleteResultSuccess)
        event = Event::create(EventTypeNames::autocomplete);
    else if (result == AutocompleteResultErrorDisabled)
        event = AutocompleteErrorEvent::create("disabled");
    else if (result == AutocompleteResultErrorCancel)
        event = AutocompleteErrorEvent::create("cancel");
    else if (result == AutocompleteResultErrorInvalid)
        event = AutocompleteErrorEvent::create("invalid");

    event->setTarget(this);
    m_pendingAutocompleteEvents.append(event.release());

    // Dispatch events later as this API is meant to work asynchronously in all situations and implementations.
    if (!m_requestAutocompleteTimer.isActive())
        m_requestAutocompleteTimer.startOneShot(0);
}

void HTMLFormElement::requestAutocompleteTimerFired(Timer<HTMLFormElement>*)
{
    Vector<RefPtr<Event> > pendingEvents;
    m_pendingAutocompleteEvents.swap(pendingEvents);
    for (size_t i = 0; i < pendingEvents.size(); ++i)
        dispatchEvent(pendingEvents[i].release());
}

void HTMLFormElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    if (name == actionAttr)
        m_attributes.parseAction(value);
    else if (name == targetAttr)
        m_attributes.setTarget(value);
    else if (name == methodAttr)
        m_attributes.updateMethodType(value);
    else if (name == enctypeAttr)
        m_attributes.updateEncodingType(value);
    else if (name == accept_charsetAttr)
        m_attributes.setAcceptCharset(value);
    else if (name == onautocompleteAttr)
        setAttributeEventListener(EventTypeNames::autocomplete, createAttributeEventListener(this, name, value));
    else if (name == onautocompleteerrorAttr)
        setAttributeEventListener(EventTypeNames::autocompleteerror, createAttributeEventListener(this, name, value));
    else
        HTMLElement::parseAttribute(name, value);
}

template<class T, size_t n> static void removeFromVector(Vector<T*, n> & vec, T* item)
{
    size_t size = vec.size();
    for (size_t i = 0; i != size; ++i)
        if (vec[i] == item) {
            vec.remove(i);
            break;
        }
}

unsigned HTMLFormElement::formElementIndexWithFormAttribute(Element* element, unsigned rangeStart, unsigned rangeEnd)
{
    if (m_associatedElements.isEmpty())
        return 0;

    ASSERT(rangeStart <= rangeEnd);

    if (rangeStart == rangeEnd)
        return rangeStart;

    unsigned left = rangeStart;
    unsigned right = rangeEnd - 1;
    unsigned short position;

    // Does binary search on m_associatedElements in order to find the index
    // to be inserted.
    while (left != right) {
        unsigned middle = left + ((right - left) / 2);
        ASSERT(middle < m_associatedElementsBeforeIndex || middle >= m_associatedElementsAfterIndex);
        position = element->compareDocumentPosition(toHTMLElement(m_associatedElements[middle]));
        if (position & DOCUMENT_POSITION_FOLLOWING)
            right = middle;
        else
            left = middle + 1;
    }

    ASSERT(left < m_associatedElementsBeforeIndex || left >= m_associatedElementsAfterIndex);
    position = element->compareDocumentPosition(toHTMLElement(m_associatedElements[left]));
    if (position & DOCUMENT_POSITION_FOLLOWING)
        return left;
    return left + 1;
}

unsigned HTMLFormElement::formElementIndex(FormAssociatedElement& associatedElement)
{
    HTMLElement& associatedHTMLElement = toHTMLElement(associatedElement);
    // Treats separately the case where this element has the form attribute
    // for performance consideration.
    if (associatedHTMLElement.fastHasAttribute(formAttr)) {
        unsigned short position = compareDocumentPosition(&associatedHTMLElement);
        if (position & DOCUMENT_POSITION_PRECEDING) {
            ++m_associatedElementsBeforeIndex;
            ++m_associatedElementsAfterIndex;
            return HTMLFormElement::formElementIndexWithFormAttribute(&associatedHTMLElement, 0, m_associatedElementsBeforeIndex - 1);
        }
        if (position & DOCUMENT_POSITION_FOLLOWING && !(position & DOCUMENT_POSITION_CONTAINED_BY))
            return HTMLFormElement::formElementIndexWithFormAttribute(&associatedHTMLElement, m_associatedElementsAfterIndex, m_associatedElements.size());
    }

    // Check for the special case where this element is the very last thing in
    // the form's tree of children; we don't want to walk the entire tree in that
    // common case that occurs during parsing; instead we'll just return a value
    // that says "add this form element to the end of the array".
    if (ElementTraversal::next(associatedHTMLElement, this)) {
        unsigned i = m_associatedElementsBeforeIndex;
        for (Element* element = this; element; element = ElementTraversal::next(*element, this)) {
            if (element == associatedHTMLElement) {
                ++m_associatedElementsAfterIndex;
                return i;
            }
            if (!element->isFormControlElement() && !element->hasTagName(objectTag))
                continue;
            if (!element->isHTMLElement() || toHTMLElement(element)->formOwner() != this)
                continue;
            ++i;
        }
    }
    return m_associatedElementsAfterIndex++;
}

void HTMLFormElement::registerFormElement(FormAssociatedElement& e)
{
    m_associatedElements.insert(formElementIndex(e), &e);
}

void HTMLFormElement::removeFormElement(FormAssociatedElement* e)
{
    unsigned index;
    for (index = 0; index < m_associatedElements.size(); ++index) {
        if (m_associatedElements[index] == e)
            break;
    }
    ASSERT_WITH_SECURITY_IMPLICATION(index < m_associatedElements.size());
    if (index < m_associatedElementsBeforeIndex)
        --m_associatedElementsBeforeIndex;
    if (index < m_associatedElementsAfterIndex)
        --m_associatedElementsAfterIndex;
    removeFromPastNamesMap(*toHTMLElement(e));
    removeFromVector(m_associatedElements, e);
}

bool HTMLFormElement::isURLAttribute(const Attribute& attribute) const
{
    return attribute.name() == actionAttr || HTMLElement::isURLAttribute(attribute);
}

void HTMLFormElement::registerImgElement(HTMLImageElement* e)
{
    ASSERT(m_imageElements.find(e) == kNotFound);
    m_imageElements.append(e);
}

void HTMLFormElement::removeImgElement(HTMLImageElement* e)
{
    ASSERT(m_imageElements.find(e) != kNotFound);
    removeFromPastNamesMap(*e);
    removeFromVector(m_imageElements, e);
}

PassRefPtr<HTMLCollection> HTMLFormElement::elements()
{
    return ensureCachedHTMLCollection(FormControls);
}

String HTMLFormElement::name() const
{
    return getNameAttribute();
}

bool HTMLFormElement::noValidate() const
{
    return fastHasAttribute(novalidateAttr);
}

// FIXME: This function should be removed because it does not do the same thing as the
// JavaScript binding for action, which treats action as a URL attribute. Last time I
// (Darin Adler) removed this, someone added it back, so I am leaving it in for now.
const AtomicString& HTMLFormElement::action() const
{
    return getAttribute(actionAttr);
}

void HTMLFormElement::setAction(const AtomicString& value)
{
    setAttribute(actionAttr, value);
}

void HTMLFormElement::setEnctype(const AtomicString& value)
{
    setAttribute(enctypeAttr, value);
}

String HTMLFormElement::method() const
{
    return FormSubmission::Attributes::methodString(m_attributes.method());
}

void HTMLFormElement::setMethod(const AtomicString& value)
{
    setAttribute(methodAttr, value);
}

bool HTMLFormElement::wasUserSubmitted() const
{
    return m_wasUserSubmitted;
}

HTMLFormControlElement* HTMLFormElement::defaultButton() const
{
    for (unsigned i = 0; i < m_associatedElements.size(); ++i) {
        if (!m_associatedElements[i]->isFormControlElement())
            continue;
        HTMLFormControlElement* control = toHTMLFormControlElement(m_associatedElements[i]);
        if (control->isSuccessfulSubmitButton())
            return control;
    }

    return 0;
}

bool HTMLFormElement::checkValidity()
{
    Vector<RefPtr<FormAssociatedElement> > controls;
    return !checkInvalidControlsAndCollectUnhandled(&controls);
}

bool HTMLFormElement::checkValidityWithoutDispatchingEvents()
{
    return !checkInvalidControlsAndCollectUnhandled(0, HTMLFormControlElement::CheckValidityDispatchEventsNone);
}

bool HTMLFormElement::checkInvalidControlsAndCollectUnhandled(Vector<RefPtr<FormAssociatedElement> >* unhandledInvalidControls, HTMLFormControlElement::CheckValidityDispatchEvents dispatchEvents)
{
    RefPtr<HTMLFormElement> protector(this);
    // Copy m_associatedElements because event handlers called from
    // HTMLFormControlElement::checkValidity() might change m_associatedElements.
    Vector<RefPtr<FormAssociatedElement> > elements;
    elements.reserveCapacity(m_associatedElements.size());
    for (unsigned i = 0; i < m_associatedElements.size(); ++i)
        elements.append(m_associatedElements[i]);
    bool hasInvalidControls = false;
    for (unsigned i = 0; i < elements.size(); ++i) {
        if (elements[i]->form() == this && elements[i]->isFormControlElement()) {
            HTMLFormControlElement* control = toHTMLFormControlElement(elements[i].get());
            if (!control->checkValidity(unhandledInvalidControls, dispatchEvents) && control->formOwner() == this)
                hasInvalidControls = true;
        }
    }
    return hasInvalidControls;
}

Node* HTMLFormElement::elementFromPastNamesMap(const AtomicString& pastName) const
{
    if (pastName.isEmpty() || !m_pastNamesMap)
        return 0;
    Node* node = m_pastNamesMap->get(pastName);
#if !ASSERT_DISABLED
    if (!node)
        return 0;
    ASSERT_WITH_SECURITY_IMPLICATION(toHTMLElement(node)->formOwner() == this);
    if (node->hasTagName(imgTag)) {
        ASSERT_WITH_SECURITY_IMPLICATION(m_imageElements.find(node) != kNotFound);
    } else if (node->hasTagName(objectTag)) {
        ASSERT_WITH_SECURITY_IMPLICATION(m_associatedElements.find(toHTMLObjectElement(node)) != kNotFound);
    } else {
        ASSERT_WITH_SECURITY_IMPLICATION(m_associatedElements.find(toHTMLFormControlElement(node)) != kNotFound);
    }
#endif
    return node;
}

void HTMLFormElement::addToPastNamesMap(Node* element, const AtomicString& pastName)
{
    if (pastName.isEmpty())
        return;
    if (!m_pastNamesMap)
        m_pastNamesMap = adoptPtr(new PastNamesMap);
    m_pastNamesMap->set(pastName, element);
}

void HTMLFormElement::removeFromPastNamesMap(HTMLElement& element)
{
    if (!m_pastNamesMap)
        return;
    PastNamesMap::iterator end = m_pastNamesMap->end();
    for (PastNamesMap::iterator it = m_pastNamesMap->begin(); it != end; ++it) {
        if (it->value == &element) {
            it->value = 0;
            // Keep looping. Single element can have multiple names.
        }
    }
}

void HTMLFormElement::getNamedElements(const AtomicString& name, Vector<RefPtr<Node> >& namedItems)
{
    // http://www.whatwg.org/specs/web-apps/current-work/multipage/forms.html#dom-form-nameditem
    elements()->namedItems(name, namedItems);

    Node* elementFromPast = elementFromPastNamesMap(name);
    if (namedItems.size() && namedItems.first() != elementFromPast)
        addToPastNamesMap(namedItems.first().get(), name);
    else if (elementFromPast && namedItems.isEmpty())
        namedItems.append(elementFromPast);
}

bool HTMLFormElement::shouldAutocomplete() const
{
    return !equalIgnoringCase(fastGetAttribute(autocompleteAttr), "off");
}

void HTMLFormElement::finishParsingChildren()
{
    HTMLElement::finishParsingChildren();
    document().formController()->restoreControlStateIn(*this);
}

void HTMLFormElement::copyNonAttributePropertiesFromElement(const Element& source)
{
    m_wasDemoted = static_cast<const HTMLFormElement&>(source).m_wasDemoted;
    HTMLElement::copyNonAttributePropertiesFromElement(source);
}

void HTMLFormElement::anonymousNamedGetter(const AtomicString& name, bool& returnValue0Enabled, RefPtr<RadioNodeList>& returnValue0, bool& returnValue1Enabled, RefPtr<Node>& returnValue1)
{
    // Call getNamedElements twice, first time check if it has a value
    // and let HTMLFormElement update its cache.
    // See issue: 867404
    {
        Vector<RefPtr<Node> > elements;
        getNamedElements(name, elements);
        if (elements.isEmpty())
            return;
    }

    // Second call may return different results from the first call,
    // but if the first the size cannot be zero.
    Vector<RefPtr<Node> > elements;
    getNamedElements(name, elements);
    ASSERT(!elements.isEmpty());

    if (elements.size() == 1) {
        returnValue1Enabled = true;
        returnValue1 = elements.at(0);
        return;
    }

    bool onlyMatchImg = elements.size() && elements.at(0)->hasTagName(imgTag);
    returnValue0Enabled = true;
    returnValue0 = radioNodeList(name, onlyMatchImg);
}

void HTMLFormElement::setDemoted(bool demoted)
{
    if (demoted)
        UseCounter::count(document(), UseCounter::DemotedFormElement);
    m_wasDemoted = demoted;
}

} // namespace
