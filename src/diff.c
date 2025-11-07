#include "sv.h"
#include "ansi.h"

#include <stdio.h>
#include <stdlib.h>

static void print_backtrack(StringView a, StringView b, int dp[a.len + 1][b.len + 1], size_t i, size_t j) {
    if (i == 0 && j == 0)
        return;

    if (i > 0 && j > 0 && a.data[i - 1] == b.data[j - 1]) {
        print_backtrack(a, b, dp, i - 1, j - 1);
        printf("%c", a.data[i - 1]);
    } else if (j > 0 && (i == 0 || dp[i][j - 1] >= dp[i - 1][j])) {
        print_backtrack(a, b, dp, i, j - 1);
        printf(GREEN "%c" RESET, b.data[j - 1]);
    } else if (i > 0) {
        print_backtrack(a, b, dp, i - 1, j);
        printf(RED "%c" RESET, a.data[i - 1]);
    }
}

void print_diff(StringView a, StringView b) {
    int (*dp)[b.len + 1] = malloc(sizeof(int[a.len + 1][b.len + 1]));
    if (dp == NULL) return;

    for (size_t i = 0; i <= a.len; i++) {
        for (size_t j = 0; j <= b.len; j++) {
            if (i == 0 || j == 0) {
                dp[i][j] = 0;
            } else if (a.data[i - 1] == b.data[j - 1]) {
                dp[i][j] = dp[i - 1][j - 1] + 1;
            } else {
                dp[i][j] = (dp[i - 1][j] > dp[i][j - 1])
                            ? dp[i - 1][j]
                            : dp[i][j - 1];
            }
        }
    }

    print_backtrack(a, b, dp, a.len, b.len);
    printf("\n");

    free(dp);
}


