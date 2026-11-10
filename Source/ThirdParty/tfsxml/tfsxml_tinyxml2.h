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
 * Replacement of the read part of tinyxml2 6.2.0 (classes and methods with the
 * same names in the tinyxml2 namespace): XMLDocument::Parse() and LoadFile()
 * build a read-only DOM, with the same decisions as tinyxml2 about the
 * validity of the content (same errors), the same nodes and the same values
 * (same handling of entities, line endings and whitespaces). XMLPrinter
 * prints the same output.
 *
 * Not provided: functions creating or modifying nodes (NewElement(),
 * InsertEndChild(), SetAttribute(), SetText(), DeleteChild()...) and clones.
 *
 * tfsxml finds the markup, the start and end tags are checked with the
 * grammar of tinyxml2, values are decoded the first time they are read.
 * The parsed content is copied (as tinyxml2 does), the content provided to
 * Parse() can be freed after the call.
 *
 * Define TFSXML_TINYXML2_NAMESPACE for using another namespace than tinyxml2
 * (e.g. for using it with the original tinyxml2 in the same program).
 */

#ifndef TFSXML_TINYXML2_H
#define TFSXML_TINYXML2_H

#include <climits>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <stdint.h>
#include <string>
#include <vector>

#ifndef TFSXML_TINYXML2_NAMESPACE
    #define TFSXML_TINYXML2_NAMESPACE tinyxml2
#endif

/* This is not the original tinyxml2 */
#define TFSXML_TINYXML2 1

/* Version of tinyxml2 which is emulated */
#ifndef TINYXML2_MAJOR_VERSION
    #define TINYXML2_MAJOR_VERSION 6
    #define TINYXML2_MINOR_VERSION 2
    #define TINYXML2_PATCH_VERSION 0
#endif

namespace TFSXML_TINYXML2_NAMESPACE
{

class XMLDocument;
class XMLElement;
class XMLAttribute;
class XMLComment;
class XMLText;
class XMLDeclaration;
class XMLUnknown;
class XMLPrinter;
class XMLNode;
class TfsxmlParser;

static const int TIXML2_MAJOR_VERSION = 6;
static const int TIXML2_MINOR_VERSION = 2;
static const int TIXML2_PATCH_VERSION = 0;

/* Deepest nesting level of element contents */
static const int TINYXML2_MAX_ELEMENT_DEPTH = 100;

/** -------------------------------------------------------------------------
        Errors
------------------------------------------------------------------------- **/

enum XMLError {
    XML_SUCCESS = 0,
    XML_NO_ATTRIBUTE,
    XML_WRONG_ATTRIBUTE_TYPE,
    XML_ERROR_FILE_NOT_FOUND,
    XML_ERROR_FILE_COULD_NOT_BE_OPENED,
    XML_ERROR_FILE_READ_ERROR,
    UNUSED_XML_ERROR_ELEMENT_MISMATCH,
    XML_ERROR_PARSING_ELEMENT,
    XML_ERROR_PARSING_ATTRIBUTE,
    UNUSED_XML_ERROR_IDENTIFYING_TAG,
    XML_ERROR_PARSING_TEXT,
    XML_ERROR_PARSING_CDATA,
    XML_ERROR_PARSING_COMMENT,
    XML_ERROR_PARSING_DECLARATION,
    XML_ERROR_PARSING_UNKNOWN,
    XML_ERROR_EMPTY_DOCUMENT,
    XML_ERROR_MISMATCHED_ELEMENT,
    XML_ERROR_PARSING,
    XML_CAN_NOT_CONVERT_TEXT,
    XML_NO_TEXT_NODE,
    XML_ELEMENT_DEPTH_EXCEEDED,

    XML_ERROR_COUNT
};

/* Handling of whitespaces in texts */
enum Whitespace {
    PRESERVE_WHITESPACE,
    COLLAPSE_WHITESPACE
};

/** -------------------------------------------------------------------------
        Visitor (see XMLNode::Accept)
------------------------------------------------------------------------- **/

class XMLVisitor
{
public:
    virtual ~XMLVisitor() {}

