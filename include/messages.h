#ifndef MESSAGES_H
#define MESSAGES_H

#include <stdlib.h>
#include <stddef.h>

static inline const char* tpv_get_random_praise() {
    static const char* messages[] = {
        "Good!",
        "Excellent!",
        "100%!",
        "You rock!",
        "Yo!",
        "Perfectly!",
        "Exactly!",
        "Outstanding!",
        "Superb!",
        "Amazing!",
        "Fantastic!",
        "Great job!",
        "Impressive!",
        "Well done!",
        "Brilliant!",
        "Spectacular!",
        "Marvelous!",
        "Incredible!",
        "Flawless!",
        "Legendary!",
        "Phenomenal!",
        "Terrific!",
        "Splendid!",
        "Astounding!",
        "You nailed it!",
        "On fire!",
        "Unstoppable!",
        "Champion!",
        "Masterful!",
        "You crushed it!",
        "So fast!",
    };
    size_t messages_count = sizeof messages / sizeof messages[0];

    return messages[rand() % messages_count];
}

static inline const char* tpv_get_random_retry_message() {
    static const char* messages[] = {
        "Try again!",
        "Almost!",
        "Not quite!",
        "You can do it!",
        "Close one!",
        "Give it another shot!",
        "Keep going!",
        "Missed it!",
        "Oops!",
        "Next time for sure!",
        "Don't give up!",
        "Keep practicing!",
        "You'll get it!",
        "So close!",
        "One more time!",
        "Stay focused!",
        "Keep trying!",
        "Practice makes perfect!",
        "Don't worry!",
        "Hang in there!",
        "Persistence pays off!",
        "You'll nail it soon!",
        "Keep at it!",
        "Mistakes happen!",
        "Almost had it!",
        "Try once more!",
        "You got this!",
        "Don't stop now!",
        "Keep pushing!",
        "Give it another go!",
    };
    size_t messages_count = sizeof messages / sizeof messages[0];

    return messages[rand() % messages_count];
}

static inline const char* tpv_get_random_goodbye_message() {
    static const char* messages[] = {
        "Goodbye!",
        "See you later!",
        "Then goodbye!",
        "See you next time!",
    };
    size_t messages_count = sizeof messages / sizeof messages[0];

    return messages[rand() % messages_count];
}

#endif // MESSAGES_H

