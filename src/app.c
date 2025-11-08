#include "app.h"

#include "diff.h"     // for print_diff
#include "sv.h"       // for StringView
#include "ansi.h"     // for GREEN, RED, BOLD, RESET
#include "messages.h" // for tpv_get_random_praise, tpv_get_random_retry_message, tpv_get_random_goodbye_message
#include "timespan.h" // for TimeSpanSec, now
#include "cli-args.h" // for CliArgs, parse_cli_args, free_cli_args

#include "datasets-utils.h" // for random_element

#include <stddef.h>   // for size_t
#include <stdio.h>    // for printf, puts, fputs
#include <stdlib.h>   // for malloc, free
#include <string.h>   // for memcpy
#include <unistd.h>   // for sleep
#include <ctype.h>    // for ispunct, tolower

TpvApp tpv_init(int argc, char** argv) {
    TpvApp app = {0};
    app.args = parse_cli_args(argc, argv);
    if (cli_args_is_null(&app.args)) {
        exit(1);
    }

    return app;
}

void tpv_free(TpvApp* app) {
    free_cli_args(&app->args);
}

void tpv_run(TpvApp* app) {
    srand(time(NULL));


    app->running = true;

    tpv_show_welcome(app);
    while (app->running) {
        tpv_handle_input(app);
    }
    tpv_show_goodbye(app);
}

TpvLine tpv_read_line(const char* prompt, StringView expected_input) {
    TpvLine line = {0};

    printf(BOLD "Type \"%.*s\"" RESET "\n", (int) expected_input.len, expected_input.data);
    fputs(prompt, stdout);

    TimeSpanSec start, end;
    start = now(); {
        char c;
        while ((c = getchar()) != '\n') {
            if (line.input_len == line.input_cap) {
                size_t new_cap = line.input_cap == 0
                    ? LINE_INPUT_BUF_INITIAL_CAPACITY
                    : line.input_cap * 2;
            
                char* new_buf = realloc(line.input_buf, new_cap);
                if (!new_buf) {
                    free(line.input_buf);
                    return TPV_LINE_NULL;
                }
            
                line.input_buf = new_buf;
                line.input_cap = new_cap;
            }
            line.input_buf[line.input_len++] = c;
        }
    } end = now();

    line.typing_time = end - start;
    line.typing_time_per_char = line.typing_time / line.input_len;
    return line;
}

void tpv_free_line(TpvLine* line) {
    free(line->input_buf);
}

void tpv_show_welcome(TpvApp* app) {
    (void) app;

    puts(BOLD "Welcome to TPV!" RESET);
    puts(BOLD "TPV" RESET " is a game that involves typing words, sentences, or other texts without mistakes " BOLD "against the clock 🕰️!" RESET);
    puts("So what are you waiting for? " BOLD "Learn to type fast!" RESET);
    puts("");
    puts("You will be shown various texts. Your task is to transcribe them as quickly as possible."
            " If you want to leave, type " BOLD "/quit" RESET " or " BOLD "/exit" RESET "!");

    for (int i = 3; i > 0; --i) {
        printf(BOLD "%d..." RESET "\n", i);
        sleep(1);
    }
}

void tpv_show_goodbye(TpvApp* app) {
    if (app->entered_items_count == 0) {
        puts("If you're gonna launch the game just to immediately close it again then maybe - just maybe - don't launch it at all?");
        puts("um jk, then goodbye or something!");
    } else {
        puts(BOLD "Lets take a look at the statistics..." RESET);
        tpv_show_stats(app, "    ");

        puts("");
        puts(tpv_get_random_goodbye_message());
    }
}

void tpv_show_stats(TpvApp* app, const char* indent) {
    TimeSpanSec avg_typing_time = app->typing_times_sum / app->entered_items_count;
    TimeSpanSec avg_typing_time_per_char = app->typing_times_per_char_sum / app->entered_items_count;
    
    float correct_to_incorect_answers_ratio =
        app->correct_count + app->incorrect_count > 0
        ? (float) app->correct_count / (app->correct_count + app->incorrect_count) * 100.0
        : 0.0f;

    const char* ratio_percent_color =
        correct_to_incorect_answers_ratio > 50.0
        ? GREEN
        : RED;

    printf("%sAverage typing time:                " BOLD "%.1lf seconds" RESET "\n", indent, avg_typing_time);
    printf("%sAverage typing time per character:  " BOLD "%.1lf seconds" RESET "\n", indent, avg_typing_time_per_char);
    printf("%sCorrect to incorrect answers ratio: " BOLD GREEN "%zu" RESET BOLD "/" RESET BOLD RED "%zu" RESET BOLD " (" "%s%.0f%%" RESET ")" "\n",
                indent, app->correct_count, app->incorrect_count, ratio_percent_color, correct_to_incorect_answers_ratio);
}

