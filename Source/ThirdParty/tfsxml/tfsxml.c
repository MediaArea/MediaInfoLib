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

#include "tfsxml.h"
#ifndef TFSXML_NO_LIBC
    #include <string.h>
#endif
#ifndef NULL
    #ifdef __cplusplus
        #define NULL 0
    #else
        #define NULL ((void *)0)
    #endif
#endif

/*
 * priv flags:
 * bit 0: inside an element header (attributes not all parsed)
 * bit 1: the last element has a content which is not yet entered nor skipped
 * bits 2+: count of levels of a skip of content to resume when more data is available
 */
#define TFSXML_HEADER 1u
#define TFSXML_OPEN   2u
#define TFSXML_LEVEL  2

/*
 * attribute / value flags:
 * bit 0: must be decoded
 */
#define TFSXML_DECODE 1u

#define tfsxml_is_space(c) ((c) == ' ' || (c) == '\n' || (c) == '\t' || (c) == '\r')

/* Characters ending names are all lower than "?", most of name characters are not */
#define tfsxml_is_name_end(c) ((unsigned char)(c) < '?' && (tfsxml_is_space(c) || (c) == '/' || (c) == '>'))
#define tfsxml_is_attr_name_end(c) ((unsigned char)(c) < '?' && (tfsxml_is_space(c) || (c) == '/' || (c) == '>' || (c) == '='))

/** -------------------------------------------------------------------------
        Scan helpers
------------------------------------------------------------------------- **/

/* First c in [p, end), NULL if not found
 * Searched characters are often near, the first bytes are tested before calling memchr() (faster for long content only) */
#ifndef TFSXML_NEAR
    #define TFSXML_NEAR 16
#endif
static const char* tfsxml_find(const char* p, const char* end, char c) {
#ifndef TFSXML_NO_LIBC
    const char* near_end = end - p > TFSXML_NEAR ? p + TFSXML_NEAR : end;
    for (; p < near_end; p++) {
        if (*p == c) {
            return p;
        }
    }
    return p < end ? (const char*)memchr(p, c, (size_t)(end - p)) : NULL;
#else
    for (; p < end; p++) {
        if (*p == c) {
            return p;
        }
    }
    return NULL;
#endif
}

/* First ">" preceded by count c ("]]>", "-->" or "?>") in [p, end), NULL if not found */
static const char* tfsxml_find_end(const char* p, const char* end, char c, int count) {
    const char* start = p;
    for (;;) {
        const char* q = tfsxml_find(p, end, '>');
        if (!q || (q - start >= count && q[-1] == c && (count == 1 || q[-2] == c))) {
            return q;
        }
        p = q + 1;
    }
}

/* End of an element name */
static const char* tfsxml_find_name_end(const char* p, const char* end) {
    while (p < end && !tfsxml_is_name_end(*p)) {
        p++;
    }
    return p;
}

/* Type of markup starting with "<" */
enum tfsxml_markup {
    tfsxml_incomplete,  /* not enough data for knowing the type */
    tfsxml_end_tag,     /* </ */
    tfsxml_pi,          /* <? */
    tfsxml_cdata,       /* <![CDATA[ */
    tfsxml_comment,     /* <!-- */
    tfsxml_declaration, /* other <! */
    tfsxml_element      /* other < */
};

static int tfsxml_markup_type(const char* p, const char* end) {
    static const char cdata[] = "<![CDATA[";
    int i;
    if (end - p < 2) {
        return tfsxml_incomplete;
    }
    switch (p[1]) {
        case '/': return tfsxml_end_tag;
        case '?': return tfsxml_pi;
        case '!': break;
        default: return tfsxml_element;
    }
    if (end - p < 3) {
        return tfsxml_incomplete;
    }
    if (p[2] == '-') {
        if (end - p < 4) {
            return tfsxml_incomplete;
        }
        return p[3] == '-' ? tfsxml_comment : tfsxml_declaration;
    }
    if (p[2] == '[') {
        for (i = 3; i < 9; i++) {
            if (end - p == i) {
                return tfsxml_incomplete;
            }
            if (p[i] != cdata[i]) {
                return tfsxml_declaration;
            }
        }
        return tfsxml_cdata;
    }
    return tfsxml_declaration;
}

