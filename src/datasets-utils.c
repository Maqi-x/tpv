#include "datasets-utils.h"

#include "sv.h"
#include "dataset.h"
#include "generator-dataset.h"

#include <stdlib.h>

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

    double ratio = 0.0;
    if (real_count + gen_count > 0)
        ratio = (double) gen_count / (real_count + gen_count);

    double adjusted_prob = gen_prob * ratio;
    double r = (double) rand() / RAND_MAX;
    bool use_gen = (r < adjusted_prob);

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

