#include "driver.h"
#include "../CurlReader/curlreader.h"
#include "../UnixTime/unixtime.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
//To compile: gcc -g -o ReefHacked driver.c -lcurl -ljson-c

int main()
{
    char username[256];
    char password[256];
    printf("Username: ");
    scanf("%s", username);

    printf("Password (Text is hidden): ");
    struct termios term, term_orig;
    tcgetattr(STDIN_FILENO, &term);
    term_orig = term;
    term.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
    scanf("%s", password);
    /* Remember to set back, or your commands won't echo! */
    tcsetattr(STDIN_FILENO, TCSANOW, &term_orig);

    //TRUE = 1
    //FALSE = 0
    CURL* curl;
    //CURLcode res;

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    if (!curl)
        return 1;

    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Ubuntu Chromium/76.0.3809.100 Chrome/76.0.3809.100 Safari/537.36");

    ReefAccount reef_account;
    //TODO: Ask proffesor how to properly copy the string onto a struct like this.
    reef_account.email = (char*)malloc(strlen(username) * sizeof(char) + 1);
    strcpy(reef_account.email, username);
    reef_account.password = (char*)malloc(strlen(password) * sizeof(char) + 1);
    strcpy(reef_account.password, password);

    printf("\nLogging in...\n");
    //TEMP: I don't reall know what to do with curl var so its all over the place currently.
    reef_account.curl = curl;

    //TODO: We don't really need to use res. Either just create one for each method call or not all all.
    if (validate_account(curl, &reef_account)) {
        printf("Error: Reef account doesn't exist\n");
        return 1;
    }

    if (validate_university(curl, &reef_account)) {
        printf("Error: Unable to validate university\n");
        return 1;
    }

    if (login_account(curl, &reef_account)) {
        printf("Error: Incorrect password\n");
        return 1;
    }

    //Note: we validate again to mimic the browser.
    /*if (!validate_university(curl, res, &reef_account)) {
            printf("Error: Unable to validate university\n");
            return 1;
        }*/

    if (account_info(curl, &reef_account)) {
        printf("Error: Unable to get account info\n");
        return 1;
    }

    if (course_info(curl, &reef_account)) {
        printf("Error: Unable to get course info\n");
        return 1;
    }

    /* Scan for joining class packets */
    //curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);

    print_reef_account(reef_account);

    printf("Waiting for class to start...\n");
    printf("P.S. Type help to see available commands!\n");

    pthread_t tid;
    pthread_create(&tid, NULL, class_check_thread, (void*)&reef_account);

    char command[256] = "";
    while (strcmp(command, "stop") != 0) {
        scanf("%s", command);
        if (strcmp(command, "help") == 0)
            printf("heeeeeeeeeelp\n");
        else if (strcmp(command, "stop") == 0) {
            reef_account.check_for_class = 0;
            pthread_cancel(tid);
        }
    }

    /* always cleanup */
    pthread_join(tid, NULL);
    free_reef_account(&reef_account);
    curl_easy_cleanup(curl);
    curl_global_cleanup();

    return 0;
}

int validate_account(CURL* curl, ReefAccount* reef_account)
{
    char email_json[256];
    sprintf(email_json, "{\"email\": \"%s\"}", reef_account->email);

    /* Specifiy relevant POST headers */
    struct curl_slist* post_header = default_post_headers();
    post_header = curl_slist_append(post_header, "Content-Type: application/json;charset=UTF-8");
    post_header = curl_slist_append(post_header, "Accept: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, post_header);

    /* Now specify the POST data */
    curl_easy_setopt(curl, CURLOPT_URL, "https://api.reef-education.com/trogon/v1/federation/account/association/check");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, email_json);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, -1L);

    CurlReader s;
    create_curl_reader(&s);

    //curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, readHeaderResposne);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_curl_reader);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

    /* Perform the request, res will get the return code */
    curl_easy_perform(curl);

    struct json_object* parsed_json = json_tokener_parse(s.characters);
    struct json_object* bool_account_exists;
    json_object_object_get_ex(parsed_json, "accountExists", &bool_account_exists);
    int account_exists = json_object_get_int(bool_account_exists);

    curl_slist_free_all(post_header);
    json_object_put(parsed_json);

    return !account_exists;
}