/* End of markup which is not an element (after ">"), NULL if not found */
static const char* tfsxml_markup_end(const char* p, const char* end, int type) {
    switch (type) {
        case tfsxml_cdata:   p = tfsxml_find_end(p + 9, end, ']', 2); break;
        case tfsxml_comment: p = tfsxml_find_end(p + 4, end, '-', 2); break;
        case tfsxml_pi:      p = tfsxml_find_end(p + 2, end, '?', 1); break;
        default:             p = tfsxml_find(p + 2, end, '>');
    }
    return p ? p + 1 : NULL;
}

/** -------------------------------------------------------------------------
        Parser helpers
------------------------------------------------------------------------- **/

static void tfsxml_move(tfsxml_string* priv, const char* p) {
    priv->len -= (unsigned)(p - priv->buf);
    priv->buf = p;
}

static void tfsxml_set_level(tfsxml_string* priv, unsigned level) {
    priv->flags = (priv->flags & (TFSXML_HEADER | TFSXML_OPEN)) | (level << TFSXML_LEVEL);
}

/* Next attribute in an element header
 * TFSXML_OPEN in flags is unset when "/" is found, value flags are set only if decode is not 0
 * return 0 if an attribute is found, -1 at the end of the header, 1 if more data is needed (pp and flags not modified) */
static int tfsxml_header_next(const char** pp, const char* end, unsigned* flags, tfsxml_string* n, tfsxml_string* v, int decode) {
    const char* p = *pp;
    unsigned f = *flags;
    for (; p < end; p++) {
        const char c = *p;
        if (c == '>') {
            *pp = p + 1;
            *flags = f;
            return -1;
        }
        if (c == '/') {
            f &= ~TFSXML_OPEN;
            continue;
        }
        if (tfsxml_is_space(c) || !(f & TFSXML_OPEN)) {
            continue; /* Whitespace, or junk after "/" */
        }

        /* Name */
        n->buf = p;
        while (p < end && !tfsxml_is_attr_name_end(*p)) {
            p++;
        }
        n->len = (unsigned)(p - n->buf);
        n->flags = 0;
        while (p < end && tfsxml_is_space(*p)) {
            p++;
        }
        if (p == end) {
            return 1;
        }
        v->flags = 0;
        if (*p != '=') {
            /* No value */
            v->buf = p;
            v->len = 0;
            *pp = p;
            *flags = f;
            return 0;
        }

        /* Value */
        do {
            p++;
        } while (p < end && tfsxml_is_space(*p));
        if (p == end) {
            return 1;
        }
        if (*p == '"' || *p == '\'') {
            const char* q = tfsxml_find(p + 1, end, *p);
            if (!q) {
                return 1;
            }
            v->buf = p + 1;
            v->len = (unsigned)(q - v->buf);
            p = q + 1;
        }
        else {
            /* Not quoted */
            v->buf = p;
            while (p < end && !tfsxml_is_name_end(*p)) {
                p++;
            }
            if (p == end) {
                return 1;
            }
            v->len = (unsigned)(p - v->buf);
        }
        if (decode && tfsxml_find(v->buf, v->buf + v->len, '&')) {
            v->flags = TFSXML_DECODE;
        }
        *pp = p;
        *flags = f;
        return 0;
    }
    return 1;
}

/* Skip the remaining attributes of the current element header */
static int tfsxml_leave_header(tfsxml_string* priv) {
    const char* p = priv->buf;
    unsigned flags = priv->flags;
    tfsxml_string n, v;
    int result;
    do {
        result = tfsxml_header_next(&p, priv->buf + priv->len, &flags, &n, &v, 0);
    } while (!result);
    tfsxml_move(priv, p);
    priv->flags = flags;
    if (result > 0) {
        return 1;
    }
    priv->flags &= ~TFSXML_HEADER;
    return 0;
}

