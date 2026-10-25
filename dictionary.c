#include "wordmate.h"

#define GAME_WORD_LIMIT 4000

static void suggest_spelling(const char *wanted);

/* Map letters to their A-Z section number. */
static int letter_number(char letter) {
    switch (tolower((unsigned char)letter)) {
        case 'a': return 1;
        case 'b': return 2;
        case 'c': return 3;
        case 'd': return 4;
        case 'e': return 5;
        case 'f': return 6;
        case 'g': return 7;
        case 'h': return 8;
        case 'i': return 9;
        case 'j': return 10;
        case 'k': return 11;
        case 'l': return 12;
        case 'm': return 13;
        case 'n': return 14;
        case 'o': return 15;
        case 'p': return 16;
        case 'q': return 17;
        case 'r': return 18;
        case 's': return 19;
        case 't': return 20;
        case 'u': return 21;
        case 'v': return 22;
        case 'w': return 23;
        case 'x': return 24;
        case 'y': return 25;
        case 'z': return 26;
        default: return 0;
    }
}

/* Read one word and meaning from the pre-sorted dictionary text file. */
static int read_word(FILE *file, Word *entry) {
    char line[LINE_MAX];

    while (read_file_line(file, line, sizeof(line))) {
        if (!line[0] || line[0] == '#') continue;

        char *separator = strchr(line, '\t');
        if (!separator) continue;
        *separator++ = '\0';
        snprintf(entry->word, sizeof(entry->word), "%s", line);
        snprintf(entry->meaning, sizeof(entry->meaning), "%s", separator);
        return letter_number(entry->word[0]) != 0;
    }
    return 0;
}

/* Keep tabs and line breaks from becoming part of a dictionary field. */
static void clean_field(char *text) {
    for (char *letter = text; *letter; ++letter) {
        if (*letter == '\t' || *letter == '\n' || *letter == '\r' || *letter == '|') {
            *letter = ' ';
        }
    }
}

/* Write a new row in the same simple word<TAB>meaning format. */
static int write_word(FILE *file, const Word *entry) {
    char word[WORD_MAX];
    char meaning[MEANING_MAX];
    snprintf(word, sizeof(word), "%s", entry->word);
    snprintf(meaning, sizeof(meaning), "%s", entry->meaning);
    clean_field(word);
    clean_field(meaning);
    return fprintf(file, "%s\t%s\n", word, meaning) >= 0;
}

/* Check that the one dictionary file is available before the menu starts. */
int ensure_data_ready(void) {
    FILE *file = fopen(DICTIONARY_FILE, "r");
    if (!file) {
        fprintf(stderr, "Could not open %s. Keep it beside the program.\n",
                DICTIONARY_FILE);
        return 0;
    }
    fclose(file);
    return 1;
}

/* Print a dictionary row in a consistent format. */
static void display_word(const Word *entry) {
    printf("\n%s\n  %s\n", entry->word,
           entry->meaning[0] ? entry->meaning : "(no meaning recorded)");
}

/* Find and display all definitions for one exact spelling. */
int dictionary_search(const char *word) {
    char wanted[WORD_MAX];
    Word entry;
    int target_section;
    int found = 0;

    snprintf(wanted, sizeof(wanted), "%s", word ? word : "");
    normalize_word(wanted);
    target_section = letter_number(wanted[0]);
    if (!target_section) {
        puts("Enter a word beginning with A-Z.");
        return 0;
    }

    FILE *file = fopen(DICTIONARY_FILE, "r");
    if (!file) return 0;

    while (read_word(file, &entry)) {
        int section = letter_number(entry.word[0]);
        if (section < target_section) continue;
        if (section > target_section) break;

        int order = strcmp(entry.word, wanted);
        if (order == 0) {
            display_word(&entry);
            found = 1;
        } else if (found && order > 0) {
            break;
        }
    }
    fclose(file);

    if (!found) {
        printf("No entry found for '%s'.\n", wanted);
        suggest_spelling(wanted);
        return 0;
    }

    /* Ask to memorize only after an exact spelling match. */
    char answer[16];
    printf("Memorize '%s'? Press y for yes, or Enter to skip: ", wanted);
    if (fgets(answer, sizeof(answer), stdin) &&
        tolower((unsigned char)answer[0]) == 'y') {
        memorize_word(wanted);
    }
    return 1;
}

