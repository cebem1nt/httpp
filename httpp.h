#ifndef _HTTPP_HEADER
#define _HTTPP_HEADER

/*
 * Tiny header only http parser library for http 1/1 version 
 * Pipelined requests are not supported because nobody realy cares about them 
 * (https://en.wikipedia.org/wiki/HTTP_pipelining#Implementation_status)
 * 
 * TODO Chunked transfer is not really supported
 * TODO Folded headers aren't handled. 
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>  /* strncasecmp (-std=c11) */

#define HTTPP_DEFAULT_HEADERS_ARR_CAP 20

#define HTTPP_MAX_METHOD_LENGTH (10 + 1)
#define HTTPP_VERSION_BUFSIZE (10 + 1)
#define HTTPP_SUPPORTED_VERSION "HTTP/1.1"

#define _HTTP_DELIMITER "\r\n"
#define _HTTP_DELIMITER_SIZE 2
#define _HTTP_MAX_STATUS_CODE_SIZE 3

#define HTTPP_METHOD_GET       0
#define HTTPP_METHOD_HEAD      1
#define HTTPP_METHOD_POST      2
#define HTTPP_METHOD_PUT       3
#define HTTPP_METHOD_DELETE    4
#define HTTPP_METHOD_CONNECT   5
#define HTTPP_METHOD_OPTIONS   6
#define HTTPP_METHOD_TRACE     7
#define HTTPP_METHOD_PATCH     8
#define HTTPP_METHOD_UNKNOWN  -1

#define httpp_string_to_method(s) ( strcmp((s), "GET")     == 0 ? HTTPP_METHOD_GET    : \
                                    strcmp((s), "HEAD")    == 0 ? HTTPP_METHOD_HEAD   : \
                                    strcmp((s), "POST")    == 0 ? HTTPP_METHOD_POST   : \
                                    strcmp((s), "PUT")     == 0 ? HTTPP_METHOD_PUT    : \
                                    strcmp((s), "DELETE")  == 0 ? HTTPP_METHOD_DELETE : \
                                    strcmp((s), "CONNECT") == 0 ? HTTPP_METHOD_CONNECT: \
                                    strcmp((s), "OPTIONS") == 0 ? HTTPP_METHOD_OPTIONS: \
                                    strcmp((s), "TRACE")   == 0 ? HTTPP_METHOD_TRACE  : \
                                    strcmp((s), "PATCH")   == 0 ? HTTPP_METHOD_PATCH  : \
                                                                  HTTPP_METHOD_UNKNOWN) \

typedef struct {
    char* ptr;
    size_t length;
} httpp_span_t;

typedef struct {
    httpp_span_t name;
    httpp_span_t value;
} httpp_header_t;

typedef struct {
    httpp_header_t* arr;
    size_t capacity;
    size_t length;
} httpp_headers_arr_t;

typedef struct {
    httpp_headers_arr_t headers;
    httpp_span_t body;
    httpp_span_t route;
    int method;
} httpp_req_t;

typedef struct {
    httpp_headers_arr_t headers;
    httpp_span_t body;
    int code;
} httpp_res_t;

const char* httpp_method_to_string(int method);
const char* httpp_status_to_string(int status_code);
char* httpp_span_to_str(httpp_span_t* span);

int httpp_parse_request(char* buf, size_t n, httpp_req_t* dest);
httpp_header_t* httpp_parse_header(httpp_headers_arr_t* dest, char* line, size_t content_len);
httpp_header_t* httpp_headers_arr_append(httpp_headers_arr_t* hs, httpp_header_t header);
httpp_header_t* httpp_headers_arr_find(httpp_headers_arr_t* hs, char* name);
httpp_header_t* httpp_res_add_header(httpp_res_t* res, char* name, char* value);

void httpp_res_set_body(httpp_res_t* res, char* body_ptr, size_t body_len);
void httpp_res_free_added(httpp_res_t* res);
char* httpp_res_to_raw(httpp_res_t* res);

