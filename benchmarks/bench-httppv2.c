// A benchmark adapted from https://github.com/h2o/picohttpparser

#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#define HTTPP_IMPLEMENTATION
#include "httppv2.h"

#define REQ                                                                                                                        \
    "GET /wp-content/uploads/2010/03/hello-kitty-darth-vader-pink.jpg HTTP/1.1\r\n"                                                \
    "Host: www.kittyhell.com\r\n"                                                                                                  \
    "User-Agent: Mozilla/5.0 (Macintosh; U; Intel Mac OS X 10.6; ja-JP-mac; rv:1.9.2.3) Gecko/20100401 Firefox/3.6.3 "             \
    "Pathtraq/0.9\r\n"                                                                                                             \
    "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"                                                  \
    "Accept-Language: ja,en-us;q=0.7,en;q=0.3\r\n"                                                                                 \
    "Accept-Encoding: gzip,deflate\r\n"                                                                                            \
    "Accept-Charset: Shift_JIS,utf-8;q=0.7,*;q=0.7\r\n"                                                                            \
    "Keep-Alive: 115\r\n"                                                                                                          \
    "Connection: keep-alive\r\n"                                                                                                   \
    "Cookie: wp_ozh_wsa_visits=2; wp_ozh_wsa_visit_lasttime=xxxxxxxxxx; "                                                          \
    "__utma=xxxxxxxxx.xxxxxxxxxx.xxxxxxxxxx.xxxxxxxxxx.xxxxxxxxxx.x; "                                                             \
    "__utmz=xxxxxxxxx.xxxxxxxxxx.x.x.utmccn=(referral)|utmcsr=reader.livedoor.com|utmcct=/reader/|utmcmd=referral\r\n"             \
    "\r\n"

double benchmark() 
{
    char* raw = REQ;
    size_t raw_len = strlen(raw);
    int i;
    double start, end;

    start = (double)clock()/CLOCKS_PER_SEC;
    for (i = 0; i < ITERATIONS; ++i) {
        httpp_req_t req;
        httpp_header_t arr[HTTPP_DEFAULT_HEADERS_ARR_CAP];

        httpp_init_req(&req, arr, HTTPP_DEFAULT_HEADERS_ARR_CAP);
        
        assert(httpp_parse_request(raw, raw_len, &req) == 0);
    }
    end = (double)clock()/CLOCKS_PER_SEC;
    return end - start;
}

int main()
{
    double total = 0.0;

    for (int i = 0; i < RUNS; i++) {
        double elapsed = benchmark();
        total += elapsed;

        printf("Run %i:\n", i);
        printf(" Elapsed time: %f\n", elapsed);
        printf(" Requests per second ≈ %.2f\n", (double) ITERATIONS / elapsed);
    }

    printf("\nAverage elapsed time %f\n", total / RUNS);
    printf("Average Requests per second ≈ %.2f\n\n", (double) ITERATIONS / (total / RUNS));
    
    return 0;
}