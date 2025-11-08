#include "cli-args.h"

#include "ansi.h"

#include "dataset.h"
#include "builtin-datasets.h"
#include "generator-dataset.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

typedef struct {
    StringView name;
    const char* data;
    unsigned int size;
} BuiltinDatasetEntry;

const BuiltinDatasetEntry* list_builtin_datasets(size_t* out_builtin_datasets_count) {
    static BuiltinDatasetEntry builtin_datasets[3];
    builtin_datasets[0] = (BuiltinDatasetEntry) { .name = SV("english-words"),     .data = embed_english_words_data,     .size = embed_english_words_size        };
    builtin_datasets[1] = (BuiltinDatasetEntry) { .name = SV("english-sentences"), .data = embed_english_sentences_data, .size = embed_english_sentences_size    };
    builtin_datasets[2] = (BuiltinDatasetEntry) { .name = SV("code-snippets"),     .data = embed_code_snippets_data,     .size = embed_code_snippets_size };
    const size_t builtin_datasets_count = sizeof(builtin_datasets) / sizeof(builtin_datasets[0]);

    if (out_builtin_datasets_count)
        *out_builtin_datasets_count = builtin_datasets_count;
    return builtin_datasets;
}

typedef struct {
    StringView name;
    StringView* aliases;
    GeneratorDataset gd;
} BuiltinGeneratorDatasetEntry;

const BuiltinGeneratorDatasetEntry* list_builtin_generator_datasets(size_t* out_builtin_generator_datasets_count) {
    static BuiltinGeneratorDatasetEntry builtin_generator_datasets[3];
    builtin_generator_datasets[0] = (BuiltinGeneratorDatasetEntry) {
        .name = SV("random-alpha-numeric-strings"), .aliases = (StringView[]) { SV("random-strings"), SV_NULL }, .gd = random_alpha_numeric_strings_generator_dataset,
    };
    builtin_generator_datasets[1] = (BuiltinGeneratorDatasetEntry) { .name = SV("random-alpha-strings"), .aliases = NULL, .gd = random_alpha_strings_generator_dataset };
    builtin_generator_datasets[2] = (BuiltinGeneratorDatasetEntry) { .name = SV("random-numbers"), .aliases = NULL, .gd = random_numbers_generator_dataset };

    const size_t builtin_generator_datasets_count = sizeof(builtin_generator_datasets) / sizeof(builtin_generator_datasets[0]);

    if (out_builtin_generator_datasets_count != NULL)
        *out_builtin_generator_datasets_count = builtin_generator_datasets_count;
    return builtin_generator_datasets;
}

DataSet load_builtin_dataset(StringView name) {
    size_t builtin_datasets_count;
    const BuiltinDatasetEntry* builtin_datasets = list_builtin_datasets(&builtin_datasets_count);

    for (size_t i = 0; i < builtin_datasets_count; ++i) {
        if (sv_eql(name, builtin_datasets[i].name)) {
            return parse_dataset_from_str(sv_from_data_and_len(builtin_datasets[i].data, builtin_datasets[i].size));
        }
    }

    return DATASET_NULL;
}

GeneratorDataset load_builtin_generator_dataset(StringView name) {
    size_t builtin_generator_datasets_count;
    const BuiltinGeneratorDatasetEntry* builtin_generator_datasets = list_builtin_generator_datasets(&builtin_generator_datasets_count);

    for (size_t i = 0; i < builtin_generator_datasets_count; ++i) {
        if (sv_eql(builtin_generator_datasets[i].name, name)) {
            return builtin_generator_datasets[i].gd;
        }

        if (builtin_generator_datasets[i].aliases == NULL) continue;
        for (StringView* alias = builtin_generator_datasets[i].aliases; !sv_is_null(*alias); alias++) {
            if (sv_eql(*alias, name)) {
                return builtin_generator_datasets[i].gd;
            }
        }
    }

    return GENERATOR_DATASET_NULL;
}

bool cli_error(const char* msg) {
    puts(msg);
    return false;
}

#if defined(__GNUC__) || defined(__clang__)
#   define PRINTF_FORMAT(format_index, first_to_check) __attribute__((format(printf, format_index, first_to_check))) 
#else
#   define PRINTF_FORMAT(format_index, first_to_check)
#endif

PRINTF_FORMAT(1, 2)
bool cli_errorf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    vprintf(fmt, args);

    va_end(args);
    putchar('\n');
    return false;
}

