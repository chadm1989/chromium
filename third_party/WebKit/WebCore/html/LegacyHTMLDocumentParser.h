/*
    Copyright (C) 1997 Martin Jones (mjones@kde.org)
              (C) 1997 Torben Weis (weis@kde.org)
              (C) 1998 Waldo Bastian (bastian@kde.org)
              (C) 2001 Dirk Mueller (mueller@kde.org)
    Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef LegacyHTMLDocumentParser_h
#define LegacyHTMLDocumentParser_h

#include "CachedResourceClient.h"
#include "CachedResourceHandle.h"
#include "FragmentScriptingPermission.h"
#include "NamedNodeMap.h"
#include "SegmentedString.h"
#include "Timer.h"
#include "DocumentParser.h"
#include <wtf/Deque.h>
#include <wtf/OwnPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

class CachedScript;
class DocumentFragment;
class Document;
class HTMLDocument;
class HTMLScriptElement;
class HTMLViewSourceDocument;
class FrameView;
class LegacyHTMLTreeBuilder;
class Node;
class LegacyPreloadScanner;
class ScriptSourceCode;

/**
 * @internal
 * represents one HTML tag. Consists of a numerical id, and the list
 * of attributes. Can also represent text. In this case the id = 0 and
 * text contains the text.
 */
struct Token {
    Token()
        : beginTag(true)
        , selfClosingTag(false)
        , brokenXMLStyle(false)
        , m_sourceInfo(0)
    { }
    ~Token() { }

    void addAttribute(AtomicString& attrName, const AtomicString& v, bool viewSourceMode);

    bool isOpenTag(const QualifiedName& fullName) const { return beginTag && fullName.localName() == tagName; }
    bool isCloseTag(const QualifiedName& fullName) const { return !beginTag && fullName.localName() == tagName; }

    void reset()
    {
        attrs = 0;
        text = 0;
        tagName = nullAtom;
        beginTag = true;
        selfClosingTag = false;
        brokenXMLStyle = false;
        if (m_sourceInfo)
            m_sourceInfo->clear();
    }

    void addViewSourceChar(UChar c) { if (!m_sourceInfo.get()) m_sourceInfo.set(new Vector<UChar>); m_sourceInfo->append(c); }

    RefPtr<NamedNodeMap> attrs;
    RefPtr<StringImpl> text;
    AtomicString tagName;
    bool beginTag;
    bool selfClosingTag;
    bool brokenXMLStyle;
    OwnPtr<Vector<UChar> > m_sourceInfo;
};

enum DoctypeState {
    DoctypeBegin,
    DoctypeBeforeName,
    DoctypeName,
    DoctypeAfterName,
    DoctypeBeforePublicID,
    DoctypePublicID,
    DoctypeAfterPublicID,
    DoctypeBeforeSystemID,
    DoctypeSystemID,
    DoctypeAfterSystemID,
    DoctypeBogus
};

class DoctypeToken {
public:
    DoctypeToken() {}

    void reset()
    {
        m_name.clear();
        m_publicID.clear();
        m_systemID.clear();
        m_state = DoctypeBegin;
        m_source.clear();
        m_forceQuirks = false;
    }

    DoctypeState state() { return m_state; }
    void setState(DoctypeState s) { m_state = s; }

    Vector<UChar> m_name;
    Vector<UChar> m_publicID;
    Vector<UChar> m_systemID;
    DoctypeState m_state;

    Vector<UChar> m_source;

    bool m_forceQuirks; // Used by the HTML5 parser.
};

//-----------------------------------------------------------------------------

// FIXME: This class does too much.  Right now it is both an HTML tokenizer as well
// as handling all of the non-tokenizer-specific junk related to tokenizing HTML
// (like dealing with <script> tags).  The HTML tokenizer bits should be pushed
// down into a separate HTML tokenizer class.

