// A benchmark adapted from https://github.com/fukamachi/fast-http

#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#define HTTPP_IMPLEMENTATION
#include "httpp.h"

#define REQ \
"GET /cookies HTTP/1.1\r\n" \
"Host: 127.0.0.1:8090\r\n" \
"Connection: keep-alive\r\n" \
"Cache-Control: max-age=0\r\n" \
"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n" \
"User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.17 (KHTML, like Gecko) Chrome/24.0.1312.56 Safari/537.17\r\n" \
"Accept-Encoding: gzip,deflate,sdch\r\n" \
"Accept-Language: en-US,en;q=0.8\r\n" \
"Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.3\r\n" \
"Cookie: name=wookie\r\n" \
"\r\n"

#define ITERATIONS 10000000

double now_seconds(void) 
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

int main(void) {
    char* raw = REQ;
    size_t raw_len = strlen(raw);
    int i;

    double t0 = now_seconds();
    for (i = 0; i < ITERATIONS; ++i) {
        httpp_req_t* req = httpp_parse_request(raw);
        if (!req) {
            fprintf(stderr, "parse failed at iter %d\n", i);
            return 1;
        }

        httpp_req_free(req);
    }
    double t1 = now_seconds();

    printf("Elapsed %f seconds.\n", t1 - t0);
    return 0;
}
