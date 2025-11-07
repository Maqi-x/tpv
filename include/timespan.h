#ifndef TIMESPAN_H
#define TIMESPAN_H

#include "sv.h"

#include <time.h>     // for timespec, clock_gettime

typedef double TimeSpanSec;

static inline TimeSpanSec now() {
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    return tp.tv_sec + (tp.tv_nsec / 1e9);
}

bool parse_timespan(StringView str, TimeSpanSec* out_timespan);

#endif // TIMESPAN_H

