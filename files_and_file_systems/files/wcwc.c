#include <locale.h>
#include <stdio.h>
#include <wchar.h>
#include <wctype.h>

typedef unsigned long long count_t;
typedef struct { count_t strings, words, chars; } counts_t;

/*
 * Buffer keep two last red chars
 *
 * It is enough to find out:
 *
 *  - if non-EOF char was red
 *  - if new word started (non-space char was red after space-like char)
 *  - if new line started (L'\n' was red)
 */
typedef struct { wint_t prev, cur; } buff_t;

counts_t total = { 0, 0, 0 };

/*
 * Prints wc-like output for counts set
 * Applicable to print both file and total counts
 */
void print_counts_string(counts_t counts, char *filename) {
    // 20 is the length of 2^64 decimal representation
    printf("%20llu %20llu %20llu", counts.strings, counts.words, counts.chars);
    if (filename != NULL) printf(" %s", filename);
    printf("\n");
}

/*
 * Analyses buffer state and updates counts
 *
 * See buffer_t description above
 */
void update_count(counts_t *counts, buff_t buff) {
    counts -> chars += 1;
    if (buff.cur == L'\n') counts -> strings += 1;
    if (!iswspace(buff.cur) && iswspace(buff.prev)) counts -> words += 1;
}

/*
 * Trying to count lines/words/chars inside stream (file or stdin) and,
 * in case of succes, prints those counts
 */
void report_stream(char *filename) {
    FILE *file = filename ? fopen(filename, "r") : stdin;
    if (file == NULL) {
        printf("%20s %20s %20s %s\n", "-", "-", "-", filename);
        return;
    }
    counts_t counts = { 0, 0, 0 };
    buff_t buff = { ' ', ' ' };
    while ((buff.cur = fgetwc(file)) != WEOF) {
        update_count(&counts, buff);
        update_count(&total,  buff);
        buff.prev = buff.cur;
    }

    print_counts_string(counts, filename);
}

int main(int argc, char **argv) {
    setlocale(LC_CTYPE, "");
    if (argc == 1) report_stream(NULL);
    for (int i = 1; i < argc; i++) report_stream(argv[i]);
    if (argc > 2) print_counts_string(total, "total");

    return 0;
}
