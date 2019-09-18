#include <stdio.h>
#include <sys/time.h>
#include <time.h>

unsigned long long get_unix_time()
{
    struct timeval tv;

    gettimeofday(&tv, NULL);

    unsigned long long millisecondsSinceEpoch = (unsigned long long)(tv.tv_sec) * 1000 + (unsigned long long)(tv.tv_usec) / 1000;

    return millisecondsSinceEpoch;
}

//NOTE: This is currently in milliseconds
void print_unix_time_formatted(unsigned long long time)
{
    printf("Unformatted Time: %llu\n", time);
}
