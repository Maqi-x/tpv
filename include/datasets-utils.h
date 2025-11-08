#ifndef DATASETS_UTILS
#define DATASETS_UTILS

#include "dataset.h"
#include "generator-dataset.h"
#include "sv.h"

StringView random_element_from_many_datasets(DataSet* datasets, size_t datasets_count);

StringView random_element(DataSet* real, size_t real_count,
                          GeneratorDataset* gen, size_t gen_count,
                          double gen_prob);
#endif // DATASETS_UTILS