/* Skip content until the end of level count of elements
 * return 0 when done, 1 if more data is needed (count of levels is kept for resuming) */
static int tfsxml_skip(tfsxml_string* priv, unsigned level) {
    const char* p = priv->buf;
    const char* end = p + priv->len;
    for (;;) {
        const char* q = tfsxml_find(p, end, '<');
        int type;
        if (!q) {
            p = end;
            break;
        }
        p = q;
        type = tfsxml_markup_type(p, end);
        if (type == tfsxml_element) {
            tfsxml_string n, v;
            unsigned flags = TFSXML_OPEN;
            int result;
            q = tfsxml_find_name_end(p + 1, end);
            do {
                result = tfsxml_header_next(&q, end, &flags, &n, &v, 0);
            } while (!result);
            if (result > 0) {
                break;
            }
            if (flags & TFSXML_OPEN) {
                level++;
            }
            p = q;
            continue;
        }
        if (type == tfsxml_incomplete) {
            break;
        }
        q = tfsxml_markup_end(p, end, type);
        if (!q) {
            break;
        }
        p = q;
        if (type == tfsxml_end_tag && !--level) {
            tfsxml_move(priv, p);
            tfsxml_set_level(priv, 0);
            return 0;
        }
    }
    tfsxml_move(priv, p);
    tfsxml_set_level(priv, level);
    return 1;
}

/** -------------------------------------------------------------------------
        Splitting of the XML content in blocks
------------------------------------------------------------------------- **/

int tfsxml_init(tfsxml_string* priv, const void* buf, unsigned len, unsigned version) {
    const char* buf_8 = (const char*)buf;

    if (version != 0 || !len) {
        return -1;
    }

    /* BOM detection */
    if (len >= 3
        && (unsigned char)buf_8[0] == 0xEF
        && (unsigned char)buf_8[1] == 0xBB
        && (unsigned char)buf_8[2] == 0xBF) {
        buf_8 += 3;
        len -= 3;
    }

    /* Skip leading whitespaces */
    while (len && tfsxml_is_space(*buf_8)) {
        buf_8++;
        len--;
    }

    /* Start detection */
    if (!len || *buf_8 != '<') {
        return -1;
    }

    /* Init */
    priv->buf = buf_8;
    priv->len = len;
    priv->flags = 0;

    return 0;
}

/* Skip what remains of the previous element (header, content not entered), return 1 if more data is needed */
static int tfsxml_skip_previous(tfsxml_string* priv) {
    unsigned level = priv->flags >> TFSXML_LEVEL;

    if (level) {
        /* Resuming the skip of the previous element content */
        return tfsxml_skip(priv, level);
    }

    /* Exiting previous element header if needed */
    if ((priv->flags & TFSXML_HEADER) && tfsxml_leave_header(priv)) {
        return 1;
    }

    /* Skipping previous element content if needed */
    if (priv->flags & TFSXML_OPEN) {
        priv->flags &= ~TFSXML_OPEN;
        return tfsxml_skip(priv, 1);
    }

    return 0;
}

int tfsxml_next(tfsxml_string* priv, tfsxml_string* n) {
    const char* p;
    const char* end;

    if (tfsxml_skip_previous(priv)) {
        return 1;
    }

    p = priv->buf;
    end = p + priv->len;
    for (;;) {
        const char* q = tfsxml_find(p, end, '<');
        int type;
        if (!q) {
            p = end;
            break;
        }
        p = q;
        type = tfsxml_markup_type(p, end);
        if (type == tfsxml_element) {
            q = tfsxml_find_name_end(p + 1, end);
            if (q == end) {
                break;
            }
            n->buf = p + 1;
            n->len = (unsigned)(q - n->buf);
            n->flags = 0;
            tfsxml_move(priv, q);
            priv->flags |= TFSXML_HEADER | TFSXML_OPEN;
            return 0;
        }
        if (type == tfsxml_incomplete) {
            break;
        }
        q = tfsxml_markup_end(p, end, type);
        if (!q) {
            break;
        }
        switch (type) {
            case tfsxml_cdata:
                p = q; /* Ignored */
                continue;
            case tfsxml_end_tag:
                n->buf = p + 2;
                n->len = (unsigned)(tfsxml_find_name_end(n->buf, q - 1) - n->buf);
                n->flags = 0;
                tfsxml_move(priv, q);
                return -1;
            default:
                n->buf = p + 1;
                n->len = (unsigned)(q - 1 - n->buf);
                n->flags = 0;
                tfsxml_move(priv, q);
                return 0;
        }
    }
    tfsxml_move(priv, p);
    return 1;
}

