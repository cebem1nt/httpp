#include <stdio.h>
#include <stdlib.h>

#define HTTPP_IMPLEMENTATION
#include "httpp.h"

char* req = 
    "POST /api/items HTTP/1.1\r\n"
    "Host: api.example.com\r\n"
    "User-Agent: MyClient/1.0\r\n"
    "Content-Type: application/json\r\n"
    "Content-Length: 48\r\n"
    "\r\n"
    "{\"name\":\"Widget\",\"quantity\":10,\"price\":9.99}";

int main() 
{
    httpp_req_t* parsed = httpp_parse_request(req);
    printf("%s\n", parsed->body); // parsed, malloc'd body
    printf("%i\n", parsed->method); // Method is an enum!

    const char* method = httpp_method_to_string(parsed->method); // Method as string (e.g POST)
    printf("%s\n", method); 

    httpp_header_t* host = httpp_find_header(parsed, "Host"); 
    printf("%s\n", host->value); // api.example.com

    httpp_res_t* response = httpp_res_new();
    response->code = Ok; // or 200
    httpp_add_header(response, "Host", "somehost.some.where");
    httpp_add_header(response, "Status", "ok");
    
    httpp_res_set_body(response, "Some body");
    char* raw = httpp_res_to_raw(response); // Convert to a malloc'd raw string 

    printf("\nComposed response: \n");
    printf("----\n%s\n----\n", raw); // Or write it to a socket

    // Don't forget to free everything
    httpp_req_free(parsed); // Free parsed request
    httpp_res_free(response); // Free response structure
    free(raw); // Free composed raw response 
}