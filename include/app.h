#ifndef APP_H
#define APP_H

#include "timespan.h"
#include "cli-args.h"

#define LINE_INPUT_BUF_INITIAL_CAPACITY 32
typedef struct TpvLine {
    char* input_buf;
    size_t input_cap;
    size_t input_len;
    TimeSpanSec typing_time;
    TimeSpanSec typing_time_per_char;
} TpvLine;

TpvLine tpv_read_line(const char* prompt, StringView expected_input);
void tpv_free_line(TpvLine* line);

typedef struct TpvApp {
    CliArgs args;

    size_t entered_words_count;
    TimeSpanSec typing_times_sum;
    TimeSpanSec typing_times_per_char_sum;

    size_t incorrect_count, correct_count;

    bool running;
} TpvApp;

TpvApp tpv_init(int argc, char** argv);
void tpv_free(TpvApp* app);
void tpv_run(TpvApp* app);

void tpv_show_welcome(TpvApp* app);
void tpv_show_goodbye(TpvApp* app);

void tpv_show_stats(TpvApp* app, const char* ident);
void tpv_handle_input(TpvApp* app);

#endif // APP_H

