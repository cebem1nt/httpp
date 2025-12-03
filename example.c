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
    httpp_req_t parsed;
    httpp_header_t headers[HTTPP_DEFAULT_HEADERS_ARR_CAP]; // Array for parsed headers
    httpp_init_req(&parsed, headers, HTTPP_DEFAULT_HEADERS_ARR_CAP); // Initialize the request on stack

    httpp_parse_request(req, strlen(req), &parsed); // Parse the request!

    printf("%s\n", parsed.body.ptr); // Pointer to the beginning of the body
    printf("%i\n", parsed.method);

    const char* method = httpp_method_to_string(parsed.method); // Method as string (e.g POST)
    printf("%s\n", method); 

    httpp_header_t* host = httpp_find_header(parsed, "Host"); 
    // Httpp stores pointers to the conentent in your buffer.
    // Each string is storred as "span" or a string view. 
    // to convert it to a null terminated string (malloc'd), use this:
    char* value = httpp_span_to_str(&host->value);
    printf("%s\n", value);

    free(value); // Dont forget to free it when you're done

    httpp_res_t response;
    httpp_header_t response_headers[2]; 
    httpp_init_res(&response, response_headers, 2);

    response.code = 200;
    httpp_res_add_header(&response, "Host", "somehost.some.where");
    httpp_header_t* status = httpp_res_add_header(&response, "Status", "ok");
    // If you try to add more headers than the capacity is, httpp_add_header will return NULL

    char* body = "Some body";
    httpp_res_set_body(&response, body, strlen(body)); // httpp_res_set_body sets the pointer to body, it doesn't copy it 

    char* raw = httpp_res_to_raw(&response); // Convert to a malloc'd raw string 

    printf("\nComposed response: \n");
    printf("----\n%s\n----\n", raw);  // Or write it to a socket
    printf("Body length = %lu\n", response.body.length);

    free(raw);

    // When using httpp_res_add_header, it strdups given name and value, so make sure to free it  
    httpp_res_free_added(&response); 
}