#include <stdlib.h>
#include <string.h>

typedef struct CurlReader {
    char* characters;
    size_t size;
} CurlReader;

size_t write_to_curl_reader(void* ptr, size_t size, size_t nmemb, CurlReader* str);
void create_curl_reader(CurlReader* s);
#pragma GCC diagnostic ignored "-Wformat"
size_t hdf(char* b, size_t size, size_t nitems, void* userdata);
