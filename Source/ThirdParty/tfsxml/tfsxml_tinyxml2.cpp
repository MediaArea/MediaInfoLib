/* Copyright (c) MediaArea.net SARL

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

(MIT License)

*/

/*
 * Compatibility layer with the tinyxml2 API, based on tfsxml
 *
 * The behavior of tinyxml2 6.2.0 is reproduced, including its quirks (see
 * Source/Doc/Tinyxml2.md).
 */

#include "tfsxml_tinyxml2.h"
#include "tfsxml.h"
#include <cstdarg>
#include <cstdlib>
#include <new>

#if defined(_MSC_VER)
    #define TFSXML_SSCANF sscanf_s
#else
    #define TFSXML_SSCANF sscanf
#endif

namespace TFSXML_TINYXML2_NAMESPACE
{

/* Decoding of raw content */
enum {
    TFSXML_ENTITIES = 0x01,     /* Entities and character references */
    TFSXML_NEWLINES = 0x02,     /* CR LF, LF CR and CR become LF */
    TFSXML_COLLAPSE = 0x04,     /* Whitespaces are collapsed (COLLAPSE_WHITESPACE) */
    TFSXML_RAW      = 0x100,    /* Value not yet decoded */
    TFSXML_RAW_NAME = 0x200     /* Attribute name not yet copied */
};

static const struct {
    const char* pattern;
    size_t      length;
    char        value;
} tfsxml_entities[5] = {
    { "quot", 4, '"'  },
    { "amp",  3, '&'  },
    { "apos", 4, '\'' },
    { "lt",   2, '<'  },
    { "gt",   2, '>'  },
};

/* First occurrence of pattern in [p, end), NULL if not found */
static char* tfsxml_search(char* p, char* end, const char* pattern)
{
    const size_t length = strlen(pattern);
    while ((size_t)(end - p) >= length) {
        char* q = (char*)memchr(p, pattern[0], (size_t)(end - p) - length + 1);
        if (!q) {
            return 0;
        }
        if (!memcmp(q, pattern, length)) {
            return q;
        }
        p = q + 1;
    }
    return 0;
}

/* Name content (element and attribute names) */
static bool tfsxml_is_name(const char* p, size_t len)
{
    if (!len || !XMLUtil::IsNameStartChar((unsigned char)*p)) {
        return false;
    }
    for (size_t i = 1; i < len; i++) {
        if (!XMLUtil::IsNameChar((unsigned char)p[i])) {
            return false;
        }
    }
    return true;
}

/* Raw name compared to a string ended by NUL */
static bool tfsxml_name_is(const char* p, const char* end, const char* name)
{
    const size_t len = (size_t)(end - p);
    return !strncmp(p, name, len) && !name[len];
}

/* Character reference starting at p ("&#") in [p, end)
 * return the position after the reference, NULL if it is invalid
 * Same decisions as tinyxml2: digits are read backward from the first ";" up to "x" or "#",
 * a value is computed even with overflows, "&#" just before the end is removed */
static const char* tfsxml_character_reference(const char* p, const char* end, char* utf8, int* length)
{
    unsigned long ucs = 0;
    unsigned mult = 1;
    const char* semicolon;
    const char* q;

    *length = 0;
    if (end - p <= 2) {
        return p + 1;
    }
    if (p[2] == 'x') {
        if (end - p <= 3) {
            return 0;
        }
        semicolon = (const char*)memchr(p + 3, ';', (size_t)(end - (p + 3)));
        if (!semicolon) {
            return 0;
        }
        for (q = semicolon - 1; *q != 'x'; q--) {
            unsigned digit;
            if (*q >= '0' && *q <= '9') {
                digit = (unsigned)(*q - '0');
            }
            else if (*q >= 'a' && *q <= 'f') {
                digit = (unsigned)(*q - 'a' + 10);
            }
            else if (*q >= 'A' && *q <= 'F') {
                digit = (unsigned)(*q - 'A' + 10);
            }
            else {
                return 0;
            }
            ucs += mult * digit;
            mult *= 16;
        }
    }
    else {
        semicolon = (const char*)memchr(p + 2, ';', (size_t)(end - (p + 2)));
        if (!semicolon) {
            return 0;
        }
        for (q = semicolon - 1; *q != '#'; q--) {
            if (*q < '0' || *q > '9') {
                return 0;
            }
            ucs += mult * (unsigned)(*q - '0');
            mult *= 10;
        }
    }
    XMLUtil::ConvertUTF32ToUTF8(ucs, utf8, length);
    return semicolon + 1;
}

/* Whitespace collapsing of a string ended by NUL (COLLAPSE_WHITESPACE) */
static char* tfsxml_collapse(char* s)
{
    while (XMLUtil::IsWhiteSpace(*s)) {
        s++;
    }
    if (*s) {
        const char* p = s;
        char* q = s;
        while (*p) {
            if (XMLUtil::IsWhiteSpace(*p)) {
                while (XMLUtil::IsWhiteSpace(*p)) {
                    p++;
                }
                if (!*p) {
                    break;
                }
                *q++ = ' ';
            }
            *q++ = *p++;
        }
        *q = '\0';
    }
    return s;
}

/** -------------------------------------------------------------------------
        Parser
------------------------------------------------------------------------- **/

/*
 * tfsxml provides the text and the markup, start and end tags are checked
 * with the tinyxml2 grammar. When tfsxml and tinyxml2 would not agree on a
 * tag (e.g. whitespace after "<"), the tag is parsed here and tfsxml is
 * restarted after it. The end of the content is also parsed here, for having
 * the same errors as tinyxml2.
 */
class TfsxmlParser
{
public:
    TfsxmlParser(XMLDocument* document, char* begin, char* end);
    void Run();

    XMLNode*            lastTop;    /* Last complete node at the document level */

private:
    struct Attr {
        char*   name;
        char*   nameEnd;
        char*   value;
        char*   valueEnd;
    };
    struct Tag {
        XMLElement::ElementClosingType closingType;
        char*   name;
        char*   nameEnd;
        char*   end;                /* After ">" */
    };

    int Call(int (*func)(tfsxml_string*, tfsxml_string*), tfsxml_string* s);
    int CallAttr(tfsxml_string* n, tfsxml_string* v);
    char* SkipWhiteSpace(char* p) const;
    bool Restart(char* p);
    bool Tail(char* p);
    bool SplitText(char* p, char* e);
    bool Markup(const tfsxml_string& n);
    bool StartTag(const tfsxml_string& n);
    bool EndTag(const tfsxml_string& n);
    bool FastStartTag(const tfsxml_string& n, Tag& tag);
    bool ParseTag(char* lt, Tag& tag);
    bool HandleTag(char* lt, const Tag& tag);
    bool AddAttr(const Attr& attr);
    bool LevelEnd();
    void AddNode(XMLNode* node, char* start, char* e, char* pos, unsigned flags);
    void AddText(char* start, char* e, char* pos, bool cdata);
    bool AddDeclaration(char* start, char* e, char* pos);
    void Error(XMLError error, const char* pos, const char* format = 0, const char* arg = 0);
    void ErrorName(XMLError error, const char* pos, const Tag& tag);

