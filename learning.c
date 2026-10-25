#include "wordmate.h"

/* Check both saved word|meaning rows and older word-only rows. */
static int word_is_saved(const char *wanted) {
    FILE *file = fopen(MEMORIZED_FILE, "r");
    if (!file) return 0;

    char line[LINE_MAX];
    while (read_file_line(file, line, sizeof(line))) {
        char *separator = strchr(line, '|');
        if (separator) *separator = '\0';
        normalize_word(line);
        if (strcmp(line, wanted) == 0) {
            fclose(file);
            return 1;
        }
    }
    fclose(file);
    return 0;
}

/* Save a word and its meaning without adding the same word twice. */
int memorize_word(const char *word) {
    char normalized[WORD_MAX];
    char meaning[MEANING_MAX];
    snprintf(normalized, sizeof(normalized), "%s", word ? word : "");
    normalize_word(normalized);

    if (word_is_saved(normalized)) {
        printf("'%s' is already memorized.\n", normalized);
        return 1;
    }
    if (!dictionary_find_meaning(normalized, meaning, sizeof(meaning))) {
        printf("No entry found for '%s'.\n", normalized);
        return 0;
    }

    FILE *file = fopen(MEMORIZED_FILE, "a");
    if (!file) {
        perror(MEMORIZED_FILE);
        return 0;
    }
    int ok = fprintf(file, "%s|%s\n", normalized, meaning) >= 0;
    if (fclose(file) != 0) ok = 0;
    if (ok) printf("Saved '%s' and its meaning to Memorized Words.\n", normalized);
    return ok;
}

/* Display saved meanings, looking up entries from the older word-only format. */
int show_memorized(void) {
    FILE *file = fopen(MEMORIZED_FILE, "r");
    if (!file) {
        puts("You have no memorized words yet.");
        return 0;
    }

    char line[LINE_MAX];
    size_t index = 0;
    puts("\n=== Memorized Words ===");
    while (read_file_line(file, line, sizeof(line))) {
        char *separator = strchr(line, '|');
        if (!line[0]) continue;

        if (separator) {
            *separator++ = '\0';
            if (!separator[0]) {
                char meaning[MEANING_MAX];
                if (dictionary_find_meaning(line, meaning, sizeof(meaning))) {
                    printf("%zu. %s - %s\n", ++index, line, meaning);
                    continue;
                }
            }
            printf("%zu. %s - %s\n", ++index, line,
                   separator[0] ? separator : "(no meaning recorded)");
        } else {
            char meaning[MEANING_MAX];
            if (dictionary_find_meaning(line, meaning, sizeof(meaning))) {
                printf("%zu. %s - %s\n", ++index, line, meaning);
            } else {
                printf("%zu. %s - (no meaning recorded)\n", ++index, line);
            }
        }
    }
    fclose(file);
    if (!index) puts("You have no memorized words yet.");
    return index > 0;
}
