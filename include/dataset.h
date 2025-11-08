#ifndef DATASET_H
#define DATASET_H

#include "sv.h"

typedef struct DataSet {
    StringView* elements;
    size_t elements_count;
    StringView raw_content;

    char* _raw_content_owned;
} DataSet;

#define DATASET_NULL ((DataSet) { 0 })

static inline bool dataset_is_null(DataSet* dataset) {
    return sv_is_null(dataset->raw_content);
}

DataSet load_dataset(StringView filepath);
DataSet parse_dataset_from_str(StringView raw_content);
void free_dataset(DataSet* dataset);
StringView random_dataset_element(DataSet* dataset);

#endif // DATASET_H