    virtual bool VisitEnter(const XMLDocument& /*doc*/)                                      { return true; }
    virtual bool VisitExit(const XMLDocument& /*doc*/)                                       { return true; }
    virtual bool VisitEnter(const XMLElement& /*element*/, const XMLAttribute* /*firstAttribute*/) { return true; }
    virtual bool VisitExit(const XMLElement& /*element*/)                                    { return true; }
    virtual bool Visit(const XMLDeclaration& /*declaration*/)                                { return true; }
    virtual bool Visit(const XMLText& /*text*/)                                              { return true; }
    virtual bool Visit(const XMLComment& /*comment*/)                                        { return true; }
    virtual bool Visit(const XMLUnknown& /*unknown*/)                                        { return true; }
};

/** -------------------------------------------------------------------------
        Utilities
------------------------------------------------------------------------- **/

class XMLUtil
{
public:
    static const char* SkipWhiteSpace(const char* p, int* curLineNumPtr);
    static char* SkipWhiteSpace(char* p, int* curLineNumPtr) { return const_cast<char*>(SkipWhiteSpace(const_cast<const char*>(p), curLineNumPtr)); }

    /* Space, tab, line feed, vertical tab, form feed, carriage return */
    static bool IsWhiteSpace(char p) { const unsigned char c = (unsigned char)p; return c == ' ' || (c >= 0x09 && c <= 0x0D); }
    /* Letters, ":", "_" and all bytes of UTF-8 sequences */
    static bool IsNameStartChar(unsigned char ch) { return ch >= 0x80 || ((ch | 0x20) >= 'a' && (ch | 0x20) <= 'z') || ch == ':' || ch == '_'; }
    static bool IsNameChar(unsigned char ch) { return IsNameStartChar(ch) || (ch >= '0' && ch <= '9') || ch == '.' || ch == '-'; }
    static bool StringEqual(const char* p, const char* q, int nChar = INT_MAX) { return p == q || !strncmp(p, q, (size_t)nChar); }
    static bool IsUTF8Continuation(char p) { return (p & 0x80) != 0; }

    static const char* ReadBOM(const char* p, bool* hasBOM);
    static const char* GetCharacterRef(const char* p, char* value, int* length);
    static void ConvertUTF32ToUTF8(unsigned long input, char* output, int* length);

    static void ToStr(int v, char* buffer, int bufferSize);
    static void ToStr(unsigned v, char* buffer, int bufferSize);
    static void ToStr(bool v, char* buffer, int bufferSize);
    static void ToStr(float v, char* buffer, int bufferSize);
    static void ToStr(double v, char* buffer, int bufferSize);
    static void ToStr(int64_t v, char* buffer, int bufferSize);

    static bool ToInt(const char* str, int* value);
    static bool ToUnsigned(const char* str, unsigned* value);
    static bool ToBool(const char* str, bool* value);
    static bool ToFloat(const char* str, float* value);
    static bool ToDouble(const char* str, double* value);
    static bool ToInt64(const char* str, int64_t* value);

    static void SetBoolSerialization(const char* writeTrue, const char* writeFalse);

private:
    static const char* writeBoolTrue;
    static const char* writeBoolFalse;
};

/** -------------------------------------------------------------------------
        Nodes
------------------------------------------------------------------------- **/

class XMLNode
{
    friend class XMLDocument;
    friend class XMLElement;
    friend class TfsxmlParser;

public:
    const XMLDocument* GetDocument() const      { return _document; }
    XMLDocument* GetDocument()                  { return _document; }

    XMLElement* ToElement();
    XMLText* ToText();
    XMLComment* ToComment();
    XMLDocument* ToDocument();
    XMLDeclaration* ToDeclaration();
    XMLUnknown* ToUnknown();
    const XMLElement* ToElement() const;
    const XMLText* ToText() const;
    const XMLComment* ToComment() const;
    const XMLDocument* ToDocument() const;
    const XMLDeclaration* ToDeclaration() const;
    const XMLUnknown* ToUnknown() const;

    /** Name of an element, content of a text, comment, declaration or unknown node, NULL for the document */
    const char* Value() const;

    /** Line of the node in the parsed content */
    int GetLineNum() const;

    const XMLNode* Parent() const               { return _parent; }
    XMLNode* Parent()                           { return _parent; }
    bool NoChildren() const                     { return !_firstChild; }

    const XMLNode* FirstChild() const           { return _firstChild; }
    XMLNode* FirstChild()                       { return _firstChild; }
    const XMLNode* LastChild() const            { return _lastChild; }
    XMLNode* LastChild()                        { return _lastChild; }
    const XMLNode* PreviousSibling() const      { return _prev; }
    XMLNode* PreviousSibling()                  { return _prev; }
    const XMLNode* NextSibling() const          { return _next; }
    XMLNode* NextSibling()                      { return _next; }