/* Get a word's first meaning for the memorized-word list. */
int dictionary_find_meaning(const char *word, char *meaning, size_t size) {
    char wanted[WORD_MAX];
    Word entry;
    int target_section;

    snprintf(wanted, sizeof(wanted), "%s", word ? word : "");
    normalize_word(wanted);
    target_section = letter_number(wanted[0]);
    if (!target_section) return 0;

    FILE *file = fopen(DICTIONARY_FILE, "r");
    if (!file) return 0;
    while (read_word(file, &entry)) {
        int section = letter_number(entry.word[0]);
        if (section < target_section) continue;
        if (section > target_section) break;
        int order = strcmp(entry.word, wanted);
        if (order == 0) {
            snprintf(meaning, size, "%s", entry.meaning);
            fclose(file);
            return 1;
        }
        if (order > 0) break;
    }
    fclose(file);
    return 0;
}

/* Compare letter positions after the shared first letter. */
static int spelling_similarity(const char *wanted, const char *candidate) {
    size_t wanted_length = strlen(wanted);
    size_t candidate_length = strlen(candidate);
    size_t longest = wanted_length > candidate_length ? wanted_length : candidate_length;
    int matches = 0;

    if (wanted[0] != candidate[0] || longest <= 1) return 0;
    for (size_t i = 1; i < wanted_length && i < candidate_length; ++i) {
        if (wanted[i] == candidate[i]) matches++;
    }
    return matches * 100 / (int)(longest - 1);
}

/* Offer spelling suggestions from the matching A-Z section only. */
static void suggest_spelling(const char *wanted) {
    Word suggestions[10];
    Word entry;
    char previous[WORD_MAX] = "";
    size_t count = 0;
    int target_section = letter_number(wanted[0]);

    FILE *file = fopen(DICTIONARY_FILE, "r");
    if (!file) return;

    while (read_word(file, &entry)) {
        int section = letter_number(entry.word[0]);
        if (section < target_section) continue;
        if (section > target_section) break;
        if (strcmp(entry.word, previous) == 0) continue;
        snprintf(previous, sizeof(previous), "%s", entry.word);
        if (spelling_similarity(wanted, entry.word) > 70 && count < 10) {
            suggestions[count++] = entry;
        }
    }
    fclose(file);

    if (!count) {
        puts("No spelling suggestions were found.");
        return;
    }

    puts("Your Word can probably be: ");
    for (size_t i = 0; i < count; ++i) {
        printf("%zu. %s\n", i + 1, suggestions[i].word);
    }

    char input[LINE_MAX];
    for (;;) {
        char *end;
        fputs("Choose a number to see its meaning (0 to cancel): ", stdout);
        if (!fgets(input, sizeof(input), stdin)) return;
        trim_newline(input);
        long selected = strtol(input, &end, 10);
        while (*end && isspace((unsigned char)*end)) ++end;
        if (end != input && *end == '\0' && selected >= 0 &&
            selected <= (long)count) {
            if (selected > 0) display_word(&suggestions[selected - 1]);
            return;
        }
        printf("Enter a number from 0 to %zu.\n", count);
    }
}

/* Rewrite one dictionary file to add, update, or delete a headword. */
static int rewrite_dictionary(const char *target, const Word *replacement, int mode) {
    FILE *input = fopen(DICTIONARY_FILE, "r");
    if (!input) return 0;

    char temporary[128];
    snprintf(temporary, sizeof(temporary), "%s.tmp", DICTIONARY_FILE);
    FILE *output = fopen(temporary, "w");
    if (!output) {
        fclose(input);
        return 0;
    }

    char line[LINE_MAX];
    int target_section = letter_number(target[0]);
    int inserted = 0;
    int matches = 0;
    int ok = 1;

    while (read_file_line(input, line, sizeof(line))) {
        /* Put a new word just before the next letter's section. */
        if (line[0] == '#') {
            int section = letter_number(line[2]);
            if (mode == 1 && !inserted && section > target_section) {
                if (!write_word(output, replacement)) ok = 0;
                inserted = 1;
            }
            if (fprintf(output, "%s\n", line) < 0) ok = 0;
            continue;
        }

        char copy[LINE_MAX];
        snprintf(copy, sizeof(copy), "%s", line);
        char *separator = strchr(copy, '\t');
        if (separator) {
            *separator = '\0';
            normalize_word(copy);
            int order = strcmp(copy, target);
            if (mode == 1 && !inserted && order > 0) {
                if (!write_word(output, replacement)) ok = 0;
                inserted = 1;
            }
            if (order == 0) {
                matches++;
                if (mode == 1) {
                    if (fprintf(output, "%s\n", line) < 0) ok = 0;
                } else if (mode == 2 && !inserted) {
                    if (!write_word(output, replacement)) ok = 0;
                    inserted = 1;
                }
                if (mode != 1) continue;
                continue;
            }
        }
        if (fprintf(output, "%s\n", line) < 0) ok = 0;
    }

    if (mode == 1 && !inserted) {
        if (!write_word(output, replacement)) ok = 0;
    }
    if ((mode == 1 && matches > 0) || (mode != 1 && matches == 0)) ok = 0;
    if (ferror(input)) ok = 0;
    fclose(input);
    if (fclose(output) != 0) ok = 0;

    if (ok && rename(temporary, DICTIONARY_FILE) == 0) return 1;
    remove(temporary);
    return 0;
}

