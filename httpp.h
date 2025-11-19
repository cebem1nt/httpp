/*
 * Tiny header only http parser library for 
 * http 1/1 version
 */

#include <stddef.h>

typedef enum {
    GET,
    POST,
    DELETE
} http_method_t;

typedef struct {
    char* name;
    char* body;
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

http_req_t* http_parse_request(char* raw);

#ifdef LIBPTTH_IMPLEMENTATION


#endif // LIBPTTH_IMPLEMENTATION