class LegacyHTMLDocumentParser : public DocumentParser, public CachedResourceClient {
public:
    LegacyHTMLDocumentParser(HTMLDocument*, bool reportErrors);
    LegacyHTMLDocumentParser(HTMLViewSourceDocument*);
    LegacyHTMLDocumentParser(DocumentFragment*, FragmentScriptingPermission = FragmentScriptingAllowed);
    virtual ~LegacyHTMLDocumentParser();

    bool forceSynchronous() const { return m_state.forceSynchronous(); }
    void setForceSynchronous(bool force);

    // Exposed for LegacyHTMLTreeBuilder::reportErrorToConsole
    bool processingContentWrittenByScript() const { return m_src.excludeLineNumbers(); }

    static void parseDocumentFragment(const String&, DocumentFragment*, FragmentScriptingPermission = FragmentScriptingAllowed);

protected:
    // Exposed for FTPDirectoryDocumentParser
    virtual void insert(const SegmentedString&);
    virtual void finish();

private:
    // DocumentParser
    virtual void append(const SegmentedString&);
    virtual bool finishWasCalled();
    virtual bool isWaitingForScripts() const;
    virtual void stopParsing();
    virtual bool processingData() const;
    virtual bool isExecutingScript() const { return !!m_executingScript; }

    virtual int lineNumber() const { return m_lineNumber; }
    virtual int columnNumber() const { return 1; }

    virtual void executeScriptsWaitingForStylesheets();

    virtual LegacyHTMLTreeBuilder* htmlTreeBuilder() const { return m_treeBuilder.get(); }
    virtual LegacyHTMLDocumentParser* asHTMLDocumentParser() { return this; }

    class State;

    void begin();
    void end();
    void reset();

    void willWriteHTML(const SegmentedString&);
    void write(const SegmentedString&, bool appendData);
    ALWAYS_INLINE void advance(State&);
    void didWriteHTML();

    PassRefPtr<Node> processToken();
    void processDoctypeToken();

    State processListing(SegmentedString, State);
    State parseComment(SegmentedString&, State);
    State parseDoctype(SegmentedString&, State);
    State parseServer(SegmentedString&, State);
    State parseText(SegmentedString&, State);
    State parseNonHTMLText(SegmentedString&, State);
    State parseTag(SegmentedString&, State);
    State parseEntity(SegmentedString&, UChar*& dest, State, unsigned& cBufferPos, bool start, bool parsingTag);
    State parseProcessingInstruction(SegmentedString&, State);
    State scriptHandler(State);
    State scriptExecution(const ScriptSourceCode&, State);
    void setSrc(const SegmentedString&);

    // check if we have enough space in the buffer.
    // if not enlarge it
    inline void checkBuffer(int len = 10)
    {
        if ((m_dest - m_buffer) > m_bufferSize - len)
            enlargeBuffer(len);
    }

    inline void checkScriptBuffer(int len = 10)
    {
        if (m_scriptCodeSize + len >= m_scriptCodeCapacity)
            enlargeScriptBuffer(len);
    }

    void enlargeBuffer(int len);
    void enlargeScriptBuffer(int len);

    bool continueProcessing(int& processedCount, double startTime, State&);
    void timerFired(Timer<LegacyHTMLDocumentParser>*);
    void allDataProcessed();

    // from CachedResourceClient
    void notifyFinished(CachedResource*);

    void executeExternalScriptsIfReady();
    void executeExternalScriptsTimerFired(Timer<LegacyHTMLDocumentParser>*);
    bool continueExecutingExternalScripts(double startTime);

    // Internal buffers
    ///////////////////
    UChar* m_buffer;
    int m_bufferSize;
    UChar* m_dest;

    Token m_currentToken;

    // This buffer holds the raw characters we've seen between the beginning of
    // the attribute name and the first character of the attribute value.
    Vector<UChar, 32> m_rawAttributeBeforeValue;

    // DocumentParser flags
    //////////////////
    // are we in quotes within a html tag
    enum { NoQuote, SingleQuote, DoubleQuote } tquote;

