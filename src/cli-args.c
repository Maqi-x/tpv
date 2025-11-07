#include "cli-args.h"

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
    builtin_datasets[0] = (BuiltinDatasetEntry) { .name = SV("english-words"),        .data = embed_english_words_data,        .size = embed_english_words_size        };
    builtin_datasets[1] = (BuiltinDatasetEntry) { .name = SV("english-sentences"),    .data = embed_english_sentences_data,    .size = embed_english_sentences_size    };
    builtin_datasets[2] = (BuiltinDatasetEntry) { .name = SV("programming-snippets"), .data = embed_programming_snippets_data, .size = embed_programming_snippets_size };
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

bool cli_errorf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    vprintf(fmt, args);

    va_end(args);
    putchar('\n');
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
            return cli_errorf("--time-limit: Unknown timespan '%s'", time_limit_string);
        }

        result->time_limit.set = true;
        return true;
    }

    StringView time_per_char_limit_string = sv_trim_prefix_or_null(opt, SV("time-per-char-limit="));
    if (!sv_is_null(time_per_char_limit_string)) {
        if (!parse_timespan(time_per_char_limit_string, &result->time_per_char_limit.value)) {
            return cli_errorf("--time-per-char-limit: Unknown timespan '%s'", time_per_char_limit_string);
        }

        result->time_per_char_limit.set = true;
        return true;
    }

    bool is_negated = sv_starts_with(opt, SV("not-"));
    StringView fopt = sv_trim_prefix(opt, SV("not-"));

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
    } else {
        return cli_errorf("%s: Unknown option", arg);
    }
}

bool parse_cli_short_option(CliArgs* result, StringView arg) {
    assert(sv_starts_with(arg, SV("-")));
    StringView opt = sv_slice(arg, 1, arg.len);

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

        cli_errorf("No built-in dataset named %s was found!", builtin_dataset_name);
        cli_errorf("all available built-in datasets:");
        for (size_t i = 0; i < builtin_datasets_count; ++i) {
            cli_errorf("    %s", builtin_datasets[i].name);
        }
        for (size_t i = 0; i < builtin_generator_datasets_count; ++i) {
            cli_errorf("    %s (generator dataset)", builtin_generator_datasets[i].name);
        }

        return false;
    } else {
        DataSet dataset = load_dataset(arg);
        if (dataset_is_null(&dataset)) {
            return cli_errorf("The %s dataset could not be read. Check if this file path truly exists and if it contains valid data.", arg);
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
