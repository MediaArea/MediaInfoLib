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
 * Tiny Fast Streamable XML parser
 *
 * No allocation, no copy: returned strings point to the buffer provided by
 * the caller. Encoded content (entities, CDATA) is decoded only on request.
 *
 * Streaming: when a function returns 1 (need of more data), priv->buf and
 * priv->len are the part of the buffer not yet consumed by the parser. The
 * caller provides a new buffer starting with this part followed by more data,
 * sets priv->buf and priv->len to the new buffer and its length (priv->flags
 * must not be modified) and calls the same function again. Strings returned
 * before point to the previous buffer.
 */

#ifndef TFSXML_H
#define TFSXML_H

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/** -------------------------------------------------------------------------
        Splitting of the XML content in blocks
------------------------------------------------------------------------- **/

/** Base structure for the parser
 *
 * @param buf  pointer to the buffer which has the data
 * @param len  length of the buffer
 * @param flags  private data, can change between versions of the library
 *
 * @note after init, priv should not be directly used (data may be something else than a buf/len pair), except for providing more data (see above)
 */
typedef struct tfsxml_string {
    const char* buf;
    unsigned    len;
    unsigned    flags;
} tfsxml_string;

/** Initialize the parser
 *
 * @param priv      pointer to a tfsxml_string dedicated instance, private use by the parser
 * @param buf       pointer to start of the buffer
 * @param len       length of the buffer
 * @param version   API version supported by the client
 *
 * @return  0 if the content may be XML (UTF-8 BOM and whitespaces are skipped, next byte is "<")
 *          -1 if the content is not XML or the version is not supported
 *          (or if the buffer has only a BOM and whitespaces, more data may be needed)
 *
 * @note after init, priv should not be directly used (data may be something else than a buf/len pair)
 */
int tfsxml_init(tfsxml_string* priv, const void* buf, unsigned len, unsigned version);

/** Get next element or other content except an element value
 *
 * @param priv  pointer to a tfsxml_string dedicated instance, private use by the parser
 * @param n  pointer to tfsxml_string instance receiving the element name or other content
 *
 * @return  0 if an element is available at the current level
 *          -1 if no element is available at the current level
 *          1 if need of more data
 *
 * @note n receives the element name, or for other content the content between "<" and ">":
 *       "?target data?" for processing instructions,
 *       "!-- text --" for comments,
 *       "!DOCTYPE ..." for other declarations.
 *       Text and CDATA sections are skipped (see tfsxml_text for getting them).
 *       Content of the previous element is skipped if it was not entered.
 * @note when -1 is returned at the end of the current element, n receives the name in the end tag
 */
int tfsxml_next(tfsxml_string* priv, tfsxml_string* n);

/** Get the text before the next element or other content (mixed content)
 *
 * @param priv  pointer to a tfsxml_string dedicated instance, private use by the parser
 * @param v  pointer to tfsxml_string instance receiving the text
 *
 * @return  0 if text is available (may contain only whitespaces)
 *          -1 if no text is available (next is an element, other content or the end of the current element)
 *          1 if need of more data (also at the end of the buffer if there is text without markup after it)
 *
 * @note CDATA sections are part of the text, tfsxml_decode removes their markup
 * @note same position as tfsxml_next: content of the previous element is skipped if it was not entered,
 *       call it before tfsxml_next for getting the text before each element and before the end of the current element
 */
int tfsxml_text(tfsxml_string* priv, tfsxml_string* v);

/** Get next attribute of an element
 *
 * @param priv  pointer to a tfsxml_string dedicated instance, private use by the parser
 * @param n  pointer to tfsxml_string instance receiving the attribute name
 * @param v  pointer to tfsxml_string instance receiving the attribute value
 *
 * @return  0 if an attribute is available for the current element
 *          -1 if no more attribute is available for the current element
 *          1 if need of more data
 *
 * @note available only just after tfsxml_next returned an element
 */
int tfsxml_attr(tfsxml_string* priv, tfsxml_string* n, tfsxml_string* v);

/** Get next value of an element
 *
 * @param priv  pointer to a tfsxml_string dedicated instance, private use by the parser
 * @param v  pointer to tfsxml_string instance receiving the value
 *
 * @return  0 if a value is available for the current element
 *          -1 if no value is available for the current element
 *          1 if need of more data
 *
 * @note the value is the content until the first sub-element, comment or processing instruction
 *       (CDATA sections are included), the rest of the element content is skipped
 */
int tfsxml_value(tfsxml_string* priv, tfsxml_string* v);

/** Enter in the element in order to get sub-elements
 *
 * @param priv  pointer to a tfsxml_string dedicated instance, private use by the parser
 *
 * @return  0 if it is possible to enter in an element
 *          -1 if it is not possible to enter in an element
 *          1 if need of more data
 */