    /** Elements, with the specified name if name is not NULL */
    const XMLElement* FirstChildElement(const char* name = 0) const;
    XMLElement* FirstChildElement(const char* name = 0)                 { return const_cast<XMLElement*>(const_cast<const XMLNode*>(this)->FirstChildElement(name)); }
    const XMLElement* LastChildElement(const char* name = 0) const;
    XMLElement* LastChildElement(const char* name = 0)                  { return const_cast<XMLElement*>(const_cast<const XMLNode*>(this)->LastChildElement(name)); }
    const XMLElement* PreviousSiblingElement(const char* name = 0) const;
    XMLElement* PreviousSiblingElement(const char* name = 0)            { return const_cast<XMLElement*>(const_cast<const XMLNode*>(this)->PreviousSiblingElement(name)); }
    const XMLElement* NextSiblingElement(const char* name = 0) const;
    XMLElement* NextSiblingElement(const char* name = 0)                { return const_cast<XMLElement*>(const_cast<const XMLNode*>(this)->NextSiblingElement(name)); }

    /** Visit of this node and its children */
    bool Accept(XMLVisitor* visitor) const;

    void SetUserData(void* userData)            { _userData = userData; }
    void* GetUserData() const                   { return _userData; }

protected:
    enum NodeType {
        TFSXML_DOCUMENT,
        TFSXML_ELEMENT,
        TFSXML_TEXT,
        TFSXML_COMMENT,
        TFSXML_DECLARATION,
        TFSXML_UNKNOWN
    };

    XMLNode(XMLDocument* document, NodeType type);
    ~XMLNode() {}

    XMLDocument*                _document;
    XMLNode*                    _parent;
    XMLNode*                    _firstChild;
    XMLNode*                    _lastChild;
    XMLNode*                    _prev;
    XMLNode*                    _next;
    void*                       _userData;
    mutable const char*         _value;         /* Raw content in the parsed content, or decoded value */
    mutable const char*         _valueEnd;      /* End of the raw content */
    const char*                 _parsePos;      /* Position of the node in the parsed content */
    mutable unsigned            _flags;         /* Decoding to do */
    NodeType                    _type;

private:
    XMLNode(const XMLNode&);
    XMLNode& operator=(const XMLNode&);
};

class XMLText : public XMLNode
{
    friend class TfsxmlParser;

public:
    void SetCData(bool isCData)                 { _isCData = isCData; }
    bool CData() const                          { return _isCData; }

private:
    explicit XMLText(XMLDocument* document) : XMLNode(document, TFSXML_TEXT), _isCData(false) {}

    bool                        _isCData;
};

class XMLComment : public XMLNode
{
    friend class TfsxmlParser;

private:
    explicit XMLComment(XMLDocument* document) : XMLNode(document, TFSXML_COMMENT) {}
};

/** Processing instruction ("<?" ... "?>") */
class XMLDeclaration : public XMLNode
{
    friend class TfsxmlParser;

private:
    explicit XMLDeclaration(XMLDocument* document) : XMLNode(document, TFSXML_DECLARATION) {}
};

/** Other "<!" ... ">" content (e.g. DOCTYPE) */
class XMLUnknown : public XMLNode
{
    friend class TfsxmlParser;

private:
    explicit XMLUnknown(XMLDocument* document) : XMLNode(document, TFSXML_UNKNOWN) {}
};

/** -------------------------------------------------------------------------
        Attributes
------------------------------------------------------------------------- **/

class XMLAttribute
{
    friend class XMLElement;
    friend class TfsxmlParser;

public:
    const char* Name() const;
    const char* Value() const;
    int GetLineNum() const;
    const XMLAttribute* Next() const            { return _next; }

    /** Value converted to a number or a boolean, 0 or false if the conversion is not possible */
    int IntValue() const                        { int i = 0; QueryIntValue(&i); return i; }
    int64_t Int64Value() const                  { int64_t i = 0; QueryInt64Value(&i); return i; }
    unsigned UnsignedValue() const              { unsigned i = 0; QueryUnsignedValue(&i); return i; }
    bool BoolValue() const                      { bool b = false; QueryBoolValue(&b); return b; }
    double DoubleValue() const                  { double d = 0; QueryDoubleValue(&d); return d; }
    float FloatValue() const                    { float f = 0; QueryFloatValue(&f); return f; }