int validate_university(CURL* curl, ReefAccount* reef_account)
{
    char email_json[256];
    sprintf(email_json, "{\"email\": \"%s\"}", reef_account->email);

    /* Specifiy relevant POST headers */
    struct curl_slist* post_header = default_post_headers();
    post_header = curl_slist_append(post_header, "Content-Type: application/json;charset=UTF-8");
    post_header = curl_slist_append(post_header, "Accept: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, post_header);

    /* Now specify the POST data */
    curl_easy_setopt(curl, CURLOPT_URL, "https://api.reef-education.com/trogon/v1/federation/account/validate");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, email_json);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, -1L);

    CurlReader s;
    create_curl_reader(&s);

    //curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, hdf);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_curl_reader);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

    curl_easy_perform(curl);

    /* If our data is already initalized */
    //if (reef_account->institution_id != NULL || reef_account->institution_name != NULL)
    //    return 1;

    struct json_object* parsed_json = json_tokener_parse(s.characters);
    struct json_object* str_institution_id;
    struct json_object* str_institution_name;
    json_object_object_get_ex(parsed_json, "institutionId", &str_institution_id);
    json_object_object_get_ex(parsed_json, "institutionName", &str_institution_name);
    if (str_institution_id == NULL || str_institution_name == NULL)
        return 1;

    /* Initalize our reef account institution id & name */
    reef_account->institution_id = (char*)malloc(strlen(json_object_get_string(str_institution_id)) * sizeof(char) + 1);
    strcpy(reef_account->institution_id, json_object_get_string(str_institution_id));

    reef_account->institution_name = (char*)malloc(strlen(json_object_get_string(str_institution_name)) * sizeof(char) + 1);
    strcpy(reef_account->institution_name, json_object_get_string(str_institution_name));

    curl_slist_free_all(post_header);
    json_object_put(parsed_json);

    return 0;
}

//TODO: Clean up unessicary curl_easy_setopt methods.
int login_account(CURL* curl, ReefAccount* reef_account)
{

    char login_json[256];
    sprintf(login_json, "{\"email\": \"%s\",\"password\": \"%s\"}", reef_account->email, reef_account->password);

    /* Specifiy relevant POST headers */
    struct curl_slist* post_header = default_post_headers();
    post_header = curl_slist_append(post_header, "Content-Type: application/vnd.reef.login-proxy-request-v1+json");
    post_header = curl_slist_append(post_header, "Accept: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, post_header);

    /* Now specify the POST data */
    curl_easy_setopt(curl, CURLOPT_URL, "https://api-gateway.reef-education.com/authproxy/login");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, login_json);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, -1L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CurlReader curl_reader;
    create_curl_reader(&curl_reader);

    //curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, hdf);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_curl_reader);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &curl_reader);

    /* Perform the request, res will get the return code */
    curl_easy_perform(curl);

    //If we didn't get a response back, assume the password is wrong.
    if (curl_reader.size <= 0)
        return 1;

    struct json_object* parsed_json = json_tokener_parse(curl_reader.characters);
    struct json_object* str_user_id;
    struct json_object* str_access_token;
    struct json_object* int_token_expire_at;
    json_object_object_get_ex(parsed_json, "userId", &str_user_id);
    json_object_object_get_ex(parsed_json, "access_token", &str_access_token);
    json_object_object_get_ex(parsed_json, "expires_in", &int_token_expire_at);

    /* Initalize our json parameters into our struct */
    reef_account->user_id = (char*)malloc(strlen(json_object_get_string(str_user_id)) * sizeof(char) + 1);
    strcpy(reef_account->user_id, json_object_get_string(str_user_id));

    reef_account->access_token = (char*)malloc(strlen(json_object_get_string(str_access_token)) * sizeof(char) + 1);
    strcpy(reef_account->access_token, json_object_get_string(str_access_token));

    //TODO: Properly set the token expiration.
    reef_account->token_expire_at = -1;

    curl_slist_free_all(post_header);
    json_object_put(parsed_json);

    return 0;
}