bool cli_show_help() {
    puts(BOLD "Usage: tpv [options] [datasets...]" RESET);
    puts("");
    puts(BOLD "Options:" RESET);
    puts("  -h, --help                                      Show this help message and exit.");
    puts("  -i, --ignore-case                               Ignore case when comparing characters.");
    puts("  -p, --ignore-punctuations                       Ignore punctuation characters during typing.");
    puts("  -r, --retry                                     Enable retry after failure.");
    puts("");
    puts("  --[no-]game-over-on-mistake                     End the game immediately after a mistake.");
    puts("  --[no-]game-over-on-exceed-time-limit           End the game if the total time limit is exceeded.");
    puts("  --[no-]game-over-on-exceed-time-per-char-limit  End the game if per-character time limit is exceeded.");
    puts("");
    puts("  --time-limit=<duration>                         Set a total time limit for the session.");
    puts("                                                  Examples: 1m, 30s, 2h10m.");
    puts("  --time-per-char-limit=<duration>                Set a per-character typing time limit.");
    puts("                                                  Examples: 500ms, 2s.");
    puts("");
    puts(BOLD "Datasets:" RESET);
    puts("  Specify one or more datasets to use for typing practice.");
    puts("  You can pass a file path or a built-in dataset name prefixed with '@'.");
    puts("  Example: typer @english-words @code-snippets");
    puts("");
    puts("  To list all built-in datasets, run: typer @unknown");
    puts("");
    puts(BOLD "Examples:" RESET);
    puts("  tpv --ignore-case @english-words");
    puts("  tpv --time-limit=2m --retry custom_dataset.txt");
    puts("  tpv -ip @code-snippets");
    return false;
}

bool set_cli_switch(StringView name, CliSwitch* cswitch, bool value) {
    if (cswitch->set) {
        printf("%.*s: Alredy set to %s", (int) name.len, name.data, cswitch->value ? "true" : "false");
        return false;
    }

    cswitch->set = true;
    cswitch->value = value;
    return true;
}

bool cli_args_add_dataset(CliArgs* args, DataSet dataset) {
    if (args->datasets_count == MAX_DATASETS) return false;
    args->datasets[args->datasets_count++] = dataset;
    return true;
}

bool cli_args_add_generator_dataset(CliArgs* args, GeneratorDataset gd) {
    if (args->generator_datasets_count == MAX_DATASETS) return false;
    args->generator_datasets[args->generator_datasets_count++] = gd;
    return true;
}

bool parse_cli_long_option(CliArgs* result, StringView arg) {
    assert(sv_starts_with(arg, SV("--")));
    StringView opt = sv_slice(arg, 2, arg.len);

    StringView time_limit_string = sv_trim_prefix_or_null(opt, SV("time-limit="));
    if (!sv_is_null(time_limit_string)) {
        if (!parse_timespan(time_limit_string, &result->time_limit.value)) {
            return cli_errorf("--time-limit: Unknown timespan '%.*s'", (int) time_limit_string.len, time_limit_string.data);
        }

        result->time_limit.set = true;
        return true;
    }

    StringView time_per_char_limit_string = sv_trim_prefix_or_null(opt, SV("time-per-char-limit="));
    if (!sv_is_null(time_per_char_limit_string)) {
        if (!parse_timespan(time_per_char_limit_string, &result->time_per_char_limit.value)) {
            return cli_errorf("--time-per-char-limit: Unknown timespan '%.*s'", (int) time_per_char_limit_string.len, time_per_char_limit_string.data);
        }

        result->time_per_char_limit.set = true;
        return true;
    }

    if (sv_eql(opt, SV("help"))) {
        return cli_show_help();
    }

    bool is_negated = sv_starts_with(opt, SV("no-"));
    StringView fopt = sv_trim_prefix(opt, SV("no-"));

    if (sv_eql(fopt, SV("ignore-case"))) {
        return set_cli_switch(arg, &result->ignore_case, !is_negated);
    } else if (sv_eql(fopt, SV("ignore-punctuations"))) {
        return set_cli_switch(arg, &result->ignore_punctuations, !is_negated);
    } else if (sv_eql(fopt, SV("game-over-on-mistake"))) {
        return set_cli_switch(arg, &result->game_over_on_mistake, !is_negated);
    } else if (sv_eql(fopt, SV("game-over-on-exceed-time-limit"))) {
        return set_cli_switch(arg, &result->game_over_on_exceed_time_limit, !is_negated);
    } else if (sv_eql(fopt, SV("game-over-on-exceed-time-per-char-limit"))) {
        return set_cli_switch(arg, &result->game_over_on_exceed_time_per_char_limit, !is_negated);
    } else if (sv_eql(fopt, SV("retry"))) {
        return set_cli_switch(arg, &result->retry, !is_negated);
    } else {
        return cli_errorf("%.*s: Unknown option. Use --help/-h for help", (int) arg.len, arg.data);
    }
}