    /** Value converted to a number or a boolean, XML_WRONG_ATTRIBUTE_TYPE if the conversion is not possible */
    XMLError QueryIntValue(int* value) const;
    XMLError QueryUnsignedValue(unsigned int* value) const;
    XMLError QueryInt64Value(int64_t* value) const;
    XMLError QueryBoolValue(bool* value) const;
    XMLError QueryDoubleValue(double* value) const;
    XMLError QueryFloatValue(float* value) const;

private:
    XMLAttribute() {}
    XMLAttribute(const XMLAttribute&);
    void operator=(const XMLAttribute&);

    bool NameIs(const char* name) const;

    XMLDocument*                _document;
    XMLAttribute*               _next;
    mutable const char*         _name;          /* Raw name in the parsed content, or name ended by NUL */
    mutable const char*         _nameEnd;
    mutable const char*         _value;         /* Raw value in the parsed content, or decoded value */
    mutable const char*         _valueEnd;
    const char*                 _parsePos;      /* Position of the attribute in the parsed content */
    mutable unsigned            _flags;         /* Decoding to do */
};

/** -------------------------------------------------------------------------
        Elements
------------------------------------------------------------------------- **/

class XMLElement : public XMLNode
{
    friend class XMLNode;
    friend class TfsxmlParser;

public:
    const char* Name() const                    { return Value(); }

    /** Value of an attribute, or NULL if the attribute does not exist or (if value is not NULL) has another value */
    const char* Attribute(const char* name, const char* value = 0) const;

    /** Attribute converted to a number or a boolean, defaultValue if the attribute does not exist or the conversion is not possible */
    int IntAttribute(const char* name, int defaultValue = 0) const                 { int i = defaultValue; QueryIntAttribute(name, &i); return i; }
    unsigned UnsignedAttribute(const char* name, unsigned defaultValue = 0) const  { unsigned i = defaultValue; QueryUnsignedAttribute(name, &i); return i; }
    int64_t Int64Attribute(const char* name, int64_t defaultValue = 0) const       { int64_t i = defaultValue; QueryInt64Attribute(name, &i); return i; }
    bool BoolAttribute(const char* name, bool defaultValue = false) const          { bool b = defaultValue; QueryBoolAttribute(name, &b); return b; }
    double DoubleAttribute(const char* name, double defaultValue = 0) const        { double d = defaultValue; QueryDoubleAttribute(name, &d); return d; }
    float FloatAttribute(const char* name, float defaultValue = 0) const           { float f = defaultValue; QueryFloatAttribute(name, &f); return f; }

    /** Attribute converted to a number or a boolean, XML_NO_ATTRIBUTE or XML_WRONG_ATTRIBUTE_TYPE if not possible (value is not modified) */
    XMLError QueryIntAttribute(const char* name, int* value) const;
    XMLError QueryUnsignedAttribute(const char* name, unsigned int* value) const;
    XMLError QueryInt64Attribute(const char* name, int64_t* value) const;
    XMLError QueryBoolAttribute(const char* name, bool* value) const;
    XMLError QueryDoubleAttribute(const char* name, double* value) const;
    XMLError QueryFloatAttribute(const char* name, float* value) const;
    XMLError QueryStringAttribute(const char* name, const char** value) const;

    int QueryAttribute(const char* name, int* value) const                         { return QueryIntAttribute(name, value); }
    int QueryAttribute(const char* name, unsigned int* value) const                { return QueryUnsignedAttribute(name, value); }
    int QueryAttribute(const char* name, int64_t* value) const                     { return QueryInt64Attribute(name, value); }
    int QueryAttribute(const char* name, bool* value) const                        { return QueryBoolAttribute(name, value); }
    int QueryAttribute(const char* name, double* value) const                      { return QueryDoubleAttribute(name, value); }
    int QueryAttribute(const char* name, float* value) const                       { return QueryFloatAttribute(name, value); }

    const XMLAttribute* FirstAttribute() const  { return _rootAttribute; }
    const XMLAttribute* FindAttribute(const char* name) const;

    /** Value of the first child if it is a text, else NULL */
    const char* GetText() const;

