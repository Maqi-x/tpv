#ifndef GENERATOR_DATASET_H
#define GENERATOR_DATASET_H

#include "sv.h"

typedef struct GeneratorDataset {
    StringView (*gen)();
} GeneratorDataset;

#define GENERATOR_DATASET_NULL ((GeneratorDataset) { 0 }) 

static inline bool generator_dataset_is_null(GeneratorDataset* gd) {
    return gd->gen == NULL;
}

extern GeneratorDataset
        random_alpha_numeric_strings_generator_dataset,
        random_alpha_strings_generator_dataset,
        random_numbers_generator_dataset;

#endif // GENERATOR_DATASET_H