bool tpv_input_eql(StringView input, StringView expected, bool ignore_case, bool ignore_punctuations) {
    if (!ignore_case && !ignore_punctuations)
        return sv_eql(input, expected);

    size_t i = 0, j = 0;

    while (i < input.len && j < expected.len) {
        char a = input.data[i];
        char b = expected.data[j];

        if (ignore_punctuations && ispunct((unsigned char) a)) {
            i++;
            continue;
        }
        if (ignore_punctuations && ispunct((unsigned char) b)) {
            j++;
            continue;
        }

        if (ignore_case) {
            a = (char) tolower((unsigned char) a);
            b = (char) tolower((unsigned char) b);
        }

        if (a != b)
            return false;

        i++;
        j++;
    }

    if (ignore_punctuations) {
        while (i < input.len && ispunct((unsigned char) input.data[i]))       i++;
        while (j < expected.len && ispunct((unsigned char) expected.data[j])) j++;
    }

    return i == input.len && j == expected.len;
}

void tpv_handle_input(TpvApp* app) {
    StringView text = random_element(app->args.datasets, app->args.datasets_count,
                                     app->args.generator_datasets, app->args.generator_datasets_count,
                                     0.3);

    while (true) {
        TpvLine line = tpv_read_line(BOLD ">>> " RESET, text);
        StringView input = sv_from_data_and_len(line.input_buf, line.input_len);

        if (sv_eql(input, SV("/quit")) || sv_eql(input, SV("/exit"))) {
            app->running = false;
            tpv_free_line(&line);
            return;
        }
        if (sv_eql(input, SV("/stats"))) {
            if (app->entered_items_count == 0) {
                puts(BOLD "No stats to display." RESET);
                tpv_free_line(&line);
                continue;
            }

            puts(BOLD "Stats:" RESET);
            tpv_show_stats(app, "    ");
            tpv_free_line(&line);
            continue;
        }
        if (sv_starts_with(input, SV("/"))) {
            printf(BOLD RED "Unknown command '%.*s'\n" RESET, (int) input.len, input.data);
            tpv_free_line(&line);
            continue;
        }

        app->entered_items_count++;
        app->typing_times_sum += line.typing_time;
        app->typing_times_per_char_sum += line.typing_time_per_char;

        bool ignore_case = !app->args.ignore_case.set || app->args.ignore_case.value;
        bool ignore_punctuations = app->args.ignore_punctuations.set && app->args.ignore_punctuations.value;

        bool is_correct = false;

        if (app->args.time_limit.set && line.typing_time > app->args.time_limit.value) {
            is_correct = false;
            printf(BOLD RED "%s" RESET " Exceeded time limit (%.2lfs > %.2lfs)\n",
                    tpv_get_random_retry_message(), line.typing_time, app->args.time_limit.value);
        } else if (app->args.time_per_char_limit.set && line.typing_time_per_char > app->args.time_per_char_limit.value) {
            is_correct = false;
            printf(BOLD RED "%s" RESET " Exceeded time limit per character (%.2lfs > %.2lfs per char)\n",
                    tpv_get_random_retry_message(), line.typing_time, app->args.time_limit.value);
        } else if (!tpv_input_eql(input, text, ignore_case, ignore_punctuations)) {
            is_correct = false;
            printf(BOLD RED "%s" RESET " Look: ", tpv_get_random_retry_message());
            print_diff(text, input);
        } else {
            is_correct = true;
            printf(BOLD GREEN "%s" RESET " Typing time: %.2f\n", tpv_get_random_praise(), line.typing_time);
        }

        tpv_free_line(&line);

        if (is_correct) {
            app->correct_count++;
            break;
        } else {
            app->incorrect_count++;
            if (app->args.retry.set && app->args.retry.value) {
                continue;
            } else {
                break;
            }
        }
    }
}