    /** First child converted to a number or a boolean, XML_NO_TEXT_NODE or XML_CAN_NOT_CONVERT_TEXT if not possible (value is not modified) */
    XMLError QueryIntText(int* ival) const;
    XMLError QueryUnsignedText(unsigned* uval) const;
    XMLError QueryInt64Text(int64_t* ival) const;
    XMLError QueryBoolText(bool* bval) const;
    XMLError QueryDoubleText(double* dval) const;
    XMLError QueryFloatText(float* fval) const;

    int IntText(int defaultValue = 0) const                                         { int i = defaultValue; QueryIntText(&i); return i; }
    unsigned UnsignedText(unsigned defaultValue = 0) const                          { unsigned i = defaultValue; QueryUnsignedText(&i); return i; }
    int64_t Int64Text(int64_t defaultValue = 0) const                               { int64_t i = defaultValue; QueryInt64Text(&i); return i; }
    bool BoolText(bool defaultValue = false) const                                  { bool b = defaultValue; QueryBoolText(&b); return b; }
    double DoubleText(double defaultValue = 0) const                                { double d = defaultValue; QueryDoubleText(&d); return d; }
    float FloatText(float defaultValue = 0) const                                   { float f = defaultValue; QueryFloatText(&f); return f; }

    enum ElementClosingType {
        OPEN,       /* <foo> */
        CLOSED,     /* <foo/> */
        CLOSING     /* </foo> */
    };
    ElementClosingType ClosingType() const      { return _closingType; }

private:
    explicit XMLElement(XMLDocument* document) : XMLNode(document, TFSXML_ELEMENT), _rootAttribute(0), _closingType(OPEN) {}

    bool NameIs(const char* name) const;

    XMLAttribute*               _rootAttribute;
    ElementClosingType          _closingType;
};

/** -------------------------------------------------------------------------
        Document
------------------------------------------------------------------------- **/

class XMLDocument : public XMLNode
{
    friend class XMLNode;
    friend class XMLAttribute;
    friend class TfsxmlParser;

public:
    XMLDocument(bool processEntities = true, Whitespace whitespaceMode = PRESERVE_WHITESPACE);
    ~XMLDocument();

    /** Parse XML content (nBytes: size of the content, or (size_t)-1 if it is ended by NUL), return XML_SUCCESS or an error */
    XMLError Parse(const char* xml, size_t nBytes = (size_t)(-1));
    XMLError LoadFile(const char* filename);
    XMLError LoadFile(FILE* fp);
    XMLError SaveFile(const char* filename, bool compact = false);
    XMLError SaveFile(FILE* fp, bool compact = false);

    bool ProcessEntities() const                { return _processEntities; }
    Whitespace WhitespaceMode() const           { return _whitespaceMode; }
    bool HasBOM() const                         { return _writeBOM; }
    void SetBOM(bool useBOM)                    { _writeBOM = useBOM; }

    /** First element of the document */
    XMLElement* RootElement()                   { return FirstChildElement(); }
    const XMLElement* RootElement() const       { return FirstChildElement(); }

    /** Print the document (to stdout if streamer is NULL) */
    void Print(XMLPrinter* streamer = 0) const;

    void ClearError()                           { SetError(XML_SUCCESS, 0, 0); }
    bool Error() const                          { return _errorID != XML_SUCCESS; }
    XMLError ErrorID() const                    { return _errorID; }
    const char* ErrorName() const;
    static const char* ErrorIDToName(XMLError errorID);
    const char* ErrorStr() const;
    void PrintError() const;
    int ErrorLineNum() const                    { return _errorLineNum; }

    /** Delete all nodes and reset the error */
    void Clear();

private:
    XMLDocument(const XMLDocument&);
    void operator=(const XMLDocument&);

    void ParseBuffer(size_t size, bool keepOnError);
    void SetError(XMLError error, int lineNum, const char* format, ...);
    void* Alloc(size_t size) const;
    void FreeAll();
    const char* Decode(const char* start, const char* end, unsigned flags) const;
    int LineNum(const char* pos) const;

    bool                        _writeBOM;
    bool                        _processEntities;
    Whitespace                  _whitespaceMode;
    XMLError                    _errorID;
    int                         _errorLineNum;
    int                         _parseLineNum;
    std::string                 _errorStr;
    char*                       _charBuffer;
    mutable const char*         _linePos;       /* Last position with a computed line number */
    mutable int                 _lineNum;
    mutable std::vector<char*>  _blocks;        /* Memory of nodes, attributes and decoded values */
    mutable char*               _blockPos;
    mutable size_t              _blockLeft;
};

/** -------------------------------------------------------------------------
        Handles (calls on NULL nodes return NULL nodes)
------------------------------------------------------------------------- **/

class XMLHandle
{
public:
    XMLHandle(XMLNode* node) : _node(node) {}
    XMLHandle(XMLNode& node) : _node(&node) {}
    XMLHandle(const XMLHandle& ref) : _node(ref._node) {}
    XMLHandle& operator=(const XMLHandle& ref)                  { _node = ref._node; return *this; }

