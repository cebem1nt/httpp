#include <stdio.h>

#define LIBPTTH_IMPLEMENTATION
#include "httpp.h"

void print(http_req_t* parsed) 
{
    if (parsed == NULL) {
        printf("Couldn't parse!\n");
        return;
    }

    printf("Method: %i (%s)\n", parsed->method, httpp_method_to_string(parsed->method));
    printf("Route: %s\n", parsed->route);

    printf("Parsed headers length: %lu\n", parsed->headers->length);
    for (size_t i = 0; i < parsed->headers->length; i++) {
        printf("Header %lu:\n", i);
        printf("   Name: :%s:\n", parsed->headers->arr[i].name);
        printf("  Value: :%s:\n", parsed->headers->arr[i].value);
    }

    printf("\n---- Parsed body: ----\n");
    printf("%s\n", parsed->body);
    printf("-------- (%lu) --------\n", strlen(parsed->body));
}

int main() 
{
    char* req1 = 
    "POST /api/items HTTP/1.1 \r\n"
    "Host: api.example.com\r\n"
    "User-Agent: MyClient/1.0\r\n"
    "Content-Type: application/json\r\n"
    "Content-Length: 48\r\n"
    "\r\n"
    "{\"name\":\"Widget\",\"quantity\":10,\"price\":9.99}";

    http_req_t* parsed = httpp_parse_request(req1);
    print(parsed);
    httpp_req_free(parsed);

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

    parsed = httpp_parse_request(req2);
    print(parsed);

    return 0;
}