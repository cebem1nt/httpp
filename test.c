#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HTTPP_IMPLEMENTATION
#include "httpp.h"

void print(httpp_req_t* parsed) 
{
    if (parsed == NULL) {
        printf("Couldn't parse!\n");
        return;
    }

    char* route = httpp_span_to_str(&parsed->route);

    printf("Method: %i (%s)\n", parsed->method, httpp_method_to_string(parsed->method));
    printf("Route: %s\n", route);

    printf("Parsed headers length: %lu\n", parsed->headers.length);

    for (size_t i = 0; i < parsed->headers.length; i++) {
        httpp_header_t header = parsed->headers.arr[i];
        char* name = httpp_span_to_str(&header.name);
        char* value = httpp_span_to_str(&header.value);

        printf("Header %lu:\n", i);
        printf("   Name: :%s:\n", name);
        printf("  Value: :%s:\n", value);

        free(name);
        free(value);
    }

    printf("\n---- Parsed body: ----\n");
    printf("%s\n", parsed->body.ptr);
    printf("-------- (%lu) --------\n", parsed->body.length);

    assert(parsed->body.length == strlen(parsed->body.ptr)); // For our simple, plain text tests
}

int main() 
{
    char* req1 = 
    "POST /api/items HTTP/1.1\r\n"
    "Host: api.example.com\r\n"
    "User-Agent: MyClient/1.0\r\n"
    "Content-Type: application/json\r\n"
    "Content-Length: 48\r\n"
    "\r\n"
    "{\"name\":\"Widget\",\"quantity\":10,\"price\":9.99}";

    httpp_req_t parsed1;
    httpp_header_t arr1[HTTPP_DEFAULT_HEADERS_ARR_CAP];

    httpp_init_req(&parsed1, arr1, HTTPP_DEFAULT_HEADERS_ARR_CAP);

    httpp_parse_request(req1, strlen(req1), &parsed1);
    print(&parsed1);

    char* req2 = 
    "POST /api/items HTTP/1.1\r\n"
    "Host: api.example.com\r\n"
    "User-Agent: MyClient/1.0\r\n"
    "Content-Type: application/json; charset=utf-8\r\n"
    "Content-Length: 106\r\n"
    "X-Trace-ID: ;;--TRACE--;;\r\n"
    "X-Feature-Flags: ,enable-new, ,\r\n"
    "\r\n"
    "{\"name\":\"SpicyWidget\",\"quantity\":-1,\"price\":9.9900,\"tags\":[\"hot\",\"ßpecial\",\"null\",null],\"meta\":{\"note\":\"line1\\nline2\\r\\nline3\",\"empty\":\"\",\"unicode\":\"🔥🚀\",\"quote_test\":\"She said: \\\"Spicy!\\\"\"}}";
    
    httpp_req_t parsed2;
    httpp_header_t arr2[HTTPP_DEFAULT_HEADERS_ARR_CAP];

    httpp_init_req(&parsed2, arr2, HTTPP_DEFAULT_HEADERS_ARR_CAP);

    httpp_parse_request(req2, strlen(req2), &parsed2);
    print(&parsed2);

    char* req3 = 
    "POST /api/items HTTP/1.1\r\n"
    "Host: api.example.com\r\n"
    "User-Agent: MyClient/1.0\r\n"
    "Content-Type: application/json; charset=utf-8\r\n"
    "Content-Length: 106\r\n"
    "X-Trace-ID: ;;--TRACE--;;\r\n"
    "X-Feature-Flags: ,enable-new, ,\r\n"
    "\r\n";

    printf("--- req3 ---\n");
    httpp_req_t parsed3;
    httpp_header_t arr3[HTTPP_DEFAULT_HEADERS_ARR_CAP];

    httpp_init_req(&parsed3, arr3, HTTPP_DEFAULT_HEADERS_ARR_CAP);

    httpp_parse_request(req3, strlen(req3), &parsed3);
    print(&parsed3);
    printf("--- end ---\n");

    httpp_res_t res;
    httpp_header_t arr[HTTPP_DEFAULT_HEADERS_ARR_CAP];

    httpp_init_res(&res, arr, HTTPP_DEFAULT_HEADERS_ARR_CAP);
    
    res.code = 200;
    httpp_res_add_header(&res, "Host", "idk.me.com");
    httpp_res_add_header(&res, "Home", "pkeofkwekgfwktokwt9wt293430592304");
    httpp_res_add_header(&res, "SOmethin", "afkofkeokfoekfo");

    char* body = "{\"hello\": 123}\n";
    httpp_res_set_body(&res, body, strlen(body));

    char* raw = httpp_res_to_raw(&res);
    printf("-----------\n");
    printf("%s", raw);
    printf("-----------\n");

    return 0;
}