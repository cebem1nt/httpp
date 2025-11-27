/*
 * Tiny header only http parser library for
 * http 1/1 version (enums don't respect your namespace much, sorry)
 * Pipelined requests are not supported because nobody realy cares about them 
 * (https://en.wikipedia.org/wiki/HTTP_pipelining#Implementation_status)
 * 
 * Chunked transfer is not really supported
 *
 * TODO Folded headers nor handled
 */

#include <stddef.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HTTPP_LINE_BUFSIZE 4096 /* A buffer for "name: value" header parsing */
#define HTTPP_INITIAL_HEADERS_ARR_CAP 20
#define HTTPP_HEADERS_ARR_LIMIT -1

#define HTTPP_MAX_METHOD_LENGTH (10 + 1)
#define HTTPP_VERSION_BUFSIZE (10 + 1)
#define HTTPP_SUPPORTED_VERSION "HTTP/1.1"

#define _HTTP_DELIMITER "\r\n"
#define _HTTP_DELIMITER_SIZE 2
#define _HTTP_MAX_STATUS_CODE_SIZE 3

#define httpp_string_to_method(s) (strcmp(s, "GET")    == 0 ? 0 : \
                                   strcmp(s, "POST")   == 0 ? 1 : \
                                   strcmp(s, "DELETE") == 0 ? 2 : -1)

#define HTTPP_ERRMEMRY  3
#define HTTPP_ERRLOGIC  2
#define HTTPP_ERRDEFLT  1

typedef enum {
    GET,
    POST,
    DELETE,
    UNKNOWN = -1
} httpp_method_t;

typedef enum {
    Continue = 100,
    Switching_Protocols,

    Ok = 200,
    Created,
    Accepted,

    Multiple_Choices = 300,
    Moved_Permanently,
    Found,
    See_Other,
    Not_Modified,
    Use_Proxy,
    Temporary_Redirect = 307,
    Permanent_Redirect,

    Bad_Request = 400,
    Unauthorized,
    Payment_Required, // Ahahaha
    Forbidden,
    Not_Found,

    Internal_Server_Error = 500,
    Not_Implemented,
    Bad_Gateway,
    Service_Unavailable,
    Gateway_Timeout,
    HTTP_Version_Not_Supported,

    Unspecified = -1
} httpp_status_t;

typedef struct {
    char* name;
    char* value;
} httpp_header_t;

typedef struct {
    httpp_header_t* arr;
    size_t capacity;
    size_t length;
} httpp_headers_arr_t;

typedef struct {
    httpp_method_t method;
    httpp_headers_arr_t* headers;
    char* route;
    char* body;
} httpp_req_t;

typedef struct {
    httpp_headers_arr_t* headers;
    httpp_status_t code;
    char* body;
} httpp_res_t;

const char* httpp_method_to_string(httpp_method_t m);
const char* httpp_status_to_string(httpp_status_t s);

httpp_req_t* httpp_req_new();
void httpp_req_free(httpp_req_t* req);
httpp_req_t* httpp_parse_request(char* raw);

httpp_header_t* httpp_parse_header(httpp_headers_arr_t* hs, char* line);

httpp_headers_arr_t* httpp_headers_arr_new();
void httpp_headers_arr_free(httpp_headers_arr_t* hs);
httpp_header_t* httpp_headers_append(httpp_headers_arr_t* hs, httpp_header_t header);
httpp_header_t* httpp_headers_add(httpp_headers_arr_t* hs, char* name, char* value); // stdrup and httpp_headers_append

httpp_res_t* httpp_res_new();
int httpp_res_set_body(httpp_res_t* res, char* body); // strdup and res->body = dupped
void httpp_res_free(httpp_res_t* res);
char* httpp_res_to_raw(httpp_res_t* res);

#define HTTPP_IMPLEMENTATION
#ifdef HTTPP_IMPLEMENTATION

#define trim(str) do { ltrim(str); rtrim(str); } while (0)

