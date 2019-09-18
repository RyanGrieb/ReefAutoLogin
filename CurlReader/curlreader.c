#include "curlreader.h"
#include <stdio.h>

size_t write_to_curl_reader(void* ptr, size_t size, size_t nmemb, CurlReader* str)
{
    size_t new_len = str->size + size * nmemb;
    str->characters = realloc(str->characters, new_len + 1);
    if (str->characters == NULL) {
        fprintf(stderr, "Error: realloc() failed\n");
        exit(EXIT_FAILURE);
    }
    memcpy(str->characters + str->size, ptr, size * nmemb);
    str->characters[new_len] = '\0';
    str->size = new_len;

    return size * nmemb;
}

void create_curl_reader(CurlReader* str)
{
    str->size = 0;
    str->characters = malloc(str->size + 1);
    if (str->characters == NULL) {
        fprintf(stderr, "Error: malloc() failed\n");
        exit(EXIT_FAILURE);
    }
    str->characters[0] = '\0';
}

size_t hdf(char* b, size_t size, size_t nitems, void* userdata)
{
    size_t numbytes = size * nitems;
    printf("%.*s\n", numbytes, b);
    return numbytes;
}