int account_info(CURL* curl, ReefAccount* reef_account)
{
    /* Specifiy relevant POST headers */
    struct curl_slist* post_header = default_post_headers();
    char* auth_token = (char*)malloc((strlen("Authorization: Bearer ") + strlen(reef_account->access_token)) * sizeof(char) + 1);
    sprintf(auth_token, "%s%s", "Authorization: Bearer ", reef_account->access_token);
    post_header = curl_slist_append(post_header, auth_token);
    post_header = curl_slist_append(post_header, "Accept: application/json");
    post_header = curl_slist_append(post_header, "Cache-Control: no-cache");
    post_header = curl_slist_append(post_header, "Expires: Mon, 26 Jul 1997 05:00:00 GMT");
    post_header = curl_slist_append(post_header, "If-Modified-Since: Mon, 26 Jul 1997 05:00:00 GMT");
    post_header = curl_slist_append(post_header, "Pragma: no-cache");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, post_header);

    /* Now specify the POST data */
    curl_easy_setopt(curl, CURLOPT_URL, "https://api.reef-education.com/trogon/v3/profile");
    //curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CurlReader curl_reader;
    create_curl_reader(&curl_reader);

    /* Turns this into a GET request */
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    // curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, hdf);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_curl_reader);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &curl_reader);

    /* Perform the request, res will get the return code */
    curl_easy_perform(curl);

    //If we didn't get a response back, assume the password is wrong.
    if (curl_reader.size <= 0)
        return 1;

    struct json_object* parsed_json = json_tokener_parse(curl_reader.characters);
    struct json_object* str_first_name;
    struct json_object* str_last_name;
    struct json_object* str_student_id; //abc123
    struct json_object* str_sec_key;
    struct json_object* str_trial_status;
    struct json_object* int_course_count;
    json_object_object_get_ex(parsed_json, "firstName", &str_first_name);
    json_object_object_get_ex(parsed_json, "lastName", &str_last_name);
    json_object_object_get_ex(parsed_json, "studentId", &str_student_id);
    json_object_object_get_ex(parsed_json, "seckey", &str_sec_key);
    json_object_object_get_ex(parsed_json, "status", &str_trial_status);
    json_object_object_get_ex(parsed_json, "courseCount", &int_course_count);

    reef_account->name = (char*)malloc((strlen(json_object_get_string(str_first_name)) + strlen(json_object_get_string(str_last_name))) * sizeof(char) + 2);
    sprintf(reef_account->name, "%s %s", json_object_get_string(str_first_name), json_object_get_string(str_last_name));

    reef_account->student_id = (char*)malloc(strlen(json_object_get_string(str_student_id)) * sizeof(char) + 1);
    strcpy(reef_account->student_id, json_object_get_string(str_student_id));

    //TODO: Use sec_key & trial status.
    reef_account->course_count = json_object_get_int(int_course_count);

    curl_slist_free_all(post_header);
    json_object_put(parsed_json);
    free(auth_token);

    return 0;
}

