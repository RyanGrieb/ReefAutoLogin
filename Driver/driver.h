#include <curl/curl.h>
#include <curl/easy.h>
#include <json-c/json.h>

typedef struct ReefCourse {
    char* name;
    char* term;
    char* instructor_name;
    char* course_id;
    unsigned long long meeting_times[7]; //7: Max possible meeting times
    int meeting_time_amount;
    int start_date;
    int end_date;
    int remote_only;
} ReefCourse;

typedef struct ReefAccount {
    ReefCourse reef_courses[7]; //7: Max possible course amount
    char* name;
    char* student_id;
    char* email;
    char* password;
    char* institution_id;
    char* institution_name;
    char* sec_key;
    char* access_token;
    char* user_id;
    char* jti;
    char* trial_status;
    int token_expire_at; //Add currtime & json data
    int course_count;

} ReefAccount;

int validate_account(CURL* curl, ReefAccount* reef_account);
int validate_university(CURL* curl, ReefAccount* reef_account);
int login_account(CURL* curl, ReefAccount* reef_account);
int account_info(CURL* curl, ReefAccount* reef_account);
int course_info(CURL* curl, ReefAccount* reef_account);
int course_status(CURL* curl, ReefAccount* reef_account);
int join_course(CURL* curl, ReefAccount* reef_account, ReefCourse* reef_course);
size_t hdf(char* b, size_t size, size_t nitems, void* userdata);
struct curl_slist* default_post_headers(void);
void print_reef_account(ReefAccount reef_account);
void free_reef_account(ReefAccount* reef_account);