    // Are we in a &... character entity description?
    enum EntityState {
        NoEntity = 0,
        SearchEntity = 1,
        NumericSearch = 2,
        Hexadecimal = 3,
        Decimal = 4,
        EntityName = 5,
        SearchSemicolon = 6
    };
    unsigned EntityUnicodeValue;

    enum TagState {
        NoTag = 0,
        TagName = 1,
        SearchAttribute = 2,
        AttributeName = 3,
        SearchEqual = 4,
        SearchValue = 5,
        QuotedValue = 6,
        Value = 7,
        SearchEnd = 8
    };

    class State {
    public:
        State() : m_bits(0) { }

        TagState tagState() const { return static_cast<TagState>(m_bits & TagMask); }
        void setTagState(TagState t) { m_bits = (m_bits & ~TagMask) | t; }
        EntityState entityState() const { return static_cast<EntityState>((m_bits & EntityMask) >> EntityShift); }
        void setEntityState(EntityState e) { m_bits = (m_bits & ~EntityMask) | (e << EntityShift); }

        bool inScript() const { return testBit(InScript); }
        void setInScript(bool v) { setBit(InScript, v); }
        bool inStyle() const { return testBit(InStyle); }
        void setInStyle(bool v) { setBit(InStyle, v); }
        bool inXmp() const { return testBit(InXmp); }
        void setInXmp(bool v) { setBit(InXmp, v); }
        bool inTitle() const { return testBit(InTitle); }
        void setInTitle(bool v) { setBit(InTitle, v); }
        bool inIFrame() const { return testBit(InIFrame); }
        void setInIFrame(bool v) { setBit(InIFrame, v); }
        bool inPlainText() const { return testBit(InPlainText); }
        void setInPlainText(bool v) { setBit(InPlainText, v); }
        bool inProcessingInstruction() const { return testBit(InProcessingInstruction); }
        void setInProcessingInstruction(bool v) { return setBit(InProcessingInstruction, v); }
        bool inComment() const { return testBit(InComment); }
        void setInComment(bool v) { setBit(InComment, v); }
        bool inDoctype() const { return testBit(InDoctype); }
        void setInDoctype(bool v) { setBit(InDoctype, v); }
        bool inTextArea() const { return testBit(InTextArea); }
        void setInTextArea(bool v) { setBit(InTextArea, v); }
        bool escaped() const { return testBit(Escaped); }
        void setEscaped(bool v) { setBit(Escaped, v); }
        bool inServer() const { return testBit(InServer); }
        void setInServer(bool v) { setBit(InServer, v); }
        bool skipLF() const { return testBit(SkipLF); }
        void setSkipLF(bool v) { setBit(SkipLF, v); }
        bool startTag() const { return testBit(StartTag); }
        void setStartTag(bool v) { setBit(StartTag, v); }
        bool discardLF() const { return testBit(DiscardLF); }
        void setDiscardLF(bool v) { setBit(DiscardLF, v); }
        bool allowYield() const { return testBit(AllowYield); }
        void setAllowYield(bool v) { setBit(AllowYield, v); }
        bool loadingExtScript() const { return testBit(LoadingExtScript); }
        void setLoadingExtScript(bool v) { setBit(LoadingExtScript, v); }
        bool forceSynchronous() const { return testBit(ForceSynchronous); }
        void setForceSynchronous(bool v) { setBit(ForceSynchronous, v); }

        bool inAnyNonHTMLText() const { return m_bits & (InScript | InStyle | InXmp | InTextArea | InTitle | InIFrame); }
        bool hasTagState() const { return m_bits & TagMask; }
        bool hasEntityState() const { return m_bits & EntityMask; }

        bool needsSpecialWriteHandling() const { return m_bits & (InScript | InStyle | InXmp | InTextArea | InTitle | InIFrame | TagMask | EntityMask | InPlainText | InComment | InDoctype | InServer | InProcessingInstruction | StartTag); }