int course_info(CURL* curl, ReefAccount* reef_account)
{

    /* Specifiy relevant POST headers */
    struct curl_slist* post_header = default_post_headers();
    char* auth_token = (char*)malloc((strlen("Authorization: Bearer ") + strlen(reef_account->access_token)) * sizeof(char) + 1);
    sprintf(auth_token, "%s%s", "Authorization: Bearer ", reef_account->access_token);
    post_header = curl_slist_append(post_header, auth_token);
    post_header = curl_slist_append(post_header, "Accept: application/vnd.reef.student-course-list-response-v1+json");
    post_header = curl_slist_append(post_header, "Cache-Control: no-cache");
    post_header = curl_slist_append(post_header, "Expires: Mon, 26 Jul 1997 05:00:00 GMT");
    post_header = curl_slist_append(post_header, "If-Modified-Since: Mon, 26 Jul 1997 05:00:00 GMT");
    post_header = curl_slist_append(post_header, "Pragma: no-cache");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, post_header);

    /* Now specify the POST data */
    curl_easy_setopt(curl, CURLOPT_URL, "https://api-gateway.reef-education.com/course/student/list");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CurlReader curl_reader;
    create_curl_reader(&curl_reader);

    /* Turns this into a GET request */
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    //curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, hdf);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_curl_reader);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &curl_reader);

    /* Perform the request, res will get the return code */
    curl_easy_perform(curl);

    struct json_object* parsed_json = json_tokener_parse(curl_reader.characters);
    struct json_object* arr_course_list;
    json_object_object_get_ex(parsed_json, "courseList", &arr_course_list);

    int i;
    for (i = 0; i < reef_account->course_count; i++) {
        struct json_object* current_course = json_object_array_get_idx(arr_course_list, i);

        struct json_object* str_course_id;
        struct json_object* str_course_name;
        struct json_object* arr_meeting_times;

        json_object_object_get_ex(current_course, "id", &str_course_id);
        json_object_object_get_ex(current_course, "name", &str_course_name);
        json_object_object_get_ex(current_course, "meetingTimes", &arr_meeting_times);

        reef_account->reef_courses[i].course_id = (char*)malloc(strlen(json_object_get_string(str_course_id)) * sizeof(char) + 1);
        strcpy(reef_account->reef_courses[i].course_id, json_object_get_string(str_course_id));

        reef_account->reef_courses[i].name = (char*)malloc(strlen(json_object_get_string(str_course_name)) * sizeof(char) + 1);
        strcpy(reef_account->reef_courses[i].name, json_object_get_string(str_course_name));

        int j;
        int meeting_time_amount = 0;
        for (j = 0; j < json_object_array_length(arr_meeting_times); j++) {
            struct json_object* int_meeting_time = json_object_array_get_idx(arr_meeting_times, j);

            reef_account->reef_courses[i].meeting_times[j] = (unsigned long)((unsigned long long)strtoll(json_object_get_string(int_meeting_time), NULL, 0) / 1000);
            meeting_time_amount++;
        }

        reef_account->reef_courses[i].meeting_time_amount = meeting_time_amount;
        reef_account->reef_courses[i].joined = 0;
    }

    curl_slist_free_all(post_header);
    json_object_put(parsed_json);
    free(auth_token);

    return 0;
}

int join_course(CURL* curl, ReefAccount* reef_account, ReefCourse reef_course)
{

    char geo_location_json[256] = "{\"geo\":{\"accuracy\":39,\"lat\":29.584474999999998,\"lon\":-98.61909589999999},\"publicIP\":null,\"auto\":false,\"id\":\"fbfa1fc2-9cf9-4096-a788-a2aae933f19c\"};";

    /* Specifiy relevant POST headers */
    struct curl_slist* post_header = default_post_headers();
    char* auth_token = (char*)malloc((strlen("Authorization: Bearer ") + strlen(reef_account->access_token)) * sizeof(char) + 1);
    sprintf(auth_token, "%s%s", "Authorization: Bearer ", reef_account->access_token);
    post_header = curl_slist_append(post_header, auth_token);
    post_header = curl_slist_append(post_header, "Accept: application/json");
    post_header = curl_slist_append(post_header, "Content-Type: application/json;charset=UTF-8");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, post_header);

    /* Now specify the POST data */
    char class_url[256];
    sprintf(class_url, "https://api.reef-education.com/trogon/v2/course/attendance/join/%s", reef_course.course_id);
    curl_easy_setopt(curl, CURLOPT_URL, class_url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, geo_location_json);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, -1L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CurlReader curl_reader;
    create_curl_reader(&curl_reader);

    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, hdf);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_curl_reader);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &curl_reader);

    /* Perform the request, res will get the return code */
    curl_easy_perform(curl);

    printf("OUT: %s\n", curl_reader.characters);

    struct json_object* parsed_json = json_tokener_parse(curl_reader.characters);
    struct json_object* str_err_check;
    json_object_object_get_ex(parsed_json, "error", &str_err_check);

    if (str_err_check)
        return 1;

    reef_course.joined = 1;

    curl_slist_free_all(post_header);
    json_object_put(parsed_json);
    free(auth_token);

    return 0;
}

