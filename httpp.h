/*
 * Tiny header only http parser library for 
 * http 1/1 version
 */

#include <stddef.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HTTPP_LINE_BUFSIZE 4096
#define HTTPP_INITIAL_HEADERS_ARR_CAP 20

#define HTTPP_MAX_METHOD_LENGTH (10 + 1)
#define HTTPP_VERSION_BUFSIZE (10 + 1)

#define httpp_string_to_method(s) (strcmp(s, "GET")    == 0 ? 0 : \
                                   strcmp(s, "POST")   == 0 ? 1 : \
                                   strcmp(s, "DELETE") == 0 ? 2 : -1)

#define HTTPP_ERRMEMRY  3
#define HTTPP_ERRLOGIC  1

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
int httpp_headers_append(http_headers_arr_t* hs, http_header_t header);
void http_headers_arr_free(http_headers_arr_t* hs);
void httpp_req_free(http_req_t* req);

#define HTTPP_IMPLEMENTATION
#ifdef HTTPP_IMPLEMENTATION
#define HTTP_DELIMITER "\r\n"
#define HTTP_DELIMITER_SIZE 2
#define HTTP_VERSION "HTTP/1.1"

#define trim(str) do { ltrim(str); rtrim(str); } while (0)

#define ltrim(str) do {                     \
    char* __str = (str);                    \
    while(*__str && isspace(*__str)) {      \
        __str++;                            \
    }                                       \
    } while (0)


#define rtrim(str) do {                     \
    char* __str = (str);                    \
    char* end = __str + strlen(__str) - 1;  \
    while (end >= __str && isspace(*end)) { \
        *end-- = '\0';                      \
    }                                       \
    } while (0)

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
    char* out = (char*) malloc(chopped_size + 1);
    if (!out)
        return NULL;

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
    http_req_t* out = (http_req_t*) malloc(sizeof(http_req_t));
    if (!out)
        return NULL;

    out->headers = (http_headers_arr_t*) malloc(sizeof(http_headers_arr_t));
    if (!out->headers) {
        free(out);
        return NULL;
    }

    out->headers->arr = (http_header_t*) malloc(sizeof(http_header_t) * HTTPP_INITIAL_HEADERS_ARR_CAP);
    if (!out->headers->arr) {
        free(out->headers);
        free(out);
        return NULL;
    }

    out->headers->capacity = HTTPP_INITIAL_HEADERS_ARR_CAP;
    out->headers->length = 0;
    out->route = NULL;
    out->body = NULL;

    return out;
}

void http_headers_arr_free(http_headers_arr_t* hs)
{
    if (!hs) return;
    for (size_t i = 0; i < hs->length; i++) {
        free(hs->arr[i].name);
        free(hs->arr[i].value);
    }
}

void httpp_req_free(http_req_t* req)
{
    if (!req) return;
    http_headers_arr_free(req->headers);

    free(req->headers->arr);
    free(req->headers);
    free(req->route);
    free(req->body);
    free(req);
}

int httpp_headers_append(http_headers_arr_t* hs, http_header_t header)
{
    if (hs->length >= hs->capacity) {
        size_t new_cap = hs->capacity * 2;
        if (new_cap <= hs->capacity) // Doesn't free on failure. Make a note about it
            return HTTPP_ERRMEMRY;

        hs->arr = (http_header_t*) realloc(hs->arr, new_cap * sizeof(http_header_t));
        if (!hs->arr)
            return HTTPP_ERRMEMRY;
        
        hs->capacity = new_cap;
    }

    hs->arr[hs->length++] = header;
    return 0;
}

int httpp_parse_header(http_headers_arr_t* hs, char* line) 
{
    char* delim_pos = strchr(line, ':');
    if (!delim_pos)
        return HTTPP_ERRLOGIC;
    
    size_t name_len = (delim_pos - line);
    if (name_len == 0)
        return HTTPP_ERRLOGIC;
    
    char* name = (char*) malloc(name_len + 1);
    if (!name)
        return HTTPP_ERRMEMRY;

    memcpy(name, line, name_len);
    name[name_len] = '\0';
    trim(name);

    char* value_start = delim_pos + 1;
    trim(value_start);

    char* value = strdup(value_start);
    if (!value)
        return HTTPP_ERRMEMRY;

    if (httpp_headers_append(hs, (http_header_t){name, value}) != 0)
        return HTTPP_ERRMEMRY;

    return 0;
}

static int http_parse_start_line(char** itr, http_req_t* dest) 
{
    char method_buf[HTTPP_MAX_METHOD_LENGTH];
    char version_buf[HTTPP_VERSION_BUFSIZE];
    char* route;

    ltrim(*itr);
    if (chop_until(' ', itr, method_buf, HTTPP_MAX_METHOD_LENGTH) != 0)
        return 1; 

    ltrim(*itr); // Route might have extra spaces at the bginning, for our implementation thats fine
    if ((route = dchop_until(' ', itr)) == NULL)
        return 1;

    ltrim(*itr);
    if (chop_until('\n', itr, version_buf, HTTPP_VERSION_BUFSIZE) != 0) {
        free(route);
        return 1;
    }

    trim(version_buf);
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
    char  line[HTTPP_LINE_BUFSIZE];

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

        if (line_size >= HTTPP_LINE_BUFSIZE) {
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