#define ltrim(str) do {                     \
    while(*(str) && isspace(*(str))) {      \
        (str)++;                            \
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
        return HTTPP_ERRDEFLT;

    size_t chopped_size = pos - *src;
    if (chopped_size >= n)
        return HTTPP_ERRDEFLT;

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

const char* httpp_method_to_string(httpp_method_t m) 
{
    switch (m) {
        case GET:       return "GET";
        case POST:      return "POST"; 
        case DELETE:    return "DELETE"; 
        default:        return "UNKNOWN";
    }
}

const char* httpp_status_to_string(httpp_status_t s) 
{
    switch (s) {
        case Continue:                   return "Continue";
        case Switching_Protocols:        return "Switching Protocols";

        case Ok:                         return "OK";
        case Created:                    return "Created";
        case Accepted:                   return "Accepted";

        case Multiple_Choices:           return "Multiple Choices";
        case Moved_Permanently:          return "Moved Permanently";
        case Found:                      return "Found";
        case See_Other:                  return "See Other";
        case Not_Modified:               return "Not Modified";
        case Use_Proxy:                  return "Use Proxy";
        case Temporary_Redirect:         return "Temporary Redirect";
        case Permanent_Redirect:         return "Permanent Redirect";

        case Bad_Request:                return "Bad Request";
        case Unauthorized:               return "Unauthorized";
        case Payment_Required:           return "Payment Required";
        case Forbidden:                  return "Forbidden";
        case Not_Found:                  return "Not Found";

        case Internal_Server_Error:      return "Internal Server Error";
        case Not_Implemented:            return "Not Implemented";
        case Bad_Gateway:                return "Bad Gateway";
        case Service_Unavailable:        return "Service Unavailable";
        case Gateway_Timeout:            return "Gateway Timeout";
        case HTTP_Version_Not_Supported: return "HTTP Version Not Supported";

        case Unspecified:
        default:                         return "Unspecified";
    }
}


httpp_headers_arr_t* httpp_headers_arr_new()
{
    httpp_headers_arr_t* out = (httpp_headers_arr_t*) malloc(sizeof(httpp_headers_arr_t));

    out->arr = (httpp_header_t*) malloc(sizeof(httpp_header_t) * HTTPP_INITIAL_HEADERS_ARR_CAP);
    if (!out->arr) {
        free(out);
        return NULL;
    }

    out->capacity = HTTPP_INITIAL_HEADERS_ARR_CAP;
    out->length = 0;
    return out;
}

httpp_req_t* httpp_req_new() 
{
    httpp_req_t* out = (httpp_req_t*) malloc(sizeof(httpp_req_t));
    if (!out)
        return NULL;

    out->headers = httpp_headers_arr_new();
    if (!out->headers) {
        free(out);
        return NULL;
    }

    out->route = NULL;
    out->body = NULL;
    return out;
}

void httpp_headers_arr_free(httpp_headers_arr_t* hs)
{
    if (!hs) return;

    for (size_t i = 0; i < hs->length; i++) {
        free(hs->arr[i].name);
        free(hs->arr[i].value);
    }
}

void httpp_req_free(httpp_req_t* req)
{
    if (!req) return;
    httpp_headers_arr_free(req->headers);

    free(req->headers->arr);
    free(req->headers);
    free(req->route);
    free(req->body);
    free(req);
}

httpp_header_t* httpp_headers_append(httpp_headers_arr_t* hs, httpp_header_t header)
{
    if (hs->length >= hs->capacity) {
        size_t new_cap = hs->capacity * 2;
        if (new_cap <= hs->capacity) // Doesn't free on failure. Make a note about it
            return NULL;

        hs->arr = (httpp_header_t*) realloc(hs->arr, new_cap * sizeof(httpp_header_t));
        if (!hs->arr)
            return NULL;
        
        hs->capacity = new_cap;
    }

    if (!header.name || !header.value) 
        return NULL;

    hs->arr[hs->length++] = header;
    return &hs->arr[hs->length];
}

httpp_header_t* httpp_headers_add(httpp_headers_arr_t* hs, char* name, char* value)
{
    if (!name || !value) 
        return NULL;

    httpp_header_t h = {
        strdup(name), 
        strdup(value)
    };

    return httpp_headers_append(hs, h);
}

httpp_header_t* httpp_parse_header(httpp_headers_arr_t* hs, char* line)
{
    char* delim_pos = strchr(line, ':');
    if (!delim_pos)
        return NULL;
    
    size_t name_len = (delim_pos - line);
    if (name_len == 0)
        return NULL;
    
    char* name = (char*) malloc(name_len + 1);
    if (!name)
        return NULL;

    memcpy(name, line, name_len);
    name[name_len] = '\0';
    trim(name);

    char* value_start = delim_pos + 1;
    trim(value_start);

    char* value = strdup(value_start);
    if (!value)
        return NULL;
    
    return httpp_headers_append(hs, (httpp_header_t){name, value});
}

static int parse_start_line(char** itr, httpp_req_t* dest) 
{
    char method_buf[HTTPP_MAX_METHOD_LENGTH];
    char version_buf[HTTPP_VERSION_BUFSIZE];
    char* route;

    ltrim(*itr);
    if (chop_until(' ', itr, method_buf, HTTPP_MAX_METHOD_LENGTH) != 0)
        return HTTPP_ERRDEFLT; 

    ltrim(*itr); // Route might have extra spaces at the bginning, for our implementation thats fine
    if ((route = dchop_until(' ', itr)) == NULL)
        return HTTPP_ERRDEFLT;

    ltrim(*itr);
    if (chop_until('\n', itr, version_buf, HTTPP_VERSION_BUFSIZE) != 0) {
        free(route);
        return HTTPP_ERRDEFLT;
    }

    rtrim(version_buf);
    if (strcmp(version_buf, HTTPP_SUPPORTED_VERSION) != 0) {
        free(route);
        return HTTPP_ERRDEFLT;
    }

    dest->method = (httpp_method_t) httpp_string_to_method(method_buf);
    dest->route = route;
    return 0;
}

httpp_req_t* httpp_parse_request(char* raw)
{
    httpp_req_t* out = httpp_req_new();

    char* itr = raw;
    char* end = raw + strlen(raw);
    char  line[HTTPP_LINE_BUFSIZE];

    if (parse_start_line(&itr, out) != 0) {
        httpp_req_free(out);
        return NULL;
    }

    while (itr < end) {
        char* del_pos = strstr(itr, _HTTP_DELIMITER);
        if (!del_pos)
            break;
        
        size_t line_size = del_pos - itr;
        if (line_size == 0) {
            itr = del_pos + _HTTP_DELIMITER_SIZE;
            break;
        }

        if (line_size >= HTTPP_LINE_BUFSIZE) {
            httpp_req_free(out);
            return NULL;
        }
        
        memcpy(line, itr, line_size);
        line[line_size] = '\0';

        if (httpp_parse_header(out->headers, line) == NULL) {
            httpp_req_free(out);
            return NULL;
        }

        if (HTTPP_HEADERS_ARR_LIMIT != -1 && out->headers->length >= HTTPP_HEADERS_ARR_LIMIT) {
            httpp_req_free(out); // Limit reached
            return NULL;
        }

        itr = del_pos + _HTTP_DELIMITER_SIZE;
    }

    out->body = strdup(itr);
    if (!out->body) {
        httpp_req_free(out);
        return NULL;
    }

    return out;
}

// --- Responses part ---
httpp_res_t* httpp_res_new()
{
    httpp_res_t* out = (httpp_res_t*) malloc(sizeof(httpp_res_t));
    if (!out)
        return NULL;

    out->headers = httpp_headers_arr_new();
    if (!out->headers) 
        return NULL;
    
    out->code = Unspecified;
    return out;
}

void httpp_res_free(httpp_res_t* res) 
{
    if (!res)
        return;

    httpp_headers_arr_free(res->headers);
    free(res->body);
}

int httpp_res_set_body(httpp_res_t* res, char* body) 
{
    res->body = strdup(body);
    if (!res->body)
        return 1;

    return 0;
}

char* httpp_res_to_raw(httpp_res_t* res)
{
    if (res == NULL || res->headers == NULL)
        return NULL; 

    if (res->code == -1 || (int) res->code > 999)
        return NULL;

    const char* status_msg = httpp_status_to_string(res->code);
    size_t out_size =  
        strlen(HTTPP_SUPPORTED_VERSION) 
        + 1 // Space
        + _HTTP_MAX_STATUS_CODE_SIZE 
        + 1 // Space
        + strlen(status_msg)
        + _HTTP_DELIMITER_SIZE;

    for (size_t i = 0; i < res->headers->length; i++) {
        httpp_header_t header = res->headers->arr[i];
        if (!header.name || !header.value)
            continue;

        out_size += strlen(header.name) + 2 // ": "
                  + strlen(header.value) 
                  + _HTTP_DELIMITER_SIZE;
    }

    out_size += _HTTP_DELIMITER_SIZE;
    if (res->body) 
        out_size += strlen(res->body);

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

    for (size_t i = 0; i < res->headers->length; i++) {
        httpp_header_t header = res->headers->arr[i];
        if (!header.name || !header.value)
            continue;

        int n = snprintf(out + offset, out_size - offset, 
                    "%s: %s\r\n", header.name, header.value);

        if (n < 0 || (size_t) n >= out_size - offset) {
            free(out);
            return NULL;
        }

        offset += n;
    }

    strcat(out, _HTTP_DELIMITER);
    if (res->body)
        strcat(out, res->body);

    return out;
}

#endif // HTTPP_IMPLEMENTATION