bool parse_cli_short_option(CliArgs* result, StringView arg) {
    assert(sv_starts_with(arg, SV("-")));

    for (size_t i = 1; i < arg.len; ++i) {
        char opt = arg.data[i];
        if (opt == 'i') {
            if (!set_cli_switch(arg, &result->ignore_case, true)) {
                return false;
            }
        } else if (opt == 'p') {
            if (!set_cli_switch(arg, &result->ignore_punctuations, true)) {
                return false;
            }
        } else if (opt == 'r') {
            if (!set_cli_switch(arg, &result->retry, true)) {
                return false;
            }
        } else if (opt == 'h') {
            return cli_show_help();
        } else {
            return cli_errorf("%.*s: Unknown option. Use --help/-h for help", (int) arg.len, arg.data);
        }
    }

    return true;
}

bool parse_cli_argument(CliArgs* result, StringView arg) {
    StringView builtin_dataset_name = sv_trim_prefix_or_null(arg, SV("@")); 
    if (!sv_is_null(builtin_dataset_name)) {
        DataSet dataset = load_builtin_dataset(builtin_dataset_name);
        if (!dataset_is_null(&dataset)) {
            return cli_args_add_dataset(result, dataset);
        }

        GeneratorDataset generator_dataset = load_builtin_generator_dataset(builtin_dataset_name);
        if (!generator_dataset_is_null(&generator_dataset)) {
            return cli_args_add_generator_dataset(result, generator_dataset);
        }

        size_t builtin_datasets_count;
        const BuiltinDatasetEntry* builtin_datasets = list_builtin_datasets(&builtin_datasets_count);

        size_t builtin_generator_datasets_count;
        const BuiltinGeneratorDatasetEntry* builtin_generator_datasets = list_builtin_generator_datasets(&builtin_generator_datasets_count);

        cli_errorf("No built-in dataset named %.*s was found!", (int) builtin_dataset_name.len, builtin_dataset_name.data);
        cli_errorf("all available built-in datasets:");
        for (size_t i = 0; i < builtin_datasets_count; ++i) {
            cli_errorf("    %.*s", (int) builtin_datasets[i].name.len, builtin_datasets[i].name.data);
        }
        for (size_t i = 0; i < builtin_generator_datasets_count; ++i) {
            cli_errorf("    %.*s (generator dataset)", (int) builtin_generator_datasets[i].name.len, builtin_generator_datasets[i].name.data);
        }

        return false;
    } else {
        DataSet dataset = load_dataset(arg);
        if (dataset_is_null(&dataset)) {
            return cli_errorf("The %.*s dataset could not be read. Check if this file path truly exists and if it contains valid data.", (int) arg.len, arg.data);
        }

        cli_args_add_dataset(result, dataset);
    }

    return true;
}

CliArgs parse_cli_args(int argc, char** argv) {
    CliArgs result = {0};
    bool parse_flags = true;
    for (size_t i = 1; i < (size_t) argc; ++i) {
        StringView arg = sv_from_cstr(argv[i]);
        
        if (sv_eql(arg, SV("--")) && parse_flags) {
            parse_flags = false;
            continue;
        }

        if (parse_flags && sv_starts_with(arg, SV("--"))) {
            if (!parse_cli_long_option(&result, arg)) {
                return CLI_ARGS_NULL;
            }
        } else if (parse_flags && sv_starts_with(arg, SV("-"))) {
            if (!parse_cli_short_option(&result, arg)) {
                return CLI_ARGS_NULL;
            }
        } else {
            if (!parse_cli_argument(&result, arg)) {
                return CLI_ARGS_NULL;
            }
        }
    }

    // if no dataset is specified, use the default setting
    if (result.datasets_count == 0 && result.generator_datasets_count == 0) {
        result.datasets[result.datasets_count++] = load_builtin_dataset(SV("english-words"));
    }
    return result;
}

void free_cli_args(CliArgs* args) {
    for (size_t i = 0; i < args->datasets_count; ++i) {
        free_dataset(&args->datasets[i]);
    }
}
