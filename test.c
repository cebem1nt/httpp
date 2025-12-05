#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HTTPP_IMPLEMENTATION
#include "httpp.h"

int tests_run = 0;
int tests_failed = 0;

const char* current_test_name = NULL;

#define TEST(name) \
    for (current_test_name = name; current_test_name; current_test_name = NULL) \
        if (1)

#define ASSERT(expr) do { \
    tests_run++; \
    if (!(expr)) { \
        fprintf(stderr, "[FAIL]: %d: %s: assertion `%s` failed\n", __LINE__, current_test_name, #expr); \
        tests_failed++; \
    } \
} while (0)

#define ASSERT_EQ_STR(a, b) ASSERT((a) != NULL && (b) != NULL && strcmp((a), (b)) == 0)
#define ASSERT_EQ_INT(a, b) ASSERT((a) == (b))
#define ARR_LEN(arr) (sizeof(arr) / sizeof(arr[0]))

struct request {
    char* raw;
    int method;
    char* route;
    size_t body_len;
    size_t headers_len;
    char* body;
};

char* mkbuf(const char* s) 
{
    size_t n = strlen(s);
    char* buf = malloc(n + 1);

    if (!buf) { 
        perror("malloc"); 
        exit(1); 
    }

    memcpy(buf, s, n + 1);
    return buf;
}

void test_start_line_basic() 
{
    TEST("Start line - basic GET") {
        char* raw = 
            "GET /hello HTTP/1.1\r\n";

        HTTPP_NEW_REQ(req, HTTPP_DEFAULT_HEADERS_ARR_CAP);
        
        int off = httpp_parse_start_line(raw, strlen(raw), &req);
        ASSERT(off > 0);
        ASSERT_EQ_INT(req.method, HTTPP_METHOD_GET);
        ASSERT(httpp_span_eq(&req.route, "/hello"));
        ASSERT(httpp_span_eq(&req.version, HTTPP_SUPPORTED_VERSION));
    }
}

void test_headers_and_body() 
{
    TEST("Headers and body parsing") {
        char* raw =
            "POST /upload HTTP/1.1\r\n"
            "Host: example.com\r\n"
            "X-Custom:   value with leading spaces\r\n"
            "\r\n"
            "BODY CONTENT";

        HTTPP_NEW_REQ(req, 10);

        int off = httpp_parse_request(raw, strlen(raw), &req);

        ASSERT(off + req.body.length == (int) strlen(raw));
        ASSERT_EQ_INT(req.method, HTTPP_METHOD_POST);
        ASSERT(httpp_span_eq(&req.route, "/upload"));
        ASSERT(httpp_span_eq(&req.version, HTTPP_SUPPORTED_VERSION));

        httpp_header_t* h = httpp_find_header(req, "host");
        ASSERT(h != NULL);
        ASSERT(httpp_span_eq(&h->value, "example.com"));

        httpp_header_t* h2 = httpp_find_header(req, "x-custom");
        ASSERT(h2 != NULL);
        ASSERT(httpp_span_eq(&h2->value, "value with leading spaces"));
        ASSERT(httpp_span_eq(&req.body, "BODY CONTENT"));
    }
}

void test_invalid_start_line() 
{
    TEST("Invalid start lines rejected") {
        char* raw1 = "BADMETHOD / HTTP/1.1\r\n";
        HTTPP_NEW_REQ(req1, 10);

        int r1 = httpp_parse_start_line(raw1, strlen(raw1), &req1);
        ASSERT(r1 == -1 || req1.method == HTTPP_METHOD_UNKNOWN);

        char* raw2 = "GET / HTTP/1.0\r\n";
        HTTPP_NEW_REQ(req2, 10);

        int r2 = httpp_parse_start_line(raw2, strlen(raw2), &req2);
        ASSERT(r2 == -1);
    }
}

void test_response_builder()
{
    TEST("Response builder and formatting") {
        HTTPP_NEW_RES(res, 10);
        char* body = "Hello!";
        size_t raw_len;

        res.code = 200;
        httpp_res_add_header(&res, "Content-Type", "text/plain");
        httpp_res_set_body(res, body, strlen(body));
        char* raw = httpp_res_to_raw(&res, &raw_len);
        
        ASSERT(raw != NULL);
        ASSERT(strstr(raw, "HTTP/1.1 200 OK\r\n") != NULL);
        ASSERT(strstr(raw, "Content-Type: text/plain\r\n") != NULL);
        ASSERT(strstr(raw, "\r\n\r\nHello!") != NULL);
        ASSERT(strlen(raw) == raw_len);

        free(raw);
        httpp_res_free_added(&res);
    }
}