    private:
        static const int EntityShift = 4;
        enum StateBits {
            TagMask = (1 << 4) - 1,
            EntityMask = (1 << 7) - (1 << 4),
            InScript = 1 << 7,
            InStyle = 1 << 8,
            // Bit 9 unused
            InXmp = 1 << 10,
            InTitle = 1 << 11,
            InPlainText = 1 << 12,
            InProcessingInstruction = 1 << 13,
            InComment = 1 << 14,
            InTextArea = 1 << 15,
            Escaped = 1 << 16,
            InServer = 1 << 17,
            SkipLF = 1 << 18,
            StartTag = 1 << 19,
            DiscardLF = 1 << 20, // FIXME: should clarify difference between skip and discard
            AllowYield = 1 << 21,
            LoadingExtScript = 1 << 22,
            ForceSynchronous = 1 << 23,
            InIFrame = 1 << 24,
            InDoctype = 1 << 25
        };

        void setBit(StateBits bit, bool value)
        {
            if (value)
                m_bits |= bit;
            else
                m_bits &= ~bit;
        }
        bool testBit(StateBits bit) const { return m_bits & bit; }

        unsigned m_bits;
    };

    State m_state;

    DoctypeToken m_doctypeToken;
    int m_doctypeSearchCount;
    int m_doctypeSecondarySearchCount;

    bool m_brokenServer;

    // Name of an attribute that we just scanned.
    AtomicString m_attrName;

    // Used to store the code of a scripting sequence
    UChar* m_scriptCode;
    // Size of the script sequenze stored in @ref #scriptCode
    int m_scriptCodeSize;
    // Maximal size that can be stored in @ref #scriptCode
    int m_scriptCodeCapacity;
    // resync point of script code size
    int m_scriptCodeResync;

    // Stores characters if we are scanning for a string like "</script>"
    UChar searchBuffer[10];

    // Counts where we are in the string we are scanning for
    int searchCount;
    // the stopper string
    const char* m_searchStopper;
    int m_searchStopperLength;

    // if no more data is coming, just parse what we have (including ext scripts that
    // may be still downloading) and finish
    bool m_noMoreData;
    // URL to get source code of script from
    String m_scriptTagSrcAttrValue;
    String m_scriptTagCharsetAttrValue;
    // the HTML code we will parse after the external script we are waiting for has loaded
    SegmentedString m_pendingSrc;

    // the HTML code we will parse after this particular script has
    // loaded, but before all pending HTML
    SegmentedString* m_currentPrependingSrc;

    // true if we are executing a script while parsing a document. This causes the parsing of
    // the output of the script to be postponed until after the script has finished executing
    int m_executingScript;
    Deque<CachedResourceHandle<CachedScript> > m_pendingScripts;
    RefPtr<HTMLScriptElement> m_scriptNode;

    bool m_requestingScript;
    bool m_hasScriptsWaitingForStylesheets;

    // if we found one broken comment, there are most likely others as well
    // store a flag to get rid of the O(n^2) behaviour in such a case.
    bool m_brokenComments;
    // current line number
    int m_lineNumber;
    int m_currentScriptTagStartLineNumber;
    int m_currentTagStartLineNumber;

    double m_tokenizerTimeDelay;
    int m_tokenizerChunkSize;

    // The timer for continued processing.
    Timer<LegacyHTMLDocumentParser> m_timer;

    // The timer for continued executing external scripts.
    Timer<LegacyHTMLDocumentParser> m_externalScriptsTimer;

// This buffer can hold arbitrarily long user-defined attribute names, such as in EMBED tags.
// So any fixed number might be too small, but rather than rewriting all usage of this buffer
// we'll just make it large enough to handle all imaginable cases.
#define CBUFLEN 1024
    UChar m_cBuffer[CBUFLEN + 2];
    unsigned int m_cBufferPos;

    SegmentedString m_src;
    OwnPtr<LegacyHTMLTreeBuilder> m_treeBuilder;
    bool m_inWrite;
    bool m_fragment;
    FragmentScriptingPermission m_scriptingPermission;

    OwnPtr<LegacyPreloadScanner> m_preloadScanner;
};

UChar decodeNamedEntity(const char*);

} // namespace WebCore

#endif // LegacyHTMLDocumentParser_h
