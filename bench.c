// A benchmark adapted from https://github.com/h2o/picohttpparser

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

double now_seconds() 
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

int main() 
{
    char* raw = REQ;
    size_t raw_len = strlen(raw);
    int i;

    double t0 = now_seconds();
    for (i = 0; i < ITERATIONS; ++i) {
        httpp_req_t req;
        httpp_header_t arr[HTTPP_DEFAULT_HEADERS_ARR_CAP];

        httpp_init_req(&req, arr, HTTPP_DEFAULT_HEADERS_ARR_CAP);
        
        if (httpp_parse_request(raw, strlen(raw), &req) != 0) {
            fprintf(stderr, "parse failed at iter %d\n", i);
            return 1;
        }
    }
    double t1 = now_seconds();

    double elapsed = t1 - t0;
    double reqs_per_second = (double) ITERATIONS / elapsed;

    printf("Elapsed %f seconds.\n", elapsed);
    printf("Requests per second ≈ %.2f \n", reqs_per_second);

    return 0;
}