#define httpp_find_header(req_or_res, name) \
    (httpp_headers_arr_find(&(req_or_res).headers, name))

static inline void httpp_init_span(httpp_span_t* span) 
{
    span->ptr = NULL;
    span->length = 0;
}

static inline void httpp_init_req(httpp_req_t* dest, httpp_header_t* headers_arr, size_t headers_capacity)
{
    httpp_init_span(&dest->route);
    httpp_init_span(&dest->body);

    dest->headers.arr = headers_arr;
    dest->headers.capacity = headers_capacity;
    dest->headers.length = 0;
}

static inline void httpp_init_res(httpp_res_t* dest, httpp_header_t* headers_arr, size_t headers_capacity)
{
    dest->code = -1;
    dest->headers.arr = headers_arr;
    dest->headers.capacity = headers_capacity;
    dest->headers.length = 0;
}

#ifdef HTTPP_IMPLEMENTATION

// Originial isspace is kinda slow...
#define __isspace(c) ((c) == ' ') || ((c) == '\r') || ((c) == '\n')

#define ltrim(str) do {                    \
    while (*(str) && __isspace(*(str))) {  \
        (str)++;                           \
    }                                      \
    } while (0)


#define ltrim_buf(str, len) do {          \
    while (*(str) && __isspace(*(str))) { \
        (str)++;                          \
        (len)--;                          \
    }                                     \
    } while (0)

static char* __strdup(char* str) 
{
    if (!str)
        return NULL;

    size_t n = strlen(str) + 1;
    char* dupped = (char*) malloc(n);
    return dupped ? (char*) memcpy(dupped, str, n) : NULL;
}

const char* httpp_method_to_string(int method) 
{
    switch (method) {
        case HTTPP_METHOD_GET:      return "GET";
        case HTTPP_METHOD_HEAD:     return "HEAD";
        case HTTPP_METHOD_POST:     return "POST";
        case HTTPP_METHOD_PUT:      return "PUT";
        case HTTPP_METHOD_DELETE:   return "DELETE";
        case HTTPP_METHOD_CONNECT:  return "CONNECT";
        case HTTPP_METHOD_OPTIONS:  return "OPTIONS";
        case HTTPP_METHOD_TRACE:    return "TRACE";
        case HTTPP_METHOD_PATCH:    return "PATCH";

        case HTTPP_METHOD_UNKNOWN:
        default:                    return "UNKNOWN";
    }
}

const char* httpp_status_to_string(int status_code) 
{
    switch (status_code) {
        // Informational
        case 100: return "Continue";
        case 101: return "Switching Protocols";
        case 102: return "Processing";
        case 103: return "Early Hints";

        // Successful
        case 200: return "OK";
        case 201: return "Created";
        case 202: return "Accepted";
        case 203: return "Non-Authoritative Information";
        case 204: return "No Content";
        case 205: return "Reset Content";
        case 206: return "Partial Content";
        case 207: return "Multi-Status";
        case 208: return "Already Reported";
        case 226: return "IM Used";

        // Redirection
        case 300: return "Multiple Choices";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 303: return "See Other";
        case 304: return "Not Modified";
        case 305: return "Use Proxy";
        case 306: return "Unused";
        case 307: return "Temporary Redirect";
        case 308: return "Permanent Redirect";

        // Client Error
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 402: return "Payment Required";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 406: return "Not Acceptable";
        case 407: return "Proxy Authentication Required";
        case 408: return "Request Timeout";
        case 409: return "Conflict";
        case 410: return "Gone";
        case 411: return "Length Required";
        case 412: return "Precondition Failed";
        case 413: return "Content Too Large";
        case 414: return "URI Too Long";
        case 415: return "Unsupported Media Type";
        case 416: return "Range Not Satisfiable";
        case 417: return "Expectation Failed";
        case 418: return "I'm a teapot";
        case 421: return "Misdirected Request";
        case 422: return "Unprocessable Content";
        case 423: return "Locked";
        case 424: return "Failed Dependency";
        case 425: return "Too Early";
        case 426: return "Upgrade Required";
        case 428: return "Precondition Required";
        case 429: return "Too Many Requests";
        case 431: return "Request Header Fields Too Large";
        case 451: return "Unavailable For Legal Reasons";

        // Server Error 
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        case 504: return "Gateway Timeout";
        case 505: return "HTTP Version Not Supported";
        case 506: return "Variant Also Negotiates";
        case 507: return "Insufficient Storage";
        case 508: return "Loop Detected";
        case 510: return "Not Extended";
        case 511: return "Network Authentication Required";

        case -1:
        default: return "Unspecified";
    }
}

