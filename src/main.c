#include "dataset.h"  // for DataSet, load_dataset, parse_dataset_from_str, free_dataset
#include "diff.h"     // for print_diff
#include "sv.h"       // for StringView
#include "ansi.h"     // for GREEN, RED, BOLD, RESET
#include "messages.h" // for tpv_get_random_praise, tpv_get_random_retry_message, tpv_get_random_goodbye_message
#include "timespan.h" // for TimeSpanSec, now
#include "cli-args.h" // for CliArgs, parse_cli_args, free_cli_args

#include <stddef.h>   // for size_t
#include <stdio.h>    // for printf, puts, fputs
#include <stdlib.h>   // for malloc, free
#include <string.h>   // for memcpy
#include <unistd.h>   // for sleep
#include <ctype.h>    // for ispunct, tolower

// used to calculate the average typing time
static size_t entered_words_count = 0;
static TimeSpanSec typing_times_sum = 0.0;
static TimeSpanSec typing_times_per_char_sum = 0.0;

static size_t correct_count = 0, incorrect_count = 0;

#define LINE_INPUT_BUF_INITIAL_CAPACITY 32
typedef struct Line {
    char* input_buf;
    size_t input_cap;
    size_t input_len;
    TimeSpanSec typing_time;
    TimeSpanSec typing_time_per_char;
} Line;

Line tpv_readline(const char* prompt, StringView expected_input) {
    Line line = {0};

    printf(BOLD "Type \"%.*s\"" RESET "\n", (int) expected_input.len, expected_input.data);
    fputs(prompt, stdout);

    TimeSpanSec start, end;
    start = now(); {
        char c;
        while ((c = getchar()) != '\n') {
            if (line.input_len == line.input_cap) {
                char* old_buf = line.input_buf;

                line.input_cap = line.input_cap == 0 ? LINE_INPUT_BUF_INITIAL_CAPACITY : line.input_cap * 2;
                line.input_buf = malloc(line.input_cap);
                if (line.input_buf == NULL) return line;

                memcpy(line.input_buf, old_buf, line.input_len * sizeof(char));
            }
            line.input_buf[line.input_len++] = c;
        }
    } end = now();

    line.typing_time = end - start;
    line.typing_time_per_char = line.typing_time / line.input_len;
    return line;
}

void free_line(Line* line) {
    free(line->input_buf);
}

void tpv_show_welcome() {
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

void tpv_show_goodbye() {
    if (entered_words_count == 0) {
        puts("If you're gonna launch the game just to immediately close it again then maybe - just maybe - don't launch it at all?");
        puts("um jk, then goodbye or something!");
    } else {
        TimeSpanSec avg_typing_time = typing_times_sum / entered_words_count;
        TimeSpanSec avg_typing_time_per_char = typing_times_per_char_sum / entered_words_count;

        // float correct_to_incorect_answers_ratio = 
        //          correct_count > incorrect_count
        //          ? (float) incorrect_count / correct_count
        //          : (float) correct_count / incorrect_count;
       
        float correct_to_incorect_answers_ratio =
            correct_count + incorrect_count > 0
            ? (float) correct_count / (correct_count + incorrect_count) * 100.0
            : 0.0f;

        const char* ratio_precent_color =
                correct_to_incorect_answers_ratio > 0.5
                ? GREEN
                : RED;

        puts(BOLD "Lets take a look at the statistics..." RESET);
        printf("    Average typing time:                " BOLD "%.1lf seconds" RESET "\n", avg_typing_time);
        printf("    Average typing time per character:  " BOLD "%.1lf seconds" RESET "\n", avg_typing_time_per_char);
        printf("    Correct to incorrect answers ratio: " BOLD GREEN "%zu" RESET BOLD "/" RESET BOLD RED "%zu" RESET BOLD " (" "%s%.0f%%" RESET ")" "\n",
                correct_count, incorrect_count, ratio_precent_color, correct_to_incorect_answers_ratio);

        puts("");
        puts(tpv_get_random_goodbye_message());
    }
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

StringView random_element_from_many_datasets(DataSet* datasets, size_t datasets_count) {
    if (datasets_count == 0) return SV_NULL;
    
    size_t all_datasets_elements_count = 0;
    for (DataSet* dataset = datasets; dataset < datasets + datasets_count; ++dataset) {
        all_datasets_elements_count += dataset->elements_count;
    }

    size_t index = rand() % all_datasets_elements_count;

    for (DataSet* dataset = datasets; dataset < datasets + datasets_count; ++dataset) {
        if (index < dataset->elements_count) {
            return dataset->elements[index];
        }

        index -= dataset->elements_count;
    }

    return SV_NULL;
}

StringView random_element(DataSet* real, size_t real_count,
                          GeneratorDataset* gen, size_t gen_count,
                          double gen_prob) {
    if (real_count == 0 && gen_count == 0)
        return SV_NULL;

    double r = (double) rand() / RAND_MAX;
    bool use_gen = (r < gen_prob);

    if (use_gen && gen_count > 0) {
        size_t i = rand() % gen_count;
        return gen[i].gen();
    }

    if (real_count > 0)
        return random_element_from_many_datasets(real, real_count);

    if (gen_count > 0)
        return gen[rand() % gen_count].gen();

    return SV_NULL;
}

int main(int argc, char** argv) {
    srand(time(NULL));

    CliArgs args = parse_cli_args(argc, argv);

    tpv_show_welcome();
    while (true) {
        StringView text = random_element(args.datasets, args.datasets_count, args.generator_datasets, args.generator_datasets_count, 0.3);

        Line line = tpv_readline(BOLD ">>> " RESET, text);
        StringView input = sv_from_data_and_len(line.input_buf, line.input_len);

        if (sv_eql(input, SV("/quit")) || sv_eql(input, SV("/exit"))) {
            break;
        }

        entered_words_count++;
        typing_times_sum += line.typing_time;
        typing_times_per_char_sum += line.typing_time_per_char;

        if (args.time_limit.set && line.typing_time > args.time_limit.value) {
            printf(BOLD RED "%s" RESET " Exceeded time limit (%.2lfs > %.2lfs)\n", tpv_get_random_retry_message(), line.typing_time, args.time_limit.value);
            incorrect_count++;
        } else if (args.time_per_char_limit.set && line.typing_time_per_char > args.time_per_char_limit.value) {
            printf(BOLD RED "%s" RESET " Exceeded time limit per character (%.2lfs > %.2lfs per char)\n", tpv_get_random_retry_message(), line.typing_time, args.time_limit.value);
            incorrect_count++;
        } else if (!tpv_input_eql(input, text, args.ignore_case.value, args.ignore_punctuations.value)) {
            printf(BOLD RED "%s" RESET " Look: ", tpv_get_random_retry_message());
            print_diff(text, input);
            incorrect_count++;
        } else {
            printf(BOLD GREEN "%s" RESET " Typing time: %.2f\n", tpv_get_random_praise(), line.typing_time);
            correct_count++;
        }

        free_line(&line);
    }
    tpv_show_goodbye();
    
    free_cli_args(&args);
}