/* Normalize a new word and its meaning before saving. */
static int normalize_entry(Word *entry) {
    if (!entry || !entry->word[0]) return 0;
    normalize_word(entry->word);
    clean_field(entry->word);
    clean_field(entry->meaning);
    return letter_number(entry->word[0]) != 0;
}

int dictionary_add(const Word *word) {
    Word entry = *word;
    if (!normalize_entry(&entry) || !rewrite_dictionary(entry.word, &entry, 1)) {
        puts("That word could not be added. It may already exist.");
        return 0;
    }
    printf("Added '%s'.\n", entry.word);
    return 1;
}

int dictionary_update(const Word *word) {
    Word entry = *word;
    if (!normalize_entry(&entry) || !rewrite_dictionary(entry.word, &entry, 2)) {
        printf("No entry found for '%s'.\n", entry.word);
        return 0;
    }
    printf("Updated '%s'.\n", entry.word);
    return 1;
}

int dictionary_delete(const char *word) {
    char target[WORD_MAX];
    snprintf(target, sizeof(target), "%s", word ? word : "");
    normalize_word(target);
    if (!letter_number(target[0]) || !rewrite_dictionary(target, NULL, 3)) {
        printf("No entry found for '%s'.\n", target);
        return 0;
    }
    printf("Deleted '%s'.\n", target);
    return 1;
}

/* Count each distinct dictionary headword once. */
static size_t count_words(void) {
    FILE *file = fopen(DICTIONARY_FILE, "r");
    if (!file) return 0;
    char previous[WORD_MAX] = "";
    Word entry;
    size_t count = 0;
    while (read_word(file, &entry)) {
        if (strcmp(entry.word, previous) != 0) {
            count++;
            snprintf(previous, sizeof(previous), "%s", entry.word);
        }
    }
    fclose(file);
    return count;
}

/* Count non-empty saved words for the simple dictionary statistics view. */
static size_t count_memorized_words(void) {
    FILE *file = fopen(MEMORIZED_FILE, "r");
    if (!file) return 0;
    char line[LINE_MAX];
    size_t count = 0;
    while (read_file_line(file, line, sizeof(line))) {
        if (line[0]) count++;
    }
    fclose(file);
    return count;
}

int dictionary_stats(void) {
    printf("Dictionary words: %zu\nMemorized words: %zu\n",
           count_words(), count_memorized_words());
    return 1;
}

/* Load a random sample of words for Missing Letters and Hangman. */
int dictionary_load_game_words(GameWord **words, size_t *count) {
    if (!words || !count) return 0;
    *words = NULL;
    *count = 0;

    GameWord *pool = malloc(GAME_WORD_LIMIT * sizeof(*pool));
    if (!pool) return 0;

    FILE *file = fopen(DICTIONARY_FILE, "r");
    if (!file) {
        free(pool);
        return 0;
    }

    char previous[WORD_MAX] = "";
    Word entry;
    size_t seen = 0;
    while (read_word(file, &entry)) {
        if (strcmp(entry.word, previous) == 0) continue;
        snprintf(previous, sizeof(previous), "%s", entry.word);

        size_t length = strlen(entry.word);
        if (length < 3 || length > 16 || strlen(entry.meaning) < 20) continue;
        seen++;
        size_t selected = (size_t)rand() % seen;
        if (*count < GAME_WORD_LIMIT) {
            pool[(*count)++] = (GameWord){0};
            selected = *count - 1;
        } else if (selected >= GAME_WORD_LIMIT) {
            continue;
        }
        snprintf(pool[selected].word, sizeof(pool[selected].word), "%s", entry.word);
        snprintf(pool[selected].meaning, sizeof(pool[selected].meaning), "%s", entry.meaning);
    }
    fclose(file);

    if (*count == 0) {
        puts("There are not enough suitable dictionary words for a game.");
        free(pool);
        return 0;
    }
    *words = pool;
    return 1;
}

/* Pick a random word and meaning from the sample for Word of the Day. */
int dictionary_random(void) {
    GameWord *words = NULL;
    size_t count = 0;
    if (!dictionary_load_game_words(&words, &count)) return 0;
    size_t selected = (size_t)rand() % count;
    printf("Word of the Day:\n\n%s\n  %s\n",
           words[selected].word, words[selected].meaning);
    free(words);
    return 1;
}