int tfsxml_text(tfsxml_string* priv, tfsxml_string* v) {
    const char* p;
    const char* end;
    unsigned flags = 0;

    if (tfsxml_skip_previous(priv)) {
        return 1;
    }

    /* Text until next markup, CDATA sections included */
    p = priv->buf;
    end = p + priv->len;
    for (;;) {
        const char* q = tfsxml_find(p, end, '<');
        int type;
        if (!q) {
            return 1; /* Text may continue after the end of the buffer */
        }
        if (!flags && tfsxml_find(p, q, '&')) {
            flags = TFSXML_DECODE;
        }
        p = q;
        type = tfsxml_markup_type(p, end);
        if (type == tfsxml_incomplete) {
            return 1;
        }
        if (type != tfsxml_cdata) {
            break;
        }
        q = tfsxml_markup_end(p, end, type);
        if (!q) {
            return 1;
        }
        flags = TFSXML_DECODE;
        p = q;
    }
    if (p == priv->buf) {
        return -1;
    }
    v->buf = priv->buf;
    v->len = (unsigned)(p - priv->buf);
    v->flags = flags;
    tfsxml_move(priv, p);
    return 0;
}

int tfsxml_attr(tfsxml_string* priv, tfsxml_string* n, tfsxml_string* v) {
    const char* p = priv->buf;
    unsigned flags = priv->flags;
    int result;

    if (!(flags & TFSXML_HEADER)) {
        return -1;
    }

    v->flags = 0;
    result = tfsxml_header_next(&p, p + priv->len, &flags, n, v, 1);
    if (result <= 0) {
        tfsxml_move(priv, p);
        priv->flags = result ? (flags & ~TFSXML_HEADER) : flags;
    }
    return result;
}

int tfsxml_value(tfsxml_string* priv, tfsxml_string* v) {
    tfsxml_string priv_bak;
    const char* p;
    const char* end;
    unsigned flags = 0;

    /* Exiting previous element header if needed */
    if ((priv->flags & TFSXML_HEADER) && tfsxml_leave_header(priv)) {
        return 1;
    }

    /* Previous element must not be finished */
    if (!(priv->flags & TFSXML_OPEN)) {
        return -1;
    }

    p = priv->buf;
    end = p + priv->len;
    for (;;) {
        const char* q = tfsxml_find(p, end, '<');
        int type;
        if (!q) {
            return 1;
        }
        if (!flags && tfsxml_find(p, q, '&')) {
            flags = TFSXML_DECODE;
        }
        p = q;
        type = tfsxml_markup_type(p, end);
        if (type == tfsxml_incomplete) {
            return 1;
        }
        if (type != tfsxml_cdata) {
            break;
        }
        q = tfsxml_markup_end(p, end, type);
        if (!q) {
            return 1;
        }
        flags = TFSXML_DECODE;
        p = q;
    }

    /* Skipping the rest of the element content */
    priv_bak = *priv;
    tfsxml_move(priv, p);
    priv->flags &= ~TFSXML_OPEN;
    if (tfsxml_skip(priv, 1)) {
        *priv = priv_bak;
        return 1;
    }
    v->buf = priv_bak.buf;
    v->len = (unsigned)(p - priv_bak.buf);
    v->flags = flags;
    return 0;
}