struct curl_slist* default_post_headers()
{
    struct curl_slist* post_header = NULL;
    post_header = curl_slist_append(post_header, "Client-Tag: REEF/STUDENT/5.0.5/WEB///");
    post_header = curl_slist_append(post_header, "Reef-Auth-Type: oauth");
    post_header = curl_slist_append(post_header, "Referer: https://app.reef-education.com/");
    post_header = curl_slist_append(post_header, "Sec-Fetch-Mode: cors");
    post_header = curl_slist_append(post_header, "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Ubuntu Chromium/76.0.3809.100 Chrome/76.0.3809.100 Safari/537.36");

    return post_header;
}

void print_reef_account(ReefAccount reef_account)
{
    printf("\n------ Reef Account ------\n\n");
    printf("Name: %s\n", reef_account.name);
    printf("Institution: %s\n", reef_account.institution_name);
    printf("Student ID: %s\n", reef_account.student_id);
    printf("Course Amount: %d\n", reef_account.course_count);

    printf("\n---- Course Info ----\n\n");
    int i;
    for (i = 0; i < reef_account.course_count; i++) {
        printf("%s\n", reef_account.reef_courses[i].name);
        int j;
        for (j = 0; j < reef_account.reef_courses[i].meeting_time_amount; j++)
            print_unix_time_formatted(reef_account.reef_courses[i].meeting_times[j]);
    }
    printf("---- Course Info ----\n\n");

    printf("------ Reef Account ------\n");

    printf("\n");
}

void free_reef_account(ReefAccount* reef_account)
{
    int i;
    for (i = 0; i < reef_account->course_count; i++) {
        free(reef_account->reef_courses[i].name);
        free(reef_account->reef_courses[i].course_id);
    }
    free(reef_account->name);
    free(reef_account->student_id);
    free(reef_account->email);
    free(reef_account->password);
    free(reef_account->institution_id);
    free(reef_account->institution_name);
    free(reef_account->access_token);
    free(reef_account->user_id);
    //free(reef_account->jti);
    //free(reef_account->trial_status);
}

void* class_check_thread(void* account)
{
    ReefAccount* reef_account = (ReefAccount*)account;
    reef_account->check_for_class = 1;

    while (reef_account->check_for_class) {
        printf("Checking...\n");

        //Utilize %H,%M,%P
        long current_time;
        time(&current_time);

        int i;
        for (i = 0; i < reef_account->course_count; i++) {
            ReefCourse reef_course = reef_account->reef_courses[i];

            int j;
            for (j = 0; j < reef_course.meeting_time_amount; j++) {
                long meeting_time = (long)reef_course.meeting_times[j];

                //Check if the day of week is the same

                if (!same_unix_time_formatted(current_time, meeting_time, "%a"))
                    continue;

                //Check if AM/PM is the same
                if (!same_unix_time_formatted(current_time, meeting_time, "%P")) {

                    //At the end of the day, we can safley say our class is over.
                    //TODO: Maybe this should go somewhere else? not sure.
                    if (reef_course.joined)
                        reef_course.joined = 0;
                    continue;
                }

                //TODO: Instead of checking agiant the hours & mins, just substract the two tims & check if were between -5mins < time < 5mins

                //Check if the hour is the same
                if (!same_unix_time_formatted(current_time, meeting_time, "%H"))
                    continue;

                char* current_time_min_format;
                unix_time_formatted(&current_time_min_format, current_time, "%M");
                char* meeting_time_min_format;
                unix_time_formatted(&meeting_time_min_format, meeting_time, "%M");

                int current_mins = atoi(current_time_min_format);
                int meeting_mins = atoi(meeting_time_min_format);

                free(current_time_min_format);
                free(meeting_time_min_format);

                //If were in beetween the start of class & 5 miniute buffer period, AND we havent joined yet, attempt to sing in.
                if (current_mins - meeting_mins > 0 && current_mins - meeting_mins <= 5 && !reef_course.joined) {
                    printf("Joining class %s\n", reef_course.name);
                    if (join_course(reef_account->curl, reef_account, reef_course)) {
                        printf("Error: Unable to join class");
                        continue;
                    }

                    printf("Joined class %s\n", reef_course.name);
                }
            }
        }

        sleep(10);
    }

    return 0;
}