void test_header_find() 
{
    TEST("Case-insensitive header search") {
        httpp_headers_arr_t hs = {0};
        httpp_header_t arr[5];

        hs.arr = arr;
        hs.capacity = 5;
        hs.length = 0;

        httpp_span_t n = {"X-Test", 6, false};
        httpp_span_t v = {"v", 1, false};

        httpp_headers_arr_append(&hs, (httpp_header_t){n, v});

        httpp_header_t* found = httpp_headers_arr_find(&hs, "x-test");

        ASSERT(found != NULL);
        ASSERT(strncasecmp(found->name.ptr, "X-Test", found->name.length) == 0);
        ASSERT(httpp_headers_arr_find(&hs, "missing") == NULL);
    }
}

void test_start_line_variations() 
{
    struct start_line {
        char* line; 
        char* route; 
        int method;
    };

    struct start_line table[] = {
        { "HEAD / HTTP/1.1\r\n", "/", HTTPP_METHOD_HEAD },
        { "PUT /a/b HTTP/1.1\r\n", "/a/b", HTTPP_METHOD_PUT },
        { "DELETE /res?id=5 HTTP/1.1\r\n", "/res?id=5", HTTPP_METHOD_DELETE },
    };

    TEST("Start line variations (table)") {
        for (size_t i = 0; i < ARR_LEN(table); i++) {
            char* raw = table[i].line;
            HTTPP_NEW_REQ(req, 0);

            int off = httpp_parse_start_line(raw, strlen(raw), &req);

            ASSERT(off > 0);
            ASSERT_EQ_INT(req.method, table[i].method);
            ASSERT(httpp_span_eq(&req.route, table[i].route));
        }
    }
}

void test_valid_req_variations() 
{
    struct request table[] = {
        {
            "GET / HTTP/1.1\r\n"
            "Host: example.com\r\n"
            "\r\n",
            HTTPP_METHOD_GET, "/", 0, 1, ""
        },

        {
            "POST /submit HTTP/1.1\r\n"
            "Host: example.com\r\n"
            "Content-Length: 13\r\n"
            "Content-Type: text/plain\r\n"
            "\r\n"
            "Line1\r\nLine2",
            HTTPP_METHOD_POST, "/submit", 12, 3, "Line1\r\nLine2"
        },

        {
            "GET /search?q=this+is+a+long+query&sort=desc&page=2 HTTP/1.1\r\n"
            "Host: search.example\r\n"
            "Accept: */*\r\n"
            "\r\n",
            HTTPP_METHOD_GET, "/search?q=this+is+a+long+query&sort=desc&page=2", 0, 2, ""
        },

        {
            "PUT /resource/123 HTTP/1.1\r\n"
            "Host: example\r\n"
            "X-Custom:    spaced value\r\n"
            "\r\n",
            HTTPP_METHOD_PUT, "/resource/123", 0, 2, ""
        },

        {
            "GET /cookies HTTP/1.1\r\n"
            "Host: ex\r\n"
            "Cookie: a=1\r\n"
            "Cookie: b=2\r\n"
            "\r\n",
            HTTPP_METHOD_GET, "/cookies", 0, 3, ""
        },

        {
            "HEAD /path/%7Euser/resource?id=10#frag HTTP/1.1\r\n"
            "Host: ex\r\n"
            "\r\n",
            HTTPP_METHOD_HEAD, "/path/%7Euser/resource?id=10#frag", 0, 1, ""
        },

        {
            "PATCH /a_b-c.123/~z HTTP/1.1\r\n"
            "Host: ex\r\n"
            "X-Flag: yes\r\n"
            "\r\n",
            HTTPP_METHOD_PATCH, "/a_b-c.123/~z", 0, 2, ""
        },

        {
            "GET /empty-header HTTP/1.1\r\n"
            "Host: ex\r\n"
            "X-Empty:\r\n"
            "\r\n",
            HTTPP_METHOD_GET, "/empty-header", 0, 2, ""
        },

        {
            "DELETE /res?id=5 HTTP/1.1\r\n"
            "Host: ex\r\n"
            "Connection: keep-alive\r\n"
            "Accept-Encoding: gzip\r\n"
            "User-Agent: test/1.0\r\n"
            "\r\n",
            HTTPP_METHOD_DELETE, "/res?id=5", 0, 4, ""
        },

        {
            "POST /empty-body HTTP/1.1\r\n"
            "Host: ex\r\n"
            "Content-Length: 0\r\n"
            "\r\n",
            HTTPP_METHOD_POST, "/empty-body", 0, 2, ""
        },
    };

    TEST("Start valid requests variations (table)") {
        for (size_t i = 0; i < ARR_LEN(table); i++) {
            char* raw = table[i].raw;
            HTTPP_NEW_REQ(req, 10);

            int off = httpp_parse_request(raw, strlen(raw), &req);
            
            ASSERT(off > 0);
            ASSERT_EQ_INT(req.method, table[i].method);
            ASSERT(httpp_span_eq(&req.route, table[i].route));
            ASSERT(req.headers.length == table[i].headers_len);
            ASSERT(req.body.length == table[i].body_len);
            ASSERT(httpp_span_eq(&req.body, table[i].body));
        }
    }
}