int tfsxml_enter(tfsxml_string* priv) {
    /* Exiting previous element header if needed */
    if ((priv->flags & TFSXML_HEADER) && tfsxml_leave_header(priv)) {
        return 1;
    }

    /* Previous element must not be finished */
    if (!(priv->flags & TFSXML_OPEN)) {
        return -1;
    }

    priv->flags &= ~TFSXML_OPEN;
    return 0;
}

int tfsxml_leave(tfsxml_string* priv) {
    unsigned level = priv->flags >> TFSXML_LEVEL;

    if (!level) {
        /* Exiting previous element header if needed */
        if ((priv->flags & TFSXML_HEADER) && tfsxml_leave_header(priv)) {
            return 1;
        }

        /* Current element and previous element content if needed */
        level = 1;
        if (priv->flags & TFSXML_OPEN) {
            priv->flags &= ~TFSXML_OPEN;
            level++;
        }
    }

    return tfsxml_skip(priv, level);
}

/** -------------------------------------------------------------------------
        Handling the encoded XML block
------------------------------------------------------------------------- **/

/* Decode an entity or character reference ("&" and ";" excluded), return the count of bytes or 0 if invalid */
static unsigned tfsxml_decode_reference(const char* p, const char* end, char* utf8) {
    static const char* const names[5] = { "amp", "apos", "gt", "lt", "quot" };
    static const char chars[5] = { '&', '\'', '>', '<', '"' };
    unsigned long value = 0;
    int i;

    if (p == end) {
        return 0;
    }

    if (*p != '#') {
        /* Predefined entities */
        for (i = 0; i < 5; i++) {
            const char* q = p;
            const char* name = names[i];
            while (q < end && *name && *q == *name) {
                q++;
                name++;
            }
            if (q == end && !*name) {
                utf8[0] = chars[i];
                return 1;
            }
        }
        return 0;
    }

    /* Character reference */
    p++;
    if (p < end && (*p == 'x' || *p == 'X')) {
        p++;
        if (p == end) {
            return 0;
        }
        for (; p < end; p++) {
            const char c = *p;
            value <<= 4;
            if (c >= '0' && c <= '9')
                value |= (unsigned long)(c - '0');
            else if (c >= 'A' && c <= 'F')
                value |= (unsigned long)(c - ('A' - 10));
            else if (c >= 'a' && c <= 'f')
                value |= (unsigned long)(c - ('a' - 10));
            else
                return 0;
            if (value > 0x10FFFF) {
                return 0;
            }
        }
    }
    else {
        if (p == end) {
            return 0;
        }
        for (; p < end; p++) {
            const char c = *p;
            if (c < '0' || c > '9') {
                return 0;
            }
            value = value * 10 + (unsigned long)(c - '0');
            if (value > 0x10FFFF) {
                return 0;
            }
        }
    }
    if (!value || (value >= 0xD800 && value <= 0xDFFF)) {
        return 0; /* Not a valid character */
    }

    /* UTF-8 */
    if (value < 0x80) {
        utf8[0] = (char)value;
        return 1;
    }
    if (value < 0x800) {
        utf8[0] = (char)(0xC0 | (value >> 6));
        utf8[1] = (char)(0x80 | (value & 0x3F));
        return 2;
    }
    if (value < 0x10000) {
        utf8[0] = (char)(0xE0 | (value >> 12));
        utf8[1] = (char)(0x80 | ((value >> 6) & 0x3F));
        utf8[2] = (char)(0x80 | (value & 0x3F));
        return 3;
    }
    utf8[0] = (char)(0xF0 | (value >> 18));
    utf8[1] = (char)(0x80 | ((value >> 12) & 0x3F));
    utf8[2] = (char)(0x80 | ((value >> 6) & 0x3F));
    utf8[3] = (char)(0x80 | (value & 0x3F));
    return 4;
}

