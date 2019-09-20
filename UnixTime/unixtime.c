#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

void unix_time_formatted(char** str, long time, char* format_character)
{
    struct tm time_formatted;
    *str = (char*)malloc(4 * sizeof(char));
    time_formatted = *localtime(&time);
    strftime(*str, sizeof(*str), format_character, &time_formatted);
}

int same_unix_time_formatted(long time1, long time2, char* format_character)
{
    char* formatted_time1;
    unix_time_formatted(&formatted_time1, time1, format_character);
    char* formatted_time2;
    unix_time_formatted(&formatted_time2, time2, format_character);

    int same_format = strcmp(formatted_time1, formatted_time2) == 0;

    free(formatted_time1);
    free(formatted_time2);

    return same_format;
}

//NOTE: This is currently in milliseconds
void print_unix_time_formatted(unsigned long time)
{
    printf("Unformatted Time: %lu\n", time);
}
