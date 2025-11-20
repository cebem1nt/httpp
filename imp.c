#include <assert.h>
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "httpp.h"

#define HTTP_DELIMETER "\r\n"
#define HTTP_DELIMETER_SIZE 2
#define HTTP_VERSION "HTTP/1.1"

#define LINE_BUFSIZE 4096
#define INITIAL_HEADERS_ARR_CAP 20

#define MAX_ROUTE_LENGTH 100 /* Can and should be adjusted */
#define MAX_METHOD_LENGTH 10 + 1
#define VERISON_BUFSIZE 10 + 1

#define http_string_to_method(s) (strcmp(s, "GET")    == 0 ? 0 : \
                                  strcmp(s, "POST")   == 0 ? 1 : \
                                  strcmp(s, "DELETE") == 0 ? 2 : -1)

#define nomem() do { fprintf(stderr, "No memory, see ya!\n"); exit(1); } while (0)
#define ltrim(str) while(*(str) && isspace(*(str))) { (str)++; }

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
    if (!ptr)
        nomem();

    return ptr;
} 

static void chop_until(char c, char** src, char* dest, size_t dest_len) 
{
    char* pos = strchr(*src, c);
    if (!pos)
        return;

    size_t chopped_size = pos - *src;
    if (chopped_size >= dest_len)
        return;

    memcpy(dest, *src, chopped_size);
    dest[chopped_size] = '\0';

    *src = pos + 1;
}

const char* http_method_to_string(http_method_t m) 
{
    switch (m) {
        case GET: return "GET";
        case POST: return "POST"; 
        case DELETE: return "DELETE"; 
    }
}        

http_req_t* http_req_new() 
{
    http_req_t* new = emalloc(sizeof(http_req_t));

    new->headers = emalloc(sizeof(http_headers_arr_t));
    new->headers->arr = emalloc(sizeof(http_header_t) * INITIAL_HEADERS_ARR_CAP);
    new->headers->capacity = INITIAL_HEADERS_ARR_CAP;
    new->headers->length = 0;

    return new;
}

void http_req_arr_append(http_headers_arr_t* hs, http_header_t header) 
{
    if (hs->length >= hs->capacity) {
        size_t new_cap = (hs->capacity * 2) * sizeof(http_header_t);
        hs->arr = erealloc(hs->arr, new_cap);
        hs->capacity = new_cap;
    }

    hs->arr[hs->length++] = header;
}

void http_parse_header(http_req_t* req, char* line) 
{
    char* delim_pos = strchr(line, ':');
    if (!delim_pos)
        return;
    
    size_t name_len = (delim_pos - line);
    if (name_len == 0)
        return;
    
    char* name = emalloc(name_len + 1);
    memcpy(name, line, name_len);
    name[name_len] = '\0';

    char* value_start = delim_pos + 1;
    ltrim(value_start);

    char* value = strdup(value_start);
    if (!value)
        nomem();

    http_header_t parsed = {name, value};
    http_req_arr_append(req->headers, parsed);
}

void http_parse_start_line(char** itr, http_req_t* dest) 
{
    char method_buf[MAX_METHOD_LENGTH];
    char route_buf[MAX_ROUTE_LENGTH];
    char version_buf[VERISON_BUFSIZE];

    ltrim(*itr);

    chop_until(' ',  itr, method_buf, MAX_METHOD_LENGTH);
    chop_until(' ',  itr, route_buf, MAX_ROUTE_LENGTH);
    chop_until('\r', itr, version_buf, VERISON_BUFSIZE);

    dest->method = http_string_to_method(method_buf);
    dest->route = strdup(route_buf); // Hmmmm

    assert(strcmp(version_buf, HTTP_VERSION) == 0 && "Unsupported protocol version");
    ltrim(*itr); // remove \n left after version
}

http_req_t* http_parse_request(char* raw) 
{
    http_req_t* out = http_req_new();

    char* itr = raw;
    char* end = raw + strlen(raw);
    char  line[LINE_BUFSIZE];

    http_parse_start_line(&itr, out);

    while (itr < end) {
        char* del_pos = strstr(itr, HTTP_DELIMETER);
        if (!del_pos)
            break;
        
        size_t line_size = del_pos - itr;
        
        if (line_size >= LINE_BUFSIZE)
            return NULL; // Idk, inform user?
        
        memcpy(line, itr, line_size);
        line[line_size] = '\0';

        http_parse_header(out, line);
        itr = del_pos + HTTP_DELIMETER_SIZE;
    }

    out->body = strdup(itr);
    return out;
}

int main() 
{
    char* req = 
    "POST /api/items HTTP/1.1\r\n"
    "Host: api.example.com\r\n"
    "User-Agent: MyClient/1.0\r\n"
    "Content-Type: application/json\r\n"
    "Content-Length: 48\r\n"
    "\r\n"
    "{\"name\":\"Widget\",\"quantity\":10,\"price\":9.99}";

    http_req_t* parsed = http_parse_request(req);

    printf("Method: %i (%s)\n", parsed->method, http_method_to_string(parsed->method));
    printf("Route: %s\n", parsed->route);

    printf("Parsed headers length: %lu\n", parsed->headers->length);
    for (size_t i = 0; i < parsed->headers->length; i++) {
        printf("Header %lu:\n", i);
        printf("   Name: %s\n", parsed->headers->arr[i].name);
        printf("  Value: %s\n", parsed->headers->arr[i].value);
    }

    printf("\n--- Parsed body: ---\n");
    printf("%s\n", parsed->body);
    printf("----------------------\n");

    return 0;
}