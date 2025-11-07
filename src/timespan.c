#include "timespan.h"
#include "sv.h"

#include <ctype.h>
#include <stdlib.h>

typedef struct TimeSpanUnit {
    StringView name_singular;
    StringView name_plural;
    StringView* shortcuts;

    double multiplier;
} TimeSpanUnit;

static const TimeSpanUnit time_units[] = {
    {
        .name_singular = SV("milisecond"),
        .name_plural = SV("miliseconds"),
        .shortcuts = (StringView[]) { SV("ms"), SV("milis"), SV_NULL },

        .multiplier = 0.001,
    },
    {
        .name_singular = SV("second"),
        .name_plural = SV("seconds"),
        .shortcuts = (StringView[]) { SV("s"), SV("sec"), SV("secs"), SV_NULL },

        .multiplier = 1.0,
    },
    {
        .name_singular = SV("minute"),
        .name_plural = SV("minutes"),
        .shortcuts = (StringView[]) { SV("m"), SV("min"), SV("mins"), SV_NULL },

        .multiplier = 60.0,
    },
    {
        .name_singular = SV("hour"),
        .name_plural = SV("hours"),
        .shortcuts = (StringView[]) { SV("h"), SV("hr"), SV_NULL },

        .multiplier = 60.0 * 60.0,
    }
};

static const size_t time_units_count = sizeof(time_units) / sizeof(time_units[0]);


static const TimeSpanUnit* find_time_unit(StringView unit_sv) {
    for (size_t i = 0; i < time_units_count; ++i) {
        const TimeSpanUnit* unit = &time_units[i];

        if (sv_eql(unit_sv, unit->name_plural)) {
            return unit;
        }

        for (StringView* s = unit->shortcuts; s && s->data; ++s) {
            if (sv_eql(unit_sv, *s))
                return unit;
        }
    }
    return NULL;
}

bool parse_timespan(StringView str, TimeSpanSec* out_timespan) {
    for (size_t i = 0; i < time_units_count; ++i) {
        const TimeSpanUnit* unit = &time_units[i];

        if (sv_eql(sv_trim_prefix(str, SV("1")), unit->name_singular)) {
            *out_timespan = 1 * unit->multiplier;
            return true;
        }
    }

    const char* p = str.data;
    const char* end = str.data + str.len;
    double total = 0.0;

    while (p < end) {
        while (p < end && isspace((unsigned char) *p))
            ++p;
        if (p >= end)
            break;

        const char* num_start = p;
        while (p < end && (isdigit((unsigned char) *p) || *p == '.'))
            ++p;

        if (num_start == p)
            break; 

        double value = strtod(num_start, NULL);

        const char* unit_start = p;
        while (p < end && isalpha((unsigned char) *p)) {
            ++p;
        }

        size_t unit_len = (size_t) (p - unit_start);
        StringView unit_sv = sv_from_data_and_len(unit_start, unit_len);

        const TimeSpanUnit* u = find_time_unit(unit_sv);
        if (u) {
            total += value * u->multiplier;
        } else {
            return false;
        }
    }

    *out_timespan = total;
    return true;
}