void tfsxml_decode(void* s, const tfsxml_string* v, void (*func)(void*, const char*, unsigned)) {
    const char* p = v->buf;
    const char* end;
    const char* begin;

    if (!(v->flags & TFSXML_DECODE)) {
        func(s, v->buf, v->len);
        return;
    }

    end = p + v->len;
    begin = p;
    while (p < end) {
        const char* q;
        if (*p == '&') {
            char utf8[4];
            unsigned len;
            q = p + 1;
            while (q < end && *q != ';' && *q != '&' && *q != '<') {
                q++;
            }
            if (q < end && *q == ';' && (len = tfsxml_decode_reference(p + 1, q, utf8))) {
                if (p > begin) {
                    func(s, begin, (unsigned)(p - begin));
                }
                func(s, utf8, len);
                p = q + 1;
                begin = p;
                continue;
            }
        }
        else if (*p == '<' && tfsxml_markup_type(p, end) == tfsxml_cdata && (q = tfsxml_markup_end(p, end, tfsxml_cdata))) {
            if (p > begin) {
                func(s, begin, (unsigned)(p - begin));
            }
            if (q - 3 > p + 9) {
                func(s, p + 9, (unsigned)(q - 3 - (p + 9)));
            }
            p = q;
            begin = p;
            continue;
        }
        p++;
    }
    func(s, begin, (unsigned)(end - begin));
}

/** -------------------------------------------------------------------------
        Helper functions related to tfsxml_string
------------------------------------------------------------------------- **/

int tfsxml_strcmp_charp(tfsxml_string a, const char* b) {
    const unsigned char* p = (const unsigned char*)a.buf;
    const unsigned char* q = (const unsigned char*)b;
    unsigned len = a.len;

    for (;; p++, q++, len--) {
        if (!len) {
            return *q ? -1 : 0; /* a is shorter than b or equal */
        }
        if (!*q) {
            return 1; /* a is longer than b */
        }
        if (*p != *q) {
            return (int)*p - (int)*q;
        }
    }
}

int tfsxml_strncmp_charp(tfsxml_string a, const char* b, unsigned n) {
    const unsigned char* p = (const unsigned char*)a.buf;
    const unsigned char* q = (const unsigned char*)b;
    unsigned len = a.len;

    for (; n; p++, q++, len--, n--) {
        if (!len) {
            return *q ? -1 : 0; /* a is shorter than b or equal */
        }
        if (!*q) {
            return 1; /* a is longer than b */
        }
        if (*p != *q) {
            return (int)*p - (int)*q;
        }
    }
    return 0;
}

tfsxml_string tfsxml_strstr_charp(tfsxml_string a, const char* b) {
    unsigned b_len = 0;
    while (b[b_len]) {
        b_len++;
    }
    if (!b_len) {
        return a;
    }

    /* Iterate string to be scanned */
    for (; a.len >= b_len; a.buf++, a.len--) {
        unsigned i = 0;
        while (i < b_len && a.buf[i] == b[i]) {
            i++;
        }
        if (i == b_len) {
            return a;
        }
    }
    a.buf = NULL;
    a.len = 0;
    return a;
}

/** -------------------------------------------------------------------------
        Helper functions related to namespaces in names
------------------------------------------------------------------------- **/

/* Length of the part before the last ":" + 1, 0 if there is no ":" */
static unsigned tfsxml_prefix_end(tfsxml_string a) {
    unsigned i = a.len;
    while (i && a.buf[i - 1] != ':') {
        i--;
    }
    return i;
}

tfsxml_string tfsxml_local_name(tfsxml_string a) {
    unsigned i = tfsxml_prefix_end(a);
    if (i) {
        a.buf += i;
        a.len -= i;
    }
    return a;
}

tfsxml_string tfsxml_prefix(tfsxml_string a) {
    unsigned i = tfsxml_prefix_end(a);
    if (!i) {
        a.buf = NULL;
        a.len = 0;
        return a;
    }
    a.len = i - 1;
    return a;
}

int tfsxml_strcmp_local_charp(tfsxml_string a, const char* b) {
    return tfsxml_strcmp_charp(tfsxml_local_name(a), b);
}

int tfsxml_strcmp_prefix_charp(tfsxml_string a, const char* b) {
    return tfsxml_strcmp_charp(tfsxml_prefix(a), b);
}