int tfsxml_enter(tfsxml_string* priv);

/** Leave the current parsed element (going to upper level)
 *
 * @param priv  pointer to a tfsxml_string dedicated instance, private use by the parser
 *
 * @return  0 if it is possible to leave an element
 *          -1 if it is not possible to leave an element
 *          1 if need of more data
 *
 * @note the current element is the one entered with tfsxml_enter, the rest of its content is skipped
 */
int tfsxml_leave(tfsxml_string* priv);

/** -------------------------------------------------------------------------
        Handling the encoded XML block
------------------------------------------------------------------------- **/

/** Convert encoded XML block (attribute or value) to real content (encoded in UTF-8)
 *
 * @param s         opaque data transmitted to func
 * @param v         XML content to decode
 * @param func      append function that will receive blocks of decoded content
 *
 * @param func_s    string to append to
 * @param func_buf  decoded content
 * @param func_len  length of the decoded content
 *
 * @note predefined entities, character references and CDATA sections are decoded,
 *       other content (including invalid references) is kept as is
 * @note see tfsxml_decode_string C++ function for an example of usage
 */
void tfsxml_decode(void* s, const tfsxml_string* v, void (*func)(void* func_s, const char* func_buf, unsigned func_len));

/** -------------------------------------------------------------------------
        Helper functions related to tfsxml_string
------------------------------------------------------------------------- **/

/** Compare two strings
 *
 * @param a  string to compare
 * @param b  string to compare
 *
 * @note similar to C strcmp function
 */
int tfsxml_strcmp_charp(tfsxml_string a, const char* b);

/** Compare two strings
 *
 * @param a  string to compare
 * @param b  string to compare
 * @param n  maximum number of characters to compare
 *
 * @note similar to C strncmp function
 */
int tfsxml_strncmp_charp(tfsxml_string a, const char* b, unsigned n);

/** Locate substring
 *
 * @param a  string to be scanned
 * @param b  substring to match
 *
 * @return  a from the first occurrence of b, or a string with NULL buf and 0 len if b is not found
 *
 * @note similar to C strstr function
 */
tfsxml_string tfsxml_strstr_charp(tfsxml_string a, const char* b);

/** -------------------------------------------------------------------------
        Helper functions related to namespaces in names
------------------------------------------------------------------------- **/

/** Local name of an element or attribute name (name without namespace prefix)
 *
 * @param a  name (e.g. "dc:title")
 *
 * @return  part of a after the last ":" (e.g. "title"), or a if there is no ":"
 */
tfsxml_string tfsxml_local_name(tfsxml_string a);

/** Namespace prefix of an element or attribute name
 *
 * @param a  name (e.g. "dc:title")
 *
 * @return  part of a before the last ":" (e.g. "dc", empty for ":title"),
 *          or a string with NULL buf and 0 len if there is no ":"
 */
tfsxml_string tfsxml_prefix(tfsxml_string a);

/** Compare the local name of a name with a string
 *
 * @param a  name (e.g. "dc:title" or "title")
 * @param b  local name (e.g. "title")
 *
 * @note similar to C strcmp function, with the local name of a (see tfsxml_local_name)
 */
int tfsxml_strcmp_local_charp(tfsxml_string a, const char* b);

/** Compare the namespace prefix of a name with a string
 *
 * @param a  name (e.g. "dc:title")
 * @param b  namespace prefix without ":" (e.g. "dc"), "" for a name without prefix
 *
 * @note similar to C strcmp function, with the prefix of a (see tfsxml_prefix)
 * @note the prefix is compared, not the namespace URI it is bound to (namespaces are not processed)
 */
int tfsxml_strcmp_prefix_charp(tfsxml_string a, const char* b);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#ifdef __cplusplus
#include <string>

inline void tfsxml_decode_string(void* d, const char* buf, unsigned len) { ((std::string*)d)->append(buf, len); }

/** Convert encoded XML block (attribute or value) to real content (encoded in UTF-8)
 *
 * @param s  string which will be appended with the decoded content
 * @param b  XML content to decode
 */
inline void tfsxml_decode(std::string& s, const tfsxml_string& b) { tfsxml_decode(&s, &b, tfsxml_decode_string); }

/** Convert encoded XML block (attribute or value) to real content (encoded in UTF-8)
 *
 * @param b  XML content to decode
 * @return  decoded content
 */
inline std::string tfsxml_decode(const tfsxml_string& b) { std::string s; tfsxml_decode(&s, &b, tfsxml_decode_string); return s; }

#endif /* __cplusplus */

#endif
