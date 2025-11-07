#include "generator-dataset.h"

#include "sv.h"

#include <stdio.h>
#include <stdlib.h>

void random_string(char* buf, size_t length, StringView charset) {
    for (size_t i = 0; i < length; i++) {
        int key = rand() % charset.len;
        buf[i] = charset.data[key];
    }
    buf[length] = '\0';
}

#define RANDOM_STRINGS_GENERATORS_MIN_LEN 3
#define RANDOM_STRINGS_GENERATORS_MAX_LEN 13
#define RANDOM_STRINGS_GENERATORS_GET_RAND_LEN() \
    (rand() % (RANDOM_STRINGS_GENERATORS_MAX_LEN - RANDOM_STRINGS_GENERATORS_MIN_LEN) + RANDOM_STRINGS_GENERATORS_MIN_LEN)

StringView random_alpha_numeric_strings_generator() {
    static char buf[RANDOM_STRINGS_GENERATORS_MAX_LEN + 1];

    size_t len = RANDOM_STRINGS_GENERATORS_GET_RAND_LEN();
    random_string(buf, len, SV("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"));

    return sv_from_data_and_len(buf, len);
}

StringView random_alpha_strings_generator() {
    static char buf[RANDOM_STRINGS_GENERATORS_MAX_LEN + 1];

    size_t len = RANDOM_STRINGS_GENERATORS_GET_RAND_LEN();
    random_string(buf, len, SV("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"));

    return sv_from_data_and_len(buf, len);
}

#define RANDOM_NUMBERS_GENERATORS_MIN_NUM 0
#define RANDOM_NUMBERS_GENERATORS_MAX_NUM 9999999
#define RANDOM_NUMBERS_GENERATORS_GET_RAND_NUM() \
    (rand() % (RANDOM_NUMBERS_GENERATORS_MAX_NUM - RANDOM_NUMBERS_GENERATORS_MIN_NUM) + RANDOM_NUMBERS_GENERATORS_MIN_NUM)

StringView random_numbers_generator() {
    static char buf[32];

    int num = RANDOM_NUMBERS_GENERATORS_GET_RAND_NUM();
    int len = snprintf(buf, sizeof(buf), "%d", num);

    return sv_from_data_and_len(buf, (size_t) len);
}

GeneratorDataset random_alpha_numeric_strings_generator_dataset = {
    .gen = random_alpha_numeric_strings_generator,
};

GeneratorDataset random_alpha_strings_generator_dataset = {
    .gen = random_alpha_strings_generator,
};

GeneratorDataset random_numbers_generator_dataset = {
    .gen = random_numbers_generator,
};