    XMLHandle FirstChild()                                      { return XMLHandle(_node ? _node->FirstChild() : 0); }
    XMLHandle FirstChildElement(const char* name = 0)           { return XMLHandle(_node ? _node->FirstChildElement(name) : 0); }
    XMLHandle LastChild()                                       { return XMLHandle(_node ? _node->LastChild() : 0); }
    XMLHandle LastChildElement(const char* name = 0)            { return XMLHandle(_node ? _node->LastChildElement(name) : 0); }
    XMLHandle PreviousSibling()                                 { return XMLHandle(_node ? _node->PreviousSibling() : 0); }
    XMLHandle PreviousSiblingElement(const char* name = 0)      { return XMLHandle(_node ? _node->PreviousSiblingElement(name) : 0); }
    XMLHandle NextSibling()                                     { return XMLHandle(_node ? _node->NextSibling() : 0); }
    XMLHandle NextSiblingElement(const char* name = 0)          { return XMLHandle(_node ? _node->NextSiblingElement(name) : 0); }

    XMLNode* ToNode()                                           { return _node; }
    XMLElement* ToElement()                                     { return _node ? _node->ToElement() : 0; }
    XMLText* ToText()                                           { return _node ? _node->ToText() : 0; }
    XMLUnknown* ToUnknown()                                     { return _node ? _node->ToUnknown() : 0; }
    XMLDeclaration* ToDeclaration()                             { return _node ? _node->ToDeclaration() : 0; }

private:
    XMLNode* _node;
};

class XMLConstHandle
{
public:
    XMLConstHandle(const XMLNode* node) : _node(node) {}
    XMLConstHandle(const XMLNode& node) : _node(&node) {}
    XMLConstHandle(const XMLConstHandle& ref) : _node(ref._node) {}
    XMLConstHandle& operator=(const XMLConstHandle& ref)                    { _node = ref._node; return *this; }

    const XMLConstHandle FirstChild() const                                 { return XMLConstHandle(_node ? _node->FirstChild() : 0); }
    const XMLConstHandle FirstChildElement(const char* name = 0) const      { return XMLConstHandle(_node ? _node->FirstChildElement(name) : 0); }
    const XMLConstHandle LastChild() const                                  { return XMLConstHandle(_node ? _node->LastChild() : 0); }
    const XMLConstHandle LastChildElement(const char* name = 0) const       { return XMLConstHandle(_node ? _node->LastChildElement(name) : 0); }
    const XMLConstHandle PreviousSibling() const                            { return XMLConstHandle(_node ? _node->PreviousSibling() : 0); }
    const XMLConstHandle PreviousSiblingElement(const char* name = 0) const { return XMLConstHandle(_node ? _node->PreviousSiblingElement(name) : 0); }
    const XMLConstHandle NextSibling() const                                { return XMLConstHandle(_node ? _node->NextSibling() : 0); }
    const XMLConstHandle NextSiblingElement(const char* name = 0) const     { return XMLConstHandle(_node ? _node->NextSiblingElement(name) : 0); }

    const XMLNode* ToNode() const                                           { return _node; }
    const XMLElement* ToElement() const                                     { return _node ? _node->ToElement() : 0; }
    const XMLText* ToText() const                                           { return _node ? _node->ToText() : 0; }
    const XMLUnknown* ToUnknown() const                                     { return _node ? _node->ToUnknown() : 0; }
    const XMLDeclaration* ToDeclaration() const                             { return _node ? _node->ToDeclaration() : 0; }

private:
    const XMLNode* _node;
};

/** -------------------------------------------------------------------------
        Printer (to a FILE, or to memory if file is NULL, see CStr())
------------------------------------------------------------------------- **/

class XMLPrinter : public XMLVisitor
{
public:
    XMLPrinter(FILE* file = 0, bool compact = false, int depth = 0);
    virtual ~XMLPrinter() {}

