#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

char* unix_time_formatted(long time, char* format_character)
{
    struct tm time_formatted;
    char* buf = (char*)malloc(4 * sizeof(char));
    time_formatted = *localtime(&time);
    strftime(buf, sizeof(buf), format_character, &time_formatted);

    return buf;
}

//NOTE: This is currently in milliseconds
void print_unix_time_formatted(unsigned long time)
{
    printf("Unformatted Time: %lu\n", time);
}