void test_invalid_req_variations() 
{
    struct request table[] = {
        {
            "GET/short HTTP/1.1\r\n"
            "Host: ex\r\n"
            "\r\n",
            HTTPP_METHOD_UNKNOWN, "/", 0, 1, ""
        },

        {
            "GET /no-crlf HTTP/1.1"
            "Host: ex\r\n"
            "\r\n",
            HTTPP_METHOD_GET, "/no-crlf", 0, 1, ""
        },

        {
            "GET /bad-header HTTP/1.1\r\n"
            "Host example\r\n"
            "\r\n",
            HTTPP_METHOD_GET, "/bad-header", 0, 1, ""
        },

        {
            "GET /leading-ws HTTP/1.1\r\n"
            " Host: ex\r\n"
            "\r\n",
            HTTPP_METHOD_GET, "/leading-ws", 0, 1, ""
        },

        {
            "GET /folded HTTP/1.1\r\n"
            "Host: ex\r\n"
            "X-Long: part1\r\n"
            " continuation\r\n"
            "\r\n",
            HTTPP_METHOD_GET, "/folded", 0, 3, ""
        },
    };

    TEST("Start invalid requests variations") {
        for (size_t i = 0; i < ARR_LEN(table); i++) {
            char* raw = table[i].raw;
            HTTPP_NEW_REQ(req, 10);

            int off = httpp_parse_request(raw, strlen(raw), &req);
            ASSERT(off == -1);
        }
    }
}

void test_totally_invalid() 
{
    TEST("Totally invalid calls") {
        HTTPP_NEW_REQ(req, 0);
        int ret;

        ret = httpp_parse_request(NULL, 10, &req);
        ASSERT(ret == -1);

        ret = httpp_parse_request("NULL", 0, &req);
        ASSERT(ret == 0);

        ret = httpp_parse_request("GET /short HTTP/1.1\r\n", 21, NULL);
        ASSERT(ret == -1);
    }
}

void test_binary_body() 
{
    TEST("Binary bodies") {
        const char hdr[] =
            "POST /binA HTTP/1.1\r\n"
            "Host: ex\r\n"
            "Content-Length: 6\r\n"
            "\r\n";

        const unsigned char body_bytes[6] = { 'A', 0x00, 'B', 0x00, 'C', 0x00 };
        size_t raw_len = (sizeof(hdr) - 1) + sizeof(body_bytes);
        char* raw = malloc(raw_len);

        memcpy(raw, hdr, sizeof(hdr) - 1);
        memcpy(raw + (sizeof(hdr) - 1), body_bytes, sizeof(body_bytes));

        HTTPP_NEW_REQ(req, 4);

        int ret = httpp_parse_request(raw, raw_len, &req);

        ASSERT(ret + req.body.length == raw_len);
        ASSERT(req.method == HTTPP_METHOD_POST);
        ASSERT(req.body.length == 6);
        ASSERT(memcmp(req.body.ptr, body_bytes, 6) == 0);

        free(raw);
    }
}

void test_edge() 
{
    TEST("Edge tests") {
        char* case1 = "GET / HTTP/1.1\r";
        char* case2 = "GET / HTTP/1.1\n";

        char* case3 = 
            "GET / HTTP/1.1\r\n"
            "X-Test:                                                                    \r\n"
            "\r\n";

        int ret;

        HTTPP_NEW_REQ(case1_parsed, 10);
        HTTPP_NEW_REQ(case2_parsed, 10);
        HTTPP_NEW_REQ(case3_parsed, 10);

        ret = httpp_parse_request(case1, strlen(case1), &case1_parsed);
        ASSERT(ret == -1);

        ret = httpp_parse_request(case2, strlen(case2), &case2_parsed);
        ASSERT(ret == -1);

        ret = httpp_parse_request(case3, strlen(case3), &case3_parsed);
        httpp_header_t* x_test = httpp_find_header(case3_parsed, "x-test");
        ASSERT(x_test->value.length == 0);
    }
}

int main() 
{
    test_start_line_basic();
    test_headers_and_body();
    test_invalid_start_line();
    test_response_builder();
    test_header_find();
    test_start_line_variations();
    test_valid_req_variations();
    test_invalid_req_variations();
    test_totally_invalid();
    test_binary_body();
    test_edge(); 

    printf("Tests run: %d, Failures: %d\n", tests_run, tests_failed);
    return tests_failed ? 1 : 0;
}