char* httpp_span_to_str(httpp_span_t* span)
{
    char* out = (char*) malloc(span->length + 1);

    if (!out)
        return NULL;

    memcpy(out, span->ptr, span->length);
    out[span->length] = '\0';
    return out;
}

httpp_header_t* httpp_headers_arr_append(httpp_headers_arr_t* hs, httpp_header_t header)
{
    if (hs->length >= hs->capacity)
        return NULL; // Array is full

    if (!header.name.ptr || !header.value.ptr) 
        return NULL; // Value is empty

    hs->arr[hs->length++] = header;
    return &hs->arr[hs->length - 1];
}

httpp_header_t* httpp_headers_arr_find(httpp_headers_arr_t* hs, char* name)
{
    // For the sake of simplicity and minimalism, it's just a for loop. No hash table here.
    size_t name_len = strlen(name);

    for (size_t i = 0; i < hs->length; i++) {
        httpp_span_t posible = hs->arr[i].name;

        if (posible.length != name_len)
            continue;

        if (strncasecmp(posible.ptr, name, posible.length) == 0)
            return &hs->arr[i];
    }

    return NULL;
}


httpp_header_t* httpp_res_add_header(httpp_res_t* res, char* name, char* value)
{
    if (!name || !value) 
        return NULL;
    
    char* mname = __strdup(name);
    if (!mname)
        return NULL;

    char* mvalue = __strdup(value);
    if (!mvalue) {
        free(mname);
        return NULL;
    }

    httpp_header_t h = {
        {mname, strlen(name)}, 
        {mvalue, strlen(value)}
    };

    httpp_header_t* out = httpp_headers_arr_append(&res->headers, h);

    if (out == NULL) {
        free(h.name.ptr);
        free(h.value.ptr);
    } 

    return out;
}

void httpp_res_free_added(httpp_res_t* res)
{
    for (size_t i = 0; i < res->headers.length; i++) {
        free(res->headers.arr[i].name.ptr);
        free(res->headers.arr[i].value.ptr);
    }
}

static int chop_until(char c, char** src, char* dest, size_t n) 
{
    char* pos = (char*) memchr(*src, c, n);
    if (!pos)
        return 1;

    size_t chopped_size = pos - *src;
    if (chopped_size >= n)
        return 1;

    memcpy(dest, *src, chopped_size);
    dest[chopped_size] = '\0';

    *src = pos + 1;
    return 0;
}

static int parse_start_line(char** itr, httpp_req_t* dest) 
{
    httpp_span_t route;
    char  method_buf[HTTPP_MAX_METHOD_LENGTH];
    char  version_buf[HTTPP_VERSION_BUFSIZE];

    if (chop_until(' ', itr, method_buf, HTTPP_MAX_METHOD_LENGTH) != 0)
        return 1; 

    route.ptr = *itr;
    char* space = strchr(*itr, ' ');
    
    if (!space)
        return 1;
    
    route.length = space - route.ptr;
    *itr = space + 1;

    if (chop_until('\r', itr, version_buf, HTTPP_VERSION_BUFSIZE) != 0)
        return 1;

    if (strcmp(version_buf, HTTPP_SUPPORTED_VERSION) != 0)
        return 1;

    dest->method = httpp_string_to_method(method_buf);
    dest->route = route;

    ltrim(*itr);
    return 0;
}