    XMLDocument*        doc;
    char*               begin;
    char*               end;
    tfsxml_string       priv;
    char*               cur;                /* Position of tfsxml */
    XMLNode*            parent;             /* Parent of new nodes */
    int                 depth;              /* Nesting level as counted by tinyxml2 (1 for the document content) */
    bool                onlyDeclarations;   /* Only declarations at the document level up to now */
    unsigned            textFlags;
    unsigned            attrFlags;
    std::vector<Attr>   attrs;
};

TfsxmlParser::TfsxmlParser(XMLDocument* document, char* begin_, char* end_) :
    lastTop(0),
    doc(document),
    begin(begin_),
    end(end_),
    cur(begin_),
    parent(document),
    depth(1),
    onlyDeclarations(true)
{
    priv.buf = 0;
    priv.len = 0;
    priv.flags = 0;
    attrFlags = (doc->_processEntities ? TFSXML_ENTITIES : 0) | TFSXML_NEWLINES;
    textFlags = attrFlags | (doc->_whitespaceMode == COLLAPSE_WHITESPACE ? TFSXML_COLLAPSE : 0);
}

/* tfsxml call, the window of data is extended if content is bigger than what tfsxml supports at once */
int TfsxmlParser::Call(int (*func)(tfsxml_string*, tfsxml_string*), tfsxml_string* s)
{
    for (;;) {
        const int result = func(&priv, s);
        const size_t left = (size_t)(end - priv.buf);
        if (result <= 0 || left <= priv.len || priv.len == UINT_MAX) {
            return result;
        }
        priv.len = left > UINT_MAX ? UINT_MAX : (unsigned)left;
    }
}

int TfsxmlParser::CallAttr(tfsxml_string* n, tfsxml_string* v)
{
    for (;;) {
        const int result = tfsxml_attr(&priv, n, v);
        const size_t left = (size_t)(end - priv.buf);
        if (result <= 0 || left <= priv.len || priv.len == UINT_MAX) {
            return result;
        }
        priv.len = left > UINT_MAX ? UINT_MAX : (unsigned)left;
    }
}

char* TfsxmlParser::SkipWhiteSpace(char* p) const
{
    while (p < end && XMLUtil::IsWhiteSpace(*p)) {
        p++;
    }
    return p;
}

void TfsxmlParser::Run()
{
    char* p = SkipWhiteSpace(begin);
    doc->_writeBOM = end - p >= 3 && (unsigned char)p[0] == 0xEF && (unsigned char)p[1] == 0xBB && (unsigned char)p[2] == 0xBF;
    if (doc->_writeBOM) {
        p += 3;
    }
    if (p == end) {
        doc->SetError(XML_ERROR_EMPTY_DOCUMENT, 0, 0);
        return;
    }
    if (!Restart(p)) {
        return;
    }

    for (;;) {
        tfsxml_string t, n;
        int result = Call(tfsxml_text, &t);
        if (!result && t.buf == cur) {
            if (!SplitText(cur, (char*)t.buf + t.len)) {
                return;
            }
            cur = (char*)t.buf + t.len;
        }
        else if (result >= 0) {
            /* End of the data (tinyxml2 errors), or tfsxml is not at the expected position (not expected) */
            if (!Tail(cur)) {
                return;
            }
            continue;
        }
        result = Call(tfsxml_next, &n);
        if (result > 0 || n.buf - (result ? 2 : 1) != cur) {
            if (!Tail(cur)) {
                return;
            }
            continue;
        }
        if (!(result ? EndTag(n) : Markup(n))) {
            return;
        }
    }
}

/* Text up to the next "<", then restart of tfsxml at this "<", return false if the parsing is finished */
bool TfsxmlParser::Restart(char* p)
{
    char* q = SkipWhiteSpace(p);
    char* lt = q;
    if (q == end) {
        return LevelEnd();
    }
    if (*q != '<') {
        lt = (char*)memchr(q, '<', (size_t)(end - q));
        if (!lt) {
            Error(XML_ERROR_PARSING_TEXT, q);
            return false;
        }
        if (lt + 1 == end) {
            Error(XML_ERROR_PARSING, q);
            return false;
        }
        AddText(p, lt, q, false);
    }
    const size_t left = (size_t)(end - lt);
    tfsxml_init(&priv, lt, left > UINT_MAX ? UINT_MAX : (unsigned)left, 0);
    cur = lt;
    return true;
}

/* Content parsed without tfsxml up to the next tag, return false if the parsing is finished */
bool TfsxmlParser::Tail(char* p)
{
    for (;;) {
        char* q = SkipWhiteSpace(p);
        char* e;
        if (q == end) {
            return LevelEnd();
        }
        if (*q != '<') {
            char* lt = (char*)memchr(q, '<', (size_t)(end - q));
            if (!lt) {
                Error(XML_ERROR_PARSING_TEXT, q);
                return false;
            }
            if (lt + 1 == end) {
                Error(XML_ERROR_PARSING, q);
                return false;
            }
            AddText(p, lt, q, false);
            p = lt;
            continue;
        }
        const size_t left = (size_t)(end - q);
        if (left >= 2 && q[1] == '?') {
            e = tfsxml_search(q + 2, end, "?>");
            if (!e) {
                Error(XML_ERROR_PARSING_DECLARATION, q);
                return false;
            }
            if (!AddDeclaration(q + 2, e, q)) {
                return false;
            }
            p = e + 2;
        }
        else if (left >= 4 && !memcmp(q, "<!--", 4)) {
            e = tfsxml_search(q + 4, end, "-->");
            if (!e) {
                Error(XML_ERROR_PARSING_COMMENT, q);
                return false;
            }
            AddNode(new (doc->Alloc(sizeof(XMLComment))) XMLComment(doc), q + 4, e, q, TFSXML_NEWLINES);
            p = e + 3;
        }
        else if (left >= 9 && !memcmp(q, "<![CDATA[", 9)) {
            e = tfsxml_search(q + 9, end, "]]>");
            if (!e) {
                Error(XML_ERROR_PARSING_CDATA, q);
                return false;
            }
            AddText(q + 9, e, q, true);
            p = e + 3;
        }
        else if (left >= 2 && q[1] == '!') {
            e = (char*)memchr(q + 2, '>', left - 2);
            if (!e) {
                Error(XML_ERROR_PARSING_UNKNOWN, q);
                return false;
            }
            AddNode(new (doc->Alloc(sizeof(XMLUnknown))) XMLUnknown(doc), q + 2, e, q, TFSXML_NEWLINES);
            p = e + 1;
        }
        else {
            Tag tag;
            if (!ParseTag(q, tag) || !HandleTag(q, tag)) {
                return false;
            }
            return Restart(tag.end);
        }
    }
}

/* Text from tfsxml: CDATA sections are separate nodes, whitespace only texts are ignored */
bool TfsxmlParser::SplitText(char* p, char* e)
{
    for (;;) {
        char* lt = (char*)memchr(p, '<', (size_t)(e - p));
        char* textEnd = lt ? lt : e;
        char* q = p;
        while (q < textEnd && XMLUtil::IsWhiteSpace(*q)) {
            q++;
        }
        if (q < textEnd) {
            AddText(p, textEnd, q, false);
        }
        if (!lt) {
            return true;
        }
        char* cdataEnd = tfsxml_search(lt + 9, e, "]]>");
        if (!cdataEnd) {
            Error(XML_ERROR_PARSING_CDATA, lt); /* Not expected, tfsxml provides only complete CDATA sections */
            return false;
        }
        AddText(lt + 9, cdataEnd, lt, true);
        p = cdataEnd + 3;
    }
}

/* Markup from tfsxml */
bool TfsxmlParser::Markup(const tfsxml_string& n)
{
    char* p = (char*)n.buf;
    char* e = p + n.len; /* ">" */
    switch (*p) {
        case '?':
            cur = e + 1;
            return AddDeclaration(p + 1, e - 1, p - 1);
        case '!':
            cur = e + 1;
            if (n.len >= 3 && p[1] == '-' && p[2] == '-') {
                AddNode(new (doc->Alloc(sizeof(XMLComment))) XMLComment(doc), p + 3, e - 2, p - 1, TFSXML_NEWLINES);
            }
            else {
                AddNode(new (doc->Alloc(sizeof(XMLUnknown))) XMLUnknown(doc), p + 1, e, p - 1, TFSXML_NEWLINES);
            }
            return true;
        default:
            return StartTag(n);
    }
}

bool TfsxmlParser::StartTag(const tfsxml_string& n)
{
    char* lt = (char*)n.buf - 1;
    Tag tag;
    if (FastStartTag(n, tag)) {
        cur = tag.end;
        if (!HandleTag(lt, tag)) {
            return false;
        }
        if (tag.closingType == XMLElement::OPEN) {
            tfsxml_enter(&priv);
        }
        return true;
    }
    if (!ParseTag(lt, tag) || !HandleTag(lt, tag)) {
        return false;
    }
    return Restart(tag.end);
}

/* End tag from tfsxml */
bool TfsxmlParser::EndTag(const tfsxml_string& n)
{
    char* lt = (char*)n.buf - 2;
    Tag tag;
    if (tfsxml_is_name(n.buf, n.len)) {
        char* p = SkipWhiteSpace((char*)n.buf + n.len);
        if (p < end && *p == '>') {
            /* Same end as tfsxml (first ">") */
            tag.closingType = XMLElement::CLOSING;
            tag.name = (char*)n.buf;
            tag.nameEnd = (char*)n.buf + n.len;
            tag.end = p + 1;
            cur = tag.end;
            return HandleTag(lt, tag);
        }
    }
    if (!ParseTag(lt, tag) || !HandleTag(lt, tag)) {
        return false;
    }
    return Restart(tag.end);
}

/* Start tag from tfsxml, return true if it matches the tinyxml2 grammar */
bool TfsxmlParser::FastStartTag(const tfsxml_string& n, Tag& tag)
{
    if (!tfsxml_is_name(n.buf, n.len)) {
        return false;
    }
    attrs.clear();
    char* p = (char*)n.buf + n.len;
    for (;;) {
        tfsxml_string an, av;
        const int result = CallAttr(&an, &av);
        if (result > 0) {
            return false;
        }
        if (result < 0) {
            break;
        }

        /* tfsxml provides the attribute after whitespaces, tinyxml2 needs a name, "=" and a quoted value */
        if (!tfsxml_is_name(an.buf, an.len)) {
            return false;
        }
        char* q = SkipWhiteSpace((char*)an.buf + an.len);
        if (q == end || *q != '=') {
            return false;
        }
        q = SkipWhiteSpace(q + 1);
        if (q == end || (*q != '"' && *q != '\'') || q + 1 != av.buf) {
            return false;
        }
        Attr attr;
        attr.name = (char*)an.buf;
        attr.nameEnd = (char*)an.buf + an.len;
        attr.value = (char*)av.buf;
        attr.valueEnd = (char*)av.buf + av.len;
        if (!AddAttr(attr)) {
            return false;
        }
        p = attr.valueEnd + 1;
    }

    /* tfsxml found the end of the tag, tinyxml2 needs ">" or "/>" after whitespaces */
    p = SkipWhiteSpace(p);
    if (p < end && *p == '>') {
        tag.closingType = XMLElement::OPEN;
        tag.end = p + 1;
    }
    else if (end - p >= 2 && p[0] == '/' && p[1] == '>') {
        tag.closingType = XMLElement::CLOSED;
        tag.end = p + 2;
    }
    else {
        return false;
    }
    tag.name = (char*)n.buf;
    tag.nameEnd = (char*)n.buf + n.len;
    return true;
}

/* Tag parsed as tinyxml2 does, return false if there is an error */
bool TfsxmlParser::ParseTag(char* lt, Tag& tag)
{
    char* p = SkipWhiteSpace(lt + 1);
    tag.closingType = XMLElement::OPEN;
    if (p < end && *p == '/') {
        tag.closingType = XMLElement::CLOSING;
        p++;
    }
    if (p == end || !XMLUtil::IsNameStartChar((unsigned char)*p)) {
        Error(XML_ERROR_PARSING, lt);
        return false;
    }
    tag.name = p;
    while (p < end && XMLUtil::IsNameChar((unsigned char)*p)) {
        p++;
    }
    tag.nameEnd = p;

    attrs.clear();
    for (;;) {
        p = SkipWhiteSpace(p);
        if (p == end) {
            ErrorName(XML_ERROR_PARSING_ELEMENT, lt, tag);
            return false;
        }
        if (XMLUtil::IsNameStartChar((unsigned char)*p)) {
            Attr attr;
            attr.name = p;
            while (p < end && XMLUtil::IsNameChar((unsigned char)*p)) {
                p++;
            }
            attr.nameEnd = p;
            bool ok = p < end;
            if (ok) {
                p = SkipWhiteSpace(p);
                ok = p < end && *p == '=';
            }
            if (ok) {
                p = SkipWhiteSpace(p + 1);
                ok = p < end && (*p == '"' || *p == '\'');
            }
            if (ok) {
                char* q = (char*)memchr(p + 1, *p, (size_t)(end - (p + 1)));
                ok = q != 0;
                if (ok) {
                    attr.value = p + 1;
                    attr.valueEnd = q;
                    p = q + 1;
                    ok = AddAttr(attr);
                }
            }
            if (!ok) {
                ErrorName(XML_ERROR_PARSING_ATTRIBUTE, attr.name, tag);
                return false;
            }
            continue;
        }
        if (*p == '>') {
            tag.end = p + 1;
            return true;
        }
        if (*p == '/' && end - p >= 2 && p[1] == '>') {
            tag.closingType = XMLElement::CLOSED; /* Also for "</a/>" */
            tag.end = p + 2;
            return true;
        }
        Error(XML_ERROR_PARSING_ELEMENT, lt);
        return false;
    }
}

/* Attribute of the current tag, return false if the name is already used */
bool TfsxmlParser::AddAttr(const Attr& attr)
{
    const size_t len = (size_t)(attr.nameEnd - attr.name);
    for (size_t i = 0; i < attrs.size(); i++) {
        if ((size_t)(attrs[i].nameEnd - attrs[i].name) == len && !memcmp(attrs[i].name, attr.name, len)) {
            return false;
        }
    }
    attrs.push_back(attr);
    return true;
}

/* Tag (attributes are in attrs), return false if the parsing is finished */
bool TfsxmlParser::HandleTag(char* lt, const Tag& tag)
{
    if (tag.closingType == XMLElement::CLOSING) {
        if (parent == doc) {
            return false; /* tinyxml2 ends the parsing without error */
        }
        XMLElement* element = static_cast<XMLElement*>(parent);
        const size_t len = (size_t)(tag.nameEnd - tag.name);
        if ((size_t)(element->_valueEnd - element->_value) != len || memcmp(element->_value, tag.name, len)) {
            Error(XML_ERROR_MISMATCHED_ELEMENT, element->_parsePos, "XMLElement name=%s", element->Name());
            return false;
        }
        parent = element->_parent;
        depth--;
        if (parent == doc) {
            lastTop = element;
        }
        return true;
    }

    XMLElement* element = new (doc->Alloc(sizeof(XMLElement))) XMLElement(doc);
    element->_closingType = tag.closingType;
    AddNode(element, tag.name, tag.nameEnd, lt, 0);
    XMLAttribute** next = &element->_rootAttribute;
    for (size_t i = 0; i < attrs.size(); i++) {
        XMLAttribute* attribute = new (doc->Alloc(sizeof(XMLAttribute))) XMLAttribute;
        attribute->_document = doc;
        attribute->_next = 0;
        attribute->_name = attrs[i].name;
        attribute->_nameEnd = attrs[i].nameEnd;
        attribute->_value = attrs[i].value;
        attribute->_valueEnd = attrs[i].valueEnd;
        attribute->_parsePos = attrs[i].name;
        attribute->_flags = attrFlags | TFSXML_RAW | TFSXML_RAW_NAME;
        *next = attribute;
        next = &attribute->_next;
    }
    if (tag.closingType == XMLElement::CLOSED) {
        if (parent == doc) {
            lastTop = element;
        }
        return true;
    }
    if (tag.end == end) {
        Error(XML_ERROR_MISMATCHED_ELEMENT, lt, "XMLElement name=%s", element->Name());
        return false;
    }
    if (++depth == TINYXML2_MAX_ELEMENT_DEPTH) {
        Error(XML_ELEMENT_DEPTH_EXCEEDED, tag.end, "Element nesting is too deep.");
        return false;
    }
    parent = element;
    return true;
}

/* End of the data, return false (parsing is finished) */
bool TfsxmlParser::LevelEnd()
{
    if (parent != doc) {
        Error(XML_ERROR_PARSING, parent->_parsePos); /* Element not closed */
    }
    return false;
}

void TfsxmlParser::AddNode(XMLNode* node, char* start, char* e, char* pos, unsigned flags)
{
    node->_value = start;
    node->_valueEnd = e;
    node->_flags = flags | TFSXML_RAW;
    node->_parsePos = pos;
    node->_parent = parent;
    node->_prev = parent->_lastChild;
    if (parent->_lastChild) {
        parent->_lastChild->_next = node;
    }
    else {
        parent->_firstChild = node;
    }
    parent->_lastChild = node;
    if (parent == doc) {
        if (node->_type != XMLNode::TFSXML_DECLARATION) {
            onlyDeclarations = false;
        }
        if (node->_type != XMLNode::TFSXML_ELEMENT) {
            lastTop = node;
        }
    }
}

void TfsxmlParser::AddText(char* start, char* e, char* pos, bool cdata)
{
    XMLText* text = new (doc->Alloc(sizeof(XMLText))) XMLText(doc);
    text->_isCData = cdata;
    AddNode(text, start, e, pos, cdata ? (unsigned)TFSXML_NEWLINES : textFlags);
}

/* Declarations are accepted only at the document level before any other node */
bool TfsxmlParser::AddDeclaration(char* start, char* e, char* pos)
{
    if (parent != doc || !onlyDeclarations) {
        Error(XML_ERROR_PARSING_DECLARATION, pos, "XMLDeclaration value=%s", doc->Decode(start, e, TFSXML_NEWLINES));
        return false;
    }
    AddNode(new (doc->Alloc(sizeof(XMLDeclaration))) XMLDeclaration(doc), start, e, pos, TFSXML_NEWLINES);
    return true;
}

void TfsxmlParser::Error(XMLError error, const char* pos, const char* format, const char* arg)
{
    doc->SetError(error, doc->LineNum(pos), format, arg);
}

void TfsxmlParser::ErrorName(XMLError error, const char* pos, const Tag& tag)
{
    const std::string name(tag.name, tag.nameEnd);
    Error(error, pos, "XMLElement name=%s", name.c_str());
}

/** -------------------------------------------------------------------------
        XMLUtil
------------------------------------------------------------------------- **/

const char* XMLUtil::writeBoolTrue = "true";
const char* XMLUtil::writeBoolFalse = "false";

const char* XMLUtil::SkipWhiteSpace(const char* p, int* curLineNumPtr)
{
    while (IsWhiteSpace(*p)) {
        if (curLineNumPtr && *p == '\n') {
            ++*curLineNumPtr;
        }
        ++p;
    }
    return p;
}

const char* XMLUtil::ReadBOM(const char* p, bool* hasBOM)
{
    *hasBOM = (unsigned char)p[0] == 0xEF && (unsigned char)p[1] == 0xBB && (unsigned char)p[2] == 0xBF;
    return *hasBOM ? p + 3 : p;
}

const char* XMLUtil::GetCharacterRef(const char* p, char* value, int* length)
{
    *length = 0;
    if (p[1] == '#' && p[2]) {
        return tfsxml_character_reference(p, p + strlen(p), value, length);
    }
    return p + 1;
}

void XMLUtil::ConvertUTF32ToUTF8(unsigned long input, char* output, int* length)
{
    if (input < 0x80) {
        *length = 1;
        output[0] = (char)input;
    }
    else if (input < 0x800) {
        *length = 2;
        output[0] = (char)(0xC0 | (input >> 6));
        output[1] = (char)(0x80 | (input & 0x3F));
    }
    else if (input < 0x10000) {
        *length = 3;
        output[0] = (char)(0xE0 | (input >> 12));
        output[1] = (char)(0x80 | ((input >> 6) & 0x3F));
        output[2] = (char)(0x80 | (input & 0x3F));
    }
    else if (input < 0x200000) {
        *length = 4;
        output[0] = (char)(0xF0 | (input >> 18));
        output[1] = (char)(0x80 | ((input >> 12) & 0x3F));
        output[2] = (char)(0x80 | ((input >> 6) & 0x3F));
        output[3] = (char)(0x80 | (input & 0x3F));
    }
    else {
        *length = 0;
    }
}

void XMLUtil::ToStr(int v, char* buffer, int bufferSize)       { snprintf(buffer, (size_t)bufferSize, "%d", v); }
void XMLUtil::ToStr(unsigned v, char* buffer, int bufferSize)  { snprintf(buffer, (size_t)bufferSize, "%u", v); }
void XMLUtil::ToStr(bool v, char* buffer, int bufferSize)      { snprintf(buffer, (size_t)bufferSize, "%s", v ? writeBoolTrue : writeBoolFalse); }
void XMLUtil::ToStr(float v, char* buffer, int bufferSize)     { snprintf(buffer, (size_t)bufferSize, "%.8g", v); }
void XMLUtil::ToStr(double v, char* buffer, int bufferSize)    { snprintf(buffer, (size_t)bufferSize, "%.17g", v); }
void XMLUtil::ToStr(int64_t v, char* buffer, int bufferSize)   { snprintf(buffer, (size_t)bufferSize, "%lld", (long long)v); }

bool XMLUtil::ToInt(const char* str, int* value)            { return TFSXML_SSCANF(str, "%d", value) == 1; }
bool XMLUtil::ToUnsigned(const char* str, unsigned* value)  { return TFSXML_SSCANF(str, "%u", value) == 1; }
bool XMLUtil::ToFloat(const char* str, float* value)        { return TFSXML_SSCANF(str, "%f", value) == 1; }
bool XMLUtil::ToDouble(const char* str, double* value)      { return TFSXML_SSCANF(str, "%lf", value) == 1; }

bool XMLUtil::ToBool(const char* str, bool* value)
{
    int i = 0;
    if (ToInt(str, &i)) {
        *value = i != 0;
        return true;
    }
    if (StringEqual(str, "true")) {
        *value = true;
        return true;
    }
    if (StringEqual(str, "false")) {
        *value = false;
        return true;
    }
    return false;
}

bool XMLUtil::ToInt64(const char* str, int64_t* value)
{
    long long v = 0;
    if (TFSXML_SSCANF(str, "%lld", &v) == 1) {
        *value = (int64_t)v;
        return true;
    }
    return false;
}

void XMLUtil::SetBoolSerialization(const char* writeTrue, const char* writeFalse)
{
    writeBoolTrue = writeTrue ? writeTrue : "true";
    writeBoolFalse = writeFalse ? writeFalse : "false";
}

/** -------------------------------------------------------------------------
        XMLNode
------------------------------------------------------------------------- **/

XMLNode::XMLNode(XMLDocument* document, NodeType type) :
    _document(document),
    _parent(0),
    _firstChild(0),
    _lastChild(0),
    _prev(0),
    _next(0),
    _userData(0),
    _value(0),
    _valueEnd(0),
    _parsePos(0),
    _flags(0),
    _type(type)
{
}

const char* XMLNode::Value() const
{
    if (_flags & TFSXML_RAW) {
        _value = _document->Decode(_value, _valueEnd, _flags & ~(unsigned)TFSXML_RAW);
        _flags = 0;
    }
    return _value;
}

int XMLNode::GetLineNum() const
{
    if (_type == TFSXML_DOCUMENT) {
        return _document->_parseLineNum;
    }
    return _document->LineNum(_parsePos);
}

const XMLElement* XMLNode::FirstChildElement(const char* name) const
{
    for (const XMLNode* node = _firstChild; node; node = node->_next) {
        if (node->_type == TFSXML_ELEMENT && (!name || static_cast<const XMLElement*>(node)->NameIs(name))) {
            return static_cast<const XMLElement*>(node);
        }
    }
    return 0;
}

const XMLElement* XMLNode::LastChildElement(const char* name) const
{
    for (const XMLNode* node = _lastChild; node; node = node->_prev) {
        if (node->_type == TFSXML_ELEMENT && (!name || static_cast<const XMLElement*>(node)->NameIs(name))) {
            return static_cast<const XMLElement*>(node);
        }
    }
    return 0;
}

const XMLElement* XMLNode::PreviousSiblingElement(const char* name) const
{
    for (const XMLNode* node = _prev; node; node = node->_prev) {
        if (node->_type == TFSXML_ELEMENT && (!name || static_cast<const XMLElement*>(node)->NameIs(name))) {
            return static_cast<const XMLElement*>(node);
        }
    }
    return 0;
}

const XMLElement* XMLNode::NextSiblingElement(const char* name) const
{
    for (const XMLNode* node = _next; node; node = node->_next) {
        if (node->_type == TFSXML_ELEMENT && (!name || static_cast<const XMLElement*>(node)->NameIs(name))) {
            return static_cast<const XMLElement*>(node);
        }
    }
    return 0;
}

bool XMLNode::Accept(XMLVisitor* visitor) const
{
    switch (_type) {
        case TFSXML_DOCUMENT: {
            const XMLDocument& document = *static_cast<const XMLDocument*>(this);
            if (visitor->VisitEnter(document)) {
                for (const XMLNode* node = _firstChild; node && node->Accept(visitor); node = node->_next) {
                }
            }
            return visitor->VisitExit(document);
        }
        case TFSXML_ELEMENT: {
            const XMLElement& element = *static_cast<const XMLElement*>(this);
            if (visitor->VisitEnter(element, element._rootAttribute)) {
                for (const XMLNode* node = _firstChild; node && node->Accept(visitor); node = node->_next) {
                }
            }
            return visitor->VisitExit(element);
        }
        case TFSXML_TEXT:           return visitor->Visit(*static_cast<const XMLText*>(this));
        case TFSXML_COMMENT:        return visitor->Visit(*static_cast<const XMLComment*>(this));
        case TFSXML_DECLARATION:    return visitor->Visit(*static_cast<const XMLDeclaration*>(this));
        default:                    return visitor->Visit(*static_cast<const XMLUnknown*>(this));
    }
}

/** -------------------------------------------------------------------------
        XMLAttribute
------------------------------------------------------------------------- **/

const char* XMLAttribute::Name() const
{
    if (_flags & TFSXML_RAW_NAME) {
        _name = _document->Decode(_name, _nameEnd, 0);
        _flags &= ~(unsigned)TFSXML_RAW_NAME;
    }
    return _name;
}

const char* XMLAttribute::Value() const
{
    if (_flags & TFSXML_RAW) {
        _value = _document->Decode(_value, _valueEnd, _flags & (TFSXML_ENTITIES | TFSXML_NEWLINES));
        _flags &= ~(unsigned)(TFSXML_RAW | TFSXML_ENTITIES | TFSXML_NEWLINES);
    }
    return _value;
}

int XMLAttribute::GetLineNum() const
{
    return _document->LineNum(_parsePos);
}

bool XMLAttribute::NameIs(const char* name) const
{
    if (_flags & TFSXML_RAW_NAME) {
        return tfsxml_name_is(_name, _nameEnd, name);
    }
    return !strcmp(_name, name);
}

XMLError XMLAttribute::QueryIntValue(int* value) const          { return XMLUtil::ToInt(Value(), value) ? XML_SUCCESS : XML_WRONG_ATTRIBUTE_TYPE; }
XMLError XMLAttribute::QueryUnsignedValue(unsigned* value) const { return XMLUtil::ToUnsigned(Value(), value) ? XML_SUCCESS : XML_WRONG_ATTRIBUTE_TYPE; }
XMLError XMLAttribute::QueryInt64Value(int64_t* value) const    { return XMLUtil::ToInt64(Value(), value) ? XML_SUCCESS : XML_WRONG_ATTRIBUTE_TYPE; }
XMLError XMLAttribute::QueryBoolValue(bool* value) const        { return XMLUtil::ToBool(Value(), value) ? XML_SUCCESS : XML_WRONG_ATTRIBUTE_TYPE; }
XMLError XMLAttribute::QueryDoubleValue(double* value) const    { return XMLUtil::ToDouble(Value(), value) ? XML_SUCCESS : XML_WRONG_ATTRIBUTE_TYPE; }
XMLError XMLAttribute::QueryFloatValue(float* value) const      { return XMLUtil::ToFloat(Value(), value) ? XML_SUCCESS : XML_WRONG_ATTRIBUTE_TYPE; }

/** -------------------------------------------------------------------------
        XMLElement
------------------------------------------------------------------------- **/

bool XMLElement::NameIs(const char* name) const
{
    if (_flags & TFSXML_RAW) {
        return tfsxml_name_is(_value, _valueEnd, name);
    }
    return !strcmp(_value, name);
}

const XMLAttribute* XMLElement::FindAttribute(const char* name) const
{
    for (const XMLAttribute* a = _rootAttribute; a; a = a->_next) {
        if (a->NameIs(name)) {
            return a;
        }
    }
    return 0;
}

const char* XMLElement::Attribute(const char* name, const char* value) const
{
    const XMLAttribute* a = FindAttribute(name);
    if (!a || (value && !XMLUtil::StringEqual(a->Value(), value))) {
        return 0;
    }
    return a->Value();
}

XMLError XMLElement::QueryIntAttribute(const char* name, int* value) const
{
    const XMLAttribute* a = FindAttribute(name);
    return a ? a->QueryIntValue(value) : XML_NO_ATTRIBUTE;
}

XMLError XMLElement::QueryUnsignedAttribute(const char* name, unsigned int* value) const
{
    const XMLAttribute* a = FindAttribute(name);
    return a ? a->QueryUnsignedValue(value) : XML_NO_ATTRIBUTE;
}

XMLError XMLElement::QueryInt64Attribute(const char* name, int64_t* value) const
{
    const XMLAttribute* a = FindAttribute(name);
    return a ? a->QueryInt64Value(value) : XML_NO_ATTRIBUTE;
}

XMLError XMLElement::QueryBoolAttribute(const char* name, bool* value) const
{
    const XMLAttribute* a = FindAttribute(name);
    return a ? a->QueryBoolValue(value) : XML_NO_ATTRIBUTE;
}

XMLError XMLElement::QueryDoubleAttribute(const char* name, double* value) const
{
    const XMLAttribute* a = FindAttribute(name);
    return a ? a->QueryDoubleValue(value) : XML_NO_ATTRIBUTE;
}

XMLError XMLElement::QueryFloatAttribute(const char* name, float* value) const
{
    const XMLAttribute* a = FindAttribute(name);
    return a ? a->QueryFloatValue(value) : XML_NO_ATTRIBUTE;
}

XMLError XMLElement::QueryStringAttribute(const char* name, const char** value) const
{
    const XMLAttribute* a = FindAttribute(name);
    if (!a) {
        return XML_NO_ATTRIBUTE;
    }
    *value = a->Value();
    return XML_SUCCESS;
}

const char* XMLElement::GetText() const
{
    if (_firstChild && _firstChild->_type == TFSXML_TEXT) {
        return _firstChild->Value();
    }
    return 0;
}

XMLError XMLElement::QueryIntText(int* ival) const
{
    const char* t = GetText();
    return !t ? XML_NO_TEXT_NODE : XMLUtil::ToInt(t, ival) ? XML_SUCCESS : XML_CAN_NOT_CONVERT_TEXT;
}

XMLError XMLElement::QueryUnsignedText(unsigned* uval) const
{
    const char* t = GetText();
    return !t ? XML_NO_TEXT_NODE : XMLUtil::ToUnsigned(t, uval) ? XML_SUCCESS : XML_CAN_NOT_CONVERT_TEXT;
}

XMLError XMLElement::QueryInt64Text(int64_t* ival) const
{
    const char* t = GetText();
    return !t ? XML_NO_TEXT_NODE : XMLUtil::ToInt64(t, ival) ? XML_SUCCESS : XML_CAN_NOT_CONVERT_TEXT;
}

XMLError XMLElement::QueryBoolText(bool* bval) const
{
    const char* t = GetText();
    return !t ? XML_NO_TEXT_NODE : XMLUtil::ToBool(t, bval) ? XML_SUCCESS : XML_CAN_NOT_CONVERT_TEXT;
}

XMLError XMLElement::QueryDoubleText(double* dval) const
{
    const char* t = GetText();
    return !t ? XML_NO_TEXT_NODE : XMLUtil::ToDouble(t, dval) ? XML_SUCCESS : XML_CAN_NOT_CONVERT_TEXT;
}

XMLError XMLElement::QueryFloatText(float* fval) const
{
    const char* t = GetText();
    return !t ? XML_NO_TEXT_NODE : XMLUtil::ToFloat(t, fval) ? XML_SUCCESS : XML_CAN_NOT_CONVERT_TEXT;
}

/** -------------------------------------------------------------------------
        XMLDocument
------------------------------------------------------------------------- **/

static const char* const tfsxml_error_names[XML_ERROR_COUNT] = {
    "XML_SUCCESS",
    "XML_NO_ATTRIBUTE",
    "XML_WRONG_ATTRIBUTE_TYPE",
    "XML_ERROR_FILE_NOT_FOUND",
    "XML_ERROR_FILE_COULD_NOT_BE_OPENED",
    "XML_ERROR_FILE_READ_ERROR",
    "UNUSED_XML_ERROR_ELEMENT_MISMATCH",
    "XML_ERROR_PARSING_ELEMENT",
    "XML_ERROR_PARSING_ATTRIBUTE",
    "UNUSED_XML_ERROR_IDENTIFYING_TAG",
    "XML_ERROR_PARSING_TEXT",
    "XML_ERROR_PARSING_CDATA",
    "XML_ERROR_PARSING_COMMENT",
    "XML_ERROR_PARSING_DECLARATION",
    "XML_ERROR_PARSING_UNKNOWN",
    "XML_ERROR_EMPTY_DOCUMENT",
    "XML_ERROR_MISMATCHED_ELEMENT",
    "XML_ERROR_PARSING",
    "XML_CAN_NOT_CONVERT_TEXT",
    "XML_NO_TEXT_NODE",
    "XML_ELEMENT_DEPTH_EXCEEDED",
};

XMLDocument::XMLDocument(bool processEntities, Whitespace whitespaceMode) :
    XMLNode(0, TFSXML_DOCUMENT),
    _writeBOM(false),
    _processEntities(processEntities),
    _whitespaceMode(whitespaceMode),
    _errorID(XML_SUCCESS),
    _errorLineNum(0),
    _parseLineNum(0),
    _charBuffer(0),
    _linePos(0),
    _lineNum(0),
    _blockPos(0),
    _blockLeft(0)
{
    _document = this;
}

XMLDocument::~XMLDocument()
{
    FreeAll();
    delete[] _charBuffer;
}

void XMLDocument::Clear()
{
    _firstChild = 0;
    _lastChild = 0;
    FreeAll();
    ClearError();
    delete[] _charBuffer;
    _charBuffer = 0;
    _linePos = 0;
}

XMLError XMLDocument::Parse(const char* xml, size_t nBytes)
{
    Clear();
    if (!nBytes || !xml || !*xml) {
        SetError(XML_ERROR_EMPTY_DOCUMENT, 0, 0);
        return _errorID;
    }
    if (nBytes == (size_t)(-1)) {
        nBytes = strlen(xml);
    }
    _charBuffer = new char[nBytes + 1];
    memcpy(_charBuffer, xml, nBytes);
    _charBuffer[nBytes] = '\0';
    ParseBuffer(nBytes, false);
    return _errorID;
}

XMLError XMLDocument::LoadFile(const char* filename)
{
    if (!filename) {
        SetError(XML_ERROR_FILE_COULD_NOT_BE_OPENED, 0, "filename=<null>");
        return _errorID;
    }
    Clear();
    FILE* fp;
#if defined(_MSC_VER)
    if (fopen_s(&fp, filename, "rb")) {
        fp = 0;
    }
#else
    fp = fopen(filename, "rb");
#endif
    if (!fp) {
        SetError(XML_ERROR_FILE_NOT_FOUND, 0, "filename=%s", filename);
        return _errorID;
    }
    LoadFile(fp);
    fclose(fp);
    return _errorID;
}

XMLError XMLDocument::LoadFile(FILE* fp)
{
    Clear();
    fseek(fp, 0, SEEK_SET);
    if (fgetc(fp) == EOF && ferror(fp)) {
        SetError(XML_ERROR_FILE_READ_ERROR, 0, 0);
        return _errorID;
    }
    fseek(fp, 0, SEEK_END);
    const long fileLength = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (fileLength == -1L || (unsigned long)fileLength >= (size_t)-1) {
        SetError(XML_ERROR_FILE_READ_ERROR, 0, 0);
        return _errorID;
    }
    if (!fileLength) {
        SetError(XML_ERROR_EMPTY_DOCUMENT, 0, 0);
        return _errorID;
    }
    const size_t size = (size_t)fileLength;
    _charBuffer = new char[size + 1];
    if (fread(_charBuffer, 1, size, fp) != size) {
        SetError(XML_ERROR_FILE_READ_ERROR, 0, 0);
        return _errorID;
    }
    _charBuffer[size] = '\0';
    ParseBuffer(size, true);
    return _errorID;
}

XMLError XMLDocument::SaveFile(const char* filename, bool compact)
{
    if (!filename) {
        SetError(XML_ERROR_FILE_COULD_NOT_BE_OPENED, 0, "filename=<null>");
        return _errorID;
    }
    FILE* fp;
#if defined(_MSC_VER)
    if (fopen_s(&fp, filename, "w")) {
        fp = 0;
    }
#else
    fp = fopen(filename, "w");
#endif
    if (!fp) {
        SetError(XML_ERROR_FILE_COULD_NOT_BE_OPENED, 0, "filename=%s", filename);
        return _errorID;
    }
    SaveFile(fp, compact);
    fclose(fp);
    return _errorID;
}

XMLError XMLDocument::SaveFile(FILE* fp, bool compact)
{
    ClearError();
    XMLPrinter stream(fp, compact);
    Print(&stream);
    return _errorID;
}

/* Parsing of _charBuffer (size bytes followed by NUL), content after a NUL is ignored */
void XMLDocument::ParseBuffer(size_t size, bool keepOnError)
{
    _parseLineNum = 1;
    char* end = (char*)memchr(_charBuffer, '\0', size);
    TfsxmlParser parser(this, _charBuffer, end ? end : _charBuffer + size);
    parser.Run();
    if (Error()) {
        if (keepOnError && parser.lastTop) {
            /* tinyxml2 LoadFile() keeps the complete nodes at the document level */
            parser.lastTop->_next = 0;
            _lastChild = parser.lastTop;
        }
        else {
            _firstChild = 0;
            _lastChild = 0;
            if (!keepOnError) {
                FreeAll();
            }
        }
    }
}

void XMLDocument::Print(XMLPrinter* streamer) const
{
    if (streamer) {
        Accept(streamer);
    }
    else {
        XMLPrinter stdoutStreamer(stdout);
        Accept(&stdoutStreamer);
    }
}

void XMLDocument::SetError(XMLError error, int lineNum, const char* format, ...)
{
    char buffer[1000];
    _errorID = error;
    _errorLineNum = lineNum;
    snprintf(buffer, sizeof(buffer), "Error=%s ErrorID=%d (0x%x) Line number=%d", ErrorIDToName(error), (int)error, (unsigned)error, lineNum);
    if (format) {
        size_t len = strlen(buffer);
        snprintf(buffer + len, sizeof(buffer) - len, ": ");
        len = strlen(buffer);
        va_list va;
        va_start(va, format);
        vsnprintf(buffer + len, sizeof(buffer) - len, format, va);
        va_end(va);
    }
    _errorStr = buffer;
}

const char* XMLDocument::ErrorIDToName(XMLError errorID)
{
    return (unsigned)errorID < XML_ERROR_COUNT ? tfsxml_error_names[errorID] : "";
}

const char* XMLDocument::ErrorName() const
{
    return ErrorIDToName(_errorID);
}

const char* XMLDocument::ErrorStr() const
{
    return _errorStr.c_str();
}

void XMLDocument::PrintError() const
{
    printf("%s\n", ErrorStr());
}

/* Memory for nodes, attributes and decoded values, freed when the document is cleared */
void* XMLDocument::Alloc(size_t size) const
{
    size = (size + 7) & ~(size_t)7;
    if (size > _blockLeft) {
        if (size > 4096) {
            char* block = new char[size];
            _blocks.push_back(block);
            return block;
        }
        const size_t count = _blocks.size();
        const size_t blockSize = (size_t)16384 << (count < 24 ? count / 4 : 6);
        _blockPos = new char[blockSize];
        _blockLeft = blockSize;
        _blocks.push_back(_blockPos);
    }
    void* p = _blockPos;
    _blockPos += size;
    _blockLeft -= size;
    return p;
}

void XMLDocument::FreeAll()
{
    for (size_t i = 0; i < _blocks.size(); i++) {
        delete[] _blocks[i];
    }
    _blocks.clear();
    _blockPos = 0;
    _blockLeft = 0;
}

/* Copy of raw content, decoded as tinyxml2 does */
const char* XMLDocument::Decode(const char* start, const char* end, unsigned flags) const
{
    char* out = (char*)Alloc((size_t)(end - start) + 1);
    size_t q = 0;
    if (!(flags & (TFSXML_ENTITIES | TFSXML_NEWLINES))) {
        q = (size_t)(end - start);
        memcpy(out, start, q);
    }
    else {
        const char* p = start;
        while (p < end) {
            const char c = *p;
            if ((flags & TFSXML_NEWLINES) && (c == '\r' || c == '\n')) {
                p += (end - p >= 2 && p[1] == (c == '\r' ? '\n' : '\r')) ? 2 : 1;
                out[q++] = '\n';
            }
            else if ((flags & TFSXML_ENTITIES) && c == '&') {
                if (end - p >= 2 && p[1] == '#') {
                    char utf8[4];
                    int length;
                    const char* next = tfsxml_character_reference(p, end, utf8, &length);
                    if (next) {
                        memcpy(out + q, utf8, (size_t)length);
                        q += (size_t)length;
                        p = next;
                    }
                    else {
                        out[q++] = '&';
                        p++;
                    }
                }
                else {
                    size_t i;
                    for (i = 0; i < 5; i++) {
                        const size_t length = tfsxml_entities[i].length;
                        if ((size_t)(end - p) > length + 1 && !memcmp(p + 1, tfsxml_entities[i].pattern, length) && p[length + 1] == ';') {
                            break;
                        }
                    }
                    if (i < 5) {
                        out[q++] = tfsxml_entities[i].value;
                        p += tfsxml_entities[i].length + 2;
                    }
                    else {
                        /* tinyxml2 6.2.0 skips the byte at this position of its in-place output, which keeps the original byte */
                        out[q] = start[q];
                        q++;
                        p++;
                    }
                }
            }
            else {
                out[q++] = c;
                p++;
            }
        }
    }
    out[q] = '\0';
    if (flags & TFSXML_COLLAPSE) {
        return tfsxml_collapse(out);
    }
    return out;
}

/* Line number of a position in the parsed content, computed from the previous computed position if possible */
int XMLDocument::LineNum(const char* pos) const
{
    if (!_charBuffer || !pos) {
        return 0;
    }
    if (!_linePos || pos < _linePos) {
        _linePos = _charBuffer;
        _lineNum = 1;
    }
    const char* p = _linePos;
    while (p < pos) {
        const char* q = (const char*)memchr(p, '\n', (size_t)(pos - p));
        if (!q) {
            break;
        }
        _lineNum++;
        p = q + 1;
    }
    _linePos = pos;
    return _lineNum;
}

/** -------------------------------------------------------------------------
        XMLPrinter
------------------------------------------------------------------------- **/

XMLPrinter::XMLPrinter(FILE* file, bool compact, int depth) :
    _elementJustOpened(false),
    _firstElement(true),
    _fp(file),
    _depth(depth),
    _textDepth(-1),
    _processEntities(true),
    _compactMode(compact),
    _buffer(1, '\0')
{
}

void XMLPrinter::Print(const char* format, ...)
{
    va_list va;
    va_start(va, format);
    if (_fp) {
        vfprintf(_fp, format, va);
    }
    else {
        va_list va2;
        va_copy(va2, va);
        const int len = vsnprintf(0, 0, format, va2);
        va_end(va2);
        if (len > 0) {
            const size_t size = _buffer.size();
            _buffer.resize(size + (size_t)len);
            vsnprintf(&_buffer[size - 1], (size_t)len + 1, format, va);
        }
    }
    va_end(va);
}

void XMLPrinter::Write(const char* data, size_t size)
{
    if (_fp) {
        fwrite(data, 1, size, _fp);
    }
    else {
        _buffer.insert(_buffer.end() - 1, data, data + size);
    }
}

void XMLPrinter::Putc(char ch)
{
    if (_fp) {
        fputc(ch, _fp);
    }
    else {
        _buffer.insert(_buffer.end() - 1, ch);
    }
}

void XMLPrinter::PrintSpace(int depth)
{
    for (int i = 0; i < depth; i++) {
        Write("    ", 4);
    }
}

void XMLPrinter::PrintString(const char* p, bool restricted)
{
    if (!_processEntities) {
        return; /* tinyxml2 6.2.0 prints nothing */
    }
    const char* q = p;
    for (; *q; q++) {
        const char* entity;
        switch (*q) {
            case '&':  entity = "&amp;"; break;
            case '<':  entity = "&lt;"; break;
            case '>':  entity = "&gt;"; break;
            case '"':  entity = restricted ? 0 : "&quot;"; break;
            case '\'': entity = restricted ? 0 : "&apos;"; break;
            default:   entity = 0;
        }
        if (entity) {
            Write(p, (size_t)(q - p));
            Write(entity);
            p = q + 1;
        }
    }
    if (p < q) {
        Write(p, (size_t)(q - p));
    }
}

void XMLPrinter::PushHeader(bool writeBOM, bool writeDec)
{
    if (writeBOM) {
        Write("\xEF\xBB\xBF", 3);
    }
    if (writeDec) {
        PushDeclaration("xml version=\"1.0\"");
    }
}

void XMLPrinter::OpenElement(const char* name, bool compactMode)
{
    SealElementIfJustOpened();
    _stack.push_back(name);
    if (_textDepth < 0 && !_firstElement && !compactMode) {
        Putc('\n');
    }
    if (!compactMode) {
        PrintSpace(_depth);
    }
    Putc('<');
    Write(name);
    _elementJustOpened = true;
    _firstElement = false;
    _depth++;
}

void XMLPrinter::PushAttribute(const char* name, const char* value)
{
    Putc(' ');
    Write(name);
    Write("=\"", 2);
    PrintString(value, false);
    Putc('"');
}

void XMLPrinter::PushAttribute(const char* name, int v)       { char buf[200]; XMLUtil::ToStr(v, buf, sizeof(buf)); PushAttribute(name, buf); }
void XMLPrinter::PushAttribute(const char* name, unsigned v)  { char buf[200]; XMLUtil::ToStr(v, buf, sizeof(buf)); PushAttribute(name, buf); }
void XMLPrinter::PushAttribute(const char* name, int64_t v)   { char buf[200]; XMLUtil::ToStr(v, buf, sizeof(buf)); PushAttribute(name, buf); }
void XMLPrinter::PushAttribute(const char* name, bool v)      { char buf[200]; XMLUtil::ToStr(v, buf, sizeof(buf)); PushAttribute(name, buf); }
void XMLPrinter::PushAttribute(const char* name, double v)    { char buf[200]; XMLUtil::ToStr(v, buf, sizeof(buf)); PushAttribute(name, buf); }

void XMLPrinter::CloseElement(bool compactMode)
{
    _depth--;
    const char* name = _stack.empty() ? "" : _stack.back();
    if (!_stack.empty()) {
        _stack.pop_back();
    }
    if (_elementJustOpened) {
        Write("/>", 2);
    }
    else {
        if (_textDepth < 0 && !compactMode) {
            Putc('\n');
            PrintSpace(_depth);
        }
        Write("</", 2);
        Write(name);
        Putc('>');
    }
    if (_textDepth == _depth) {
        _textDepth = -1;
    }
    if (_depth == 0 && !compactMode) {
        Putc('\n');
    }
    _elementJustOpened = false;
}

void XMLPrinter::SealElementIfJustOpened()
{
    if (_elementJustOpened) {
        _elementJustOpened = false;
        Putc('>');
    }
}

void XMLPrinter::PushText(const char* text, bool cdata)
{
    _textDepth = _depth - 1;
    SealElementIfJustOpened();
    if (cdata) {
        Write("<![CDATA[", 9);
        Write(text);
        Write("]]>", 3);
    }
    else {
        PrintString(text, true);
    }
}

void XMLPrinter::PushText(int64_t value)   { char buf[200]; XMLUtil::ToStr(value, buf, sizeof(buf)); PushText(buf, false); }
void XMLPrinter::PushText(int value)       { char buf[200]; XMLUtil::ToStr(value, buf, sizeof(buf)); PushText(buf, false); }
void XMLPrinter::PushText(unsigned value)  { char buf[200]; XMLUtil::ToStr(value, buf, sizeof(buf)); PushText(buf, false); }
void XMLPrinter::PushText(bool value)      { char buf[200]; XMLUtil::ToStr(value, buf, sizeof(buf)); PushText(buf, false); }
void XMLPrinter::PushText(float value)     { char buf[200]; XMLUtil::ToStr(value, buf, sizeof(buf)); PushText(buf, false); }
void XMLPrinter::PushText(double value)    { char buf[200]; XMLUtil::ToStr(value, buf, sizeof(buf)); PushText(buf, false); }

void XMLPrinter::PushComment(const char* comment)
{
    SealElementIfJustOpened();
    if (_textDepth < 0 && !_firstElement && !_compactMode) {
        Putc('\n');
        PrintSpace(_depth);
    }
    _firstElement = false;
    Write("<!--", 4);
    Write(comment);
    Write("-->", 3);
}

void XMLPrinter::PushDeclaration(const char* value)
{
    SealElementIfJustOpened();
    if (_textDepth < 0 && !_firstElement && !_compactMode) {
        Putc('\n');
        PrintSpace(_depth);
    }
    _firstElement = false;
    Write("<?", 2);
    Write(value);
    Write("?>", 2);
}

void XMLPrinter::PushUnknown(const char* value)
{
    SealElementIfJustOpened();
    if (_textDepth < 0 && !_firstElement && !_compactMode) {
        Putc('\n');
        PrintSpace(_depth);
    }
    _firstElement = false;
    Write("<!", 2);
    Write(value);
    Putc('>');
}

bool XMLPrinter::VisitEnter(const XMLDocument& doc)
{
    _processEntities = doc.ProcessEntities();
    if (doc.HasBOM()) {
        PushHeader(true, false);
    }
    return true;
}

bool XMLPrinter::VisitEnter(const XMLElement& element, const XMLAttribute* attribute)
{
    const XMLElement* parentElem = element.Parent() ? element.Parent()->ToElement() : 0;
    OpenElement(element.Name(), parentElem ? CompactMode(*parentElem) : _compactMode);
    for (; attribute; attribute = attribute->Next()) {
        PushAttribute(attribute->Name(), attribute->Value());
    }
    return true;
}

bool XMLPrinter::VisitExit(const XMLElement& element)
{
    CloseElement(CompactMode(element));
    return true;
}

bool XMLPrinter::Visit(const XMLText& text)
{
    PushText(text.Value(), text.CData());
    return true;
}

bool XMLPrinter::Visit(const XMLComment& comment)
{
    PushComment(comment.Value());
    return true;
}

bool XMLPrinter::Visit(const XMLDeclaration& declaration)
{
    PushDeclaration(declaration.Value());
    return true;
}

bool XMLPrinter::Visit(const XMLUnknown& unknown)
{
    PushUnknown(unknown.Value());
    return true;
}

} /* namespace */
