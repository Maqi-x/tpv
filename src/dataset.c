#include "dataset.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline char* read_file_alloced(const char* filepath, size_t* out_size) {
    FILE* file = fopen(filepath, "rb");
    if (file == NULL) goto e1;

    long size; int seek;
    if ((seek = fseek(file, 0, SEEK_END)) != 0) goto e2;
    if ((size = ftell(file))              <= 0) goto e2;
    rewind(file);

    char* data = malloc((size_t) size);
    if (data == NULL) goto e3;

    size_t readed = fread(data, sizeof(char), (size_t) size, file);
    if (readed != (size_t) size) goto e3;

    fclose(file);
    *out_size = size;
    return data;

e3: free(data);
e2: fclose(file);
e1: return NULL;
}

#define DATASET_ELEMENTS_INITIAL_CAPACITY 450000

static inline StringView* append_element(StringView** elements, size_t* elements_count, size_t* elements_cap, StringView elem) {
    if (*elements_cap == *elements_count) {
        *elements_cap = *elements_cap == 0 ? DATASET_ELEMENTS_INITIAL_CAPACITY : (size_t) ((double) *elements_count * 1.2);
        StringView* new_elements = realloc(*elements, *elements_cap * sizeof(StringView));
        if (new_elements == NULL) return NULL;
        *elements = new_elements;
    }

    (*elements)[(*elements_count)++] = elem;
    return *elements;
}

static inline StringView* parse_dataset_elements(StringView raw_content, size_t* out_elements_count) {
    size_t elements_count = 0, elements_cap = 0;
    StringView* elements = NULL;

    size_t last_element_start = 0;
    for (size_t i = 0; i < raw_content.len; ++i) {
        char c = raw_content.data[i];
        if (c == '\n') {
            if (append_element(&elements, &elements_count, &elements_cap,
                               sv_slice(raw_content, last_element_start, i)) == NULL) {
                goto e2;
            }
            last_element_start = i + 1;
            continue;
        }
    }

    *out_elements_count = elements_count;
    return elements;

e2: free(elements);
    return NULL;
}

DataSet load_dataset(StringView filepath){
    DataSet result;

    result._raw_content_owned = read_file_alloced(filepath.data, &result.raw_content.len);
    if (result._raw_content_owned == NULL) goto e1;

    result.raw_content.data = result._raw_content_owned;

    result.elements = parse_dataset_elements(result.raw_content, &result.elements_count);
    if (result.elements == NULL) goto e2;

    return result;

e2: free(result._raw_content_owned);
e1: return DATASET_NULL;
}

DataSet parse_dataset_from_str(StringView raw_content) {
    DataSet result;
    
    result._raw_content_owned = NULL;
    result.raw_content = raw_content;

    result.elements = parse_dataset_elements(result.raw_content, &result.elements_count);
    if (result.elements == NULL) return DATASET_NULL;

    return result;
}

void free_dataset(DataSet* dataset) {
    free(dataset->elements);
    if (dataset->_raw_content_owned != NULL) {
        free(dataset->_raw_content_owned);
    }
}

StringView random_dataset_element(DataSet* dataset) {
    if (dataset->elements_count == 0) {
        return SV_NULL;
    }
    return dataset->elements[rand() % dataset->elements_count];
}