    /** Streaming API */
    void PushHeader(bool writeBOM, bool writeDeclaration);
    void OpenElement(const char* name, bool compactMode = false);
    void PushAttribute(const char* name, const char* value);
    void PushAttribute(const char* name, int value);
    void PushAttribute(const char* name, unsigned value);
    void PushAttribute(const char* name, int64_t value);
    void PushAttribute(const char* name, bool value);
    void PushAttribute(const char* name, double value);
    virtual void CloseElement(bool compactMode = false);
    void PushText(const char* text, bool cdata = false);
    void PushText(int value);
    void PushText(unsigned value);
    void PushText(int64_t value);
    void PushText(bool value);
    void PushText(float value);
    void PushText(double value);
    void PushComment(const char* comment);
    void PushDeclaration(const char* value);
    void PushUnknown(const char* value);

    virtual bool VisitEnter(const XMLDocument& doc);
    virtual bool VisitExit(const XMLDocument& /*doc*/) { return true; }
    virtual bool VisitEnter(const XMLElement& element, const XMLAttribute* attribute);
    virtual bool VisitExit(const XMLElement& element);
    virtual bool Visit(const XMLText& text);
    virtual bool Visit(const XMLComment& comment);
    virtual bool Visit(const XMLDeclaration& declaration);
    virtual bool Visit(const XMLUnknown& unknown);

    /** Printed content (if printing to memory), size includes the terminating NUL */
    const char* CStr() const                    { return &_buffer[0]; }
    int CStrSize() const                        { return (int)_buffer.size(); }
    void ClearBuffer()                          { _buffer.assign(1, '\0'); _firstElement = true; }

protected:
    virtual bool CompactMode(const XMLElement&) { return _compactMode; }
    virtual void PrintSpace(int depth);
    void Print(const char* format, ...);
    void Write(const char* data, size_t size);
    void Write(const char* data)                { Write(data, strlen(data)); }
    void Putc(char ch);
    void SealElementIfJustOpened();

    bool                        _elementJustOpened;
    std::vector<const char*>    _stack;

private:
    XMLPrinter(const XMLPrinter&);
    XMLPrinter& operator=(const XMLPrinter&);

    void PrintString(const char* p, bool restrictedEntitySet);

    bool                        _firstElement;
    FILE*                       _fp;
    int                         _depth;
    int                         _textDepth;
    bool                        _processEntities;
    bool                        _compactMode;
    std::vector<char>           _buffer;
};

/** -------------------------------------------------------------------------
        Inline functions
------------------------------------------------------------------------- **/

inline XMLElement* XMLNode::ToElement()                         { return _type == TFSXML_ELEMENT ? static_cast<XMLElement*>(this) : 0; }
inline XMLText* XMLNode::ToText()                               { return _type == TFSXML_TEXT ? static_cast<XMLText*>(this) : 0; }
inline XMLComment* XMLNode::ToComment()                         { return _type == TFSXML_COMMENT ? static_cast<XMLComment*>(this) : 0; }
inline XMLDocument* XMLNode::ToDocument()                       { return _type == TFSXML_DOCUMENT ? static_cast<XMLDocument*>(this) : 0; }
inline XMLDeclaration* XMLNode::ToDeclaration()                 { return _type == TFSXML_DECLARATION ? static_cast<XMLDeclaration*>(this) : 0; }
inline XMLUnknown* XMLNode::ToUnknown()                         { return _type == TFSXML_UNKNOWN ? static_cast<XMLUnknown*>(this) : 0; }
inline const XMLElement* XMLNode::ToElement() const             { return _type == TFSXML_ELEMENT ? static_cast<const XMLElement*>(this) : 0; }
inline const XMLText* XMLNode::ToText() const                   { return _type == TFSXML_TEXT ? static_cast<const XMLText*>(this) : 0; }
inline const XMLComment* XMLNode::ToComment() const             { return _type == TFSXML_COMMENT ? static_cast<const XMLComment*>(this) : 0; }
inline const XMLDocument* XMLNode::ToDocument() const           { return _type == TFSXML_DOCUMENT ? static_cast<const XMLDocument*>(this) : 0; }
inline const XMLDeclaration* XMLNode::ToDeclaration() const     { return _type == TFSXML_DECLARATION ? static_cast<const XMLDeclaration*>(this) : 0; }
inline const XMLUnknown* XMLNode::ToUnknown() const             { return _type == TFSXML_UNKNOWN ? static_cast<const XMLUnknown*>(this) : 0; }

} /* namespace */

#endif
