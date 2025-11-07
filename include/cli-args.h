#ifndef CLI_ARGS_H
#define CLI_ARGS_H

#include "timespan.h"
#include "dataset.h"
#include "generator-dataset.h"

#include <stddef.h>
#include <stdbool.h>

typedef struct CliSwitch {
    bool value;
    bool set;
} CliSwitch;

typedef struct CliTimeSpanOption {
    TimeSpanSec value;
    bool set;
} CliTimeSpanOption;

#ifndef MAX_DATASETS
#   define MAX_DATASETS 128
#endif

typedef struct CliArgs {
    DataSet datasets[MAX_DATASETS];
    size_t datasets_count;

    GeneratorDataset generator_datasets[MAX_DATASETS];
    size_t generator_datasets_count;

    CliTimeSpanOption time_limit;
    CliTimeSpanOption time_per_char_limit;

    CliSwitch retry;
    CliSwitch game_over_on_mistake;
    CliSwitch game_over_on_exceed_time_limit;
    CliSwitch game_over_on_exceed_time_per_char_limit;

    CliSwitch ignore_case;
    CliSwitch ignore_punctuations;
} CliArgs;

#define CLI_ARGS_NULL ((CliArgs) { 0 })

CliArgs parse_cli_args(int argc, char** argv);
void free_cli_args(CliArgs* args);

#endif // CLI_ARGS_H

