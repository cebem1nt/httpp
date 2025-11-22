/*
 * Tiny header only http parser library for 
 * http 1/1 version
 */

#include <stddef.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define _HTTPP_LINE_BUFSIZE 4096
#define _HTTPP_INITIAL_HEADERS_ARR_CAP 20

#define _HTTPP_MAX_METHOD_LENGTH (10 + 1)
#define _HTTPP_VERSION_BUFSIZE (10 + 1)

#define httpp_string_to_method(s) (strcmp(s, "GET")    == 0 ? 0 : \
                                   strcmp(s, "POST")   == 0 ? 1 : \
                                   strcmp(s, "DELETE") == 0 ? 2 : -1)

typedef enum {
    GET,
    POST,
    DELETE,
    UNKNOWN = -1
} http_method_t;

typedef struct {
    char* name;
    char* value;
} http_header_t;

typedef struct {
    http_header_t* arr;
    size_t capacity;
    size_t length;
} http_headers_arr_t;

typedef struct {
    http_method_t method;
    http_headers_arr_t* headers;
    char* route;
    char* body;
} http_req_t;

const char* httpp_method_to_string(http_method_t m);
http_req_t* httpp_req_new();
http_req_t* httpp_parse_request(char* raw);

int httpp_parse_header(http_headers_arr_t* hs, char* line);
void httpp_headers_append(http_headers_arr_t* hs, http_header_t header);
void httpp_req_free(http_req_t* req);

#ifdef HTTPP_IMPLEMENTATION
#define HTTP_DELIMITER "\r\n"
#define HTTP_DELIMITER_SIZE 2
#define HTTP_VERSION "HTTP/1.1"

#define nomem() do { fprintf(stderr, "No memory, see ya!\n"); exit(1); } while (0)
#define ltrim(str)                     \
    while(*(str) && isspace(*(str))) { \
        (str)++;                       \
    }

#define rtrim(str) do {                     \
    char* end = (str) + strlen(str) - 1;    \
    while (end >= (str) && isspace(*end)) { \
        *end = '\0';                        \
        --end;                              \
    }                                       \
    } while (0)

static inline void* emalloc(size_t size) 
{
    void* ptr = malloc(size);
    if (!ptr)
        nomem();

    return ptr;
}

static inline void* erealloc(void* ptr, size_t size) 
{
    void* new_ptr = realloc(ptr, size);
    if (!new_ptr)
        nomem();

    return new_ptr;
} 

static int chop_until(char c, char** src, char* dest, size_t n) 
{
    char* pos = strchr(*src, c);
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

static char* dchop_until(char c, char** src) 
{
    char* pos = strchr(*src, c);
    if (!pos)
        return NULL;

    size_t chopped_size = pos - *src;
    char* out = (char*) emalloc(chopped_size + 1);

    memcpy(out, *src, chopped_size);
    out[chopped_size] = '\0';

    *src = pos + 1;
    return out;
}

const char* httpp_method_to_string(http_method_t m) 
{
    switch (m) {
        case GET: return "GET";
        case POST: return "POST"; 
        case DELETE: return "DELETE"; 
        default: return "UNKNOWN";
    }
}

http_req_t* httpp_req_new() 
{
    http_req_t* out = (http_req_t*) emalloc(sizeof(http_req_t));

    out->headers = (http_headers_arr_t*) emalloc(sizeof(http_headers_arr_t));
    out->headers->arr = (http_header_t*) emalloc(sizeof(http_header_t) * _HTTPP_INITIAL_HEADERS_ARR_CAP);
    out->headers->capacity = _HTTPP_INITIAL_HEADERS_ARR_CAP;
    out->headers->length = 0;
    out->route = NULL;
    out->body = NULL;

    return out;
}

void httpp_req_free(http_req_t* req) 
{
    if (!req) return;

    for (size_t i = 0; i < req->headers->length; i++) {
        free(req->headers->arr[i].name);
        free(req->headers->arr[i].value);
    }

    free(req->headers->arr);
    free(req->headers);
    free(req->route);
    free(req->body);
    free(req);
}

void httpp_headers_append(http_headers_arr_t* hs, http_header_t header) 
{
    if (hs->length >= hs->capacity) {
        size_t new_cap = hs->capacity * 2;
        hs->arr = (http_header_t*) erealloc(hs->arr, new_cap * sizeof(http_header_t));
        hs->capacity = new_cap;
    }

    hs->arr[hs->length++] = header;
}

int httpp_parse_header(http_headers_arr_t* hs, char* line) 
{
    char* delim_pos = strchr(line, ':');
    if (!delim_pos)
        return 1;
    
    size_t name_len = (delim_pos - line);
    if (name_len == 0)
        return 2;
    
    char* name = (char*) emalloc(name_len + 1);
    memcpy(name, line, name_len);
    name[name_len] = '\0';

    char* value_start = delim_pos + 1;
    ltrim(value_start);

    char* value = strdup(value_start);
    if (!value)
        nomem();

    httpp_headers_append(hs, (http_header_t){name, value});
    return 0;
}

static int http_parse_start_line(char** itr, http_req_t* dest) 
{
    char method_buf[_HTTPP_MAX_METHOD_LENGTH];
    char version_buf[_HTTPP_VERSION_BUFSIZE];
    char* route;

    ltrim(*itr);

    if (chop_until(' ', itr, method_buf, _HTTPP_MAX_METHOD_LENGTH) != 0)
        return 1; 

    ltrim(*itr); // Route might have extra spaces at the bginning, for our implementation thats fine
    if ((route = dchop_until(' ', itr)) == NULL)
        return 1;

    ltrim(*itr);
    if (chop_until('\n', itr, version_buf, _HTTPP_VERSION_BUFSIZE) != 0) {
        free(route);
        return 1;
    }

    rtrim(version_buf);
    if (strcmp(version_buf, HTTP_VERSION) != 0) {
        free(route);
        return 1;
    }

    dest->method = (http_method_t) httpp_string_to_method(method_buf);
    dest->route = route;
    return 0;
}

http_req_t* httpp_parse_request(char* raw)
{
    http_req_t* out = httpp_req_new();

    char* itr = raw;
    char* end = raw + strlen(raw);
    char  line[_HTTPP_LINE_BUFSIZE];

    if (http_parse_start_line(&itr, out) != 0) {
        httpp_req_free(out);
        return NULL;
    }

    while (itr < end) {
        char* del_pos = strstr(itr, HTTP_DELIMITER);
        if (!del_pos)
            break;
        
        size_t line_size = del_pos - itr;
        if (line_size == 0) {
            itr = del_pos + HTTP_DELIMITER_SIZE;
            break;
        }

        if (line_size >= _HTTPP_LINE_BUFSIZE) {
            httpp_req_free(out);
            return NULL;
        }
        
        memcpy(line, itr, line_size);
        line[line_size] = '\0';

        if (httpp_parse_header(out->headers, line) != 0) {
            httpp_req_free(out);
            return NULL;
        }

        itr = del_pos + HTTP_DELIMITER_SIZE;
    }

    // TODO, there is a thing called chunked transfer, which is 
    // definitely not handled well by this parser
    out->body = strdup(itr);
    if (!out->body) {
        httpp_req_free(out);
        return NULL;
    }

    return out;
}
#endif // HTTPP_IMPLEMENTATION