httpp_header_t* httpp_parse_header(httpp_headers_arr_t* dest, char* line, size_t content_len)
{
    // RFC says that header starting with whitespace or any other non printable ascii should be rejected.
    if (__isspace(*line))
        return NULL;

    char* colon = (char*) memchr(line, ':', content_len);
    if (!colon)
        return NULL;

    size_t name_len = colon - line;

    char* value_start = colon + 1;
    size_t value_len = content_len - name_len - 1;

    ltrim_buf(value_start, value_len);

    httpp_span_t name = {line, name_len};
    httpp_span_t value = {value_start, value_len};

    return httpp_headers_arr_append(dest, (httpp_header_t){name, value});
}

int httpp_parse_request(char* buf, size_t n, httpp_req_t* dest)
{
    if (n <= 0 || buf == NULL || dest == NULL)
        return 0;

    char* itr = buf;
    char* end = buf + n;

    if (parse_start_line(&itr, dest) != 0)
        return 1;

    while (itr < end) {
        char* del_pos = strstr(itr, _HTTP_DELIMITER);
        if (!del_pos)
            break;
        
        size_t line_size = del_pos - itr;
        if (line_size == 0) {
            itr = del_pos + _HTTP_DELIMITER_SIZE;
            break;
        }

        if (httpp_parse_header(&dest->headers, itr, line_size) == NULL)
            return 1;

        itr = del_pos + _HTTP_DELIMITER_SIZE;
    }

    dest->body.ptr = itr; // Itr now points at the beginning of the body
    dest->body.length = n - (itr - buf);

    return 0;
}

void httpp_res_set_body(httpp_res_t* res, char* body_ptr, size_t body_len)
{
    res->body = (httpp_span_t){body_ptr, body_len};
}

char* httpp_res_to_raw(httpp_res_t* res)
{
    if (res == NULL)
        return NULL; 

    if (res->code == -1 || res->code > 600)
        return NULL;

    const char* status_msg = httpp_status_to_string(res->code);
    size_t out_size =  
        strlen(HTTPP_SUPPORTED_VERSION) 
        + 1 // Space
        + _HTTP_MAX_STATUS_CODE_SIZE 
        + 1 // Space
        + strlen(status_msg)
        + _HTTP_DELIMITER_SIZE;

    for (size_t i = 0; i < res->headers.length; i++) {
        httpp_header_t header = res->headers.arr[i];
        
        if (!header.name.ptr || !header.value.ptr)
            continue;

        out_size += header.name.length + 2 // ": "
                  + header.value.length 
                  + _HTTP_DELIMITER_SIZE;
    }

    out_size += _HTTP_DELIMITER_SIZE;
    out_size += res->body.length;
    out_size += 1; // '\0'

    char* out = (char*) malloc(out_size);
    if (!out)
        return NULL;

    int written = snprintf(out, out_size, "%s %d %s\r\n", HTTPP_SUPPORTED_VERSION, res->code, status_msg);
    if (written < 0 || (size_t) written >= out_size) {
        free(out);
        return NULL;
    }

    size_t offset = written;

    for (size_t i = 0; i < res->headers.length; i++) {
        httpp_header_t header = res->headers.arr[i];
        if (!header.name.ptr || !header.value.ptr)
            continue;

        int n = snprintf(out + offset, out_size - offset, 
                    "%s: %s\r\n", header.name.ptr, header.value.ptr);

        if (n < 0 || (size_t) n >= out_size - offset) {
            free(out);
            return NULL;
        }

        offset += n;
    }

    strcat(out, _HTTP_DELIMITER);

    if (res->body.length) {
        char* end = out + offset + _HTTP_DELIMITER_SIZE;
        memcpy(end, res->body.ptr, res->body.length);
        end[res->body.length] = '\0';
    }
        
    return out;
}

#endif // HTTPP_IMPLEMENTATION
#endif // _HTTPP_HEADER

/*
* MIT License
* 
* Copyright (c) 2025 Mint
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/