#include "wordmate.h"

#include <time.h>

static const char *const hangman_stages[] = {
    "  +---+\n  |   |\n  O   |\n /|\\  |\n / \\  |\n      |\n=========\n",
    "  +---+\n  |   |\n  O   |\n /|\\  |\n /    |\n      |\n=========\n",
    "  +---+\n  |   |\n  O   |\n /|\\  |\n      |\n      |\n=========\n",
    "  +---+\n  |   |\n  O   |\n /|   |\n      |\n      |\n=========\n",
    "  +---+\n  |   |\n  O   |\n  |   |\n      |\n      |\n=========\n",
    "  +---+\n  |   |\n  O   |\n      |\n      |\n      |\n=========\n",
    "  +---+\n  |   |\n      |\n      |\n      |\n      |\n=========\n"
};

/* Read one line from the player and remove its newline. */
static int read_line(const char *prompt, char *buffer, size_t size) {
    fputs(prompt, stdout);
    if (!fgets(buffer, (int)size, stdin)) return 0;
    trim_newline(buffer);
    return 1;
}

/* Hangman words must contain only lowercase English letters. */
static int is_simple_word(const char *word) {
    if (*word == '\0') return 0;

    for (const unsigned char *letter = (const unsigned char *)word; *letter; ++letter) {
        if (*letter < 'a' || *letter > 'z') return 0;
    }
    return 1;
}

/* Pick a random eligible word from the dictionary's game-word pool. */
static GameWord *choose_random_word(GameWord *pool, size_t count, size_t max_length) {
    size_t available = 0;

    for (size_t i = 0; i < count; ++i) {
        size_t length = strlen(pool[i].word);
        if (is_simple_word(pool[i].word) && length <= max_length) available++;
    }
    if (available == 0) return NULL;

    size_t selected = (size_t)rand() % available;
    for (size_t i = 0; i < count; ++i) {
        size_t length = strlen(pool[i].word);
        if (!is_simple_word(pool[i].word) || length > max_length) continue;
        if (selected == 0) return &pool[i];
        --selected;
    }
    return NULL;
}

/* Play Project Hangman using a random word and its meaning as the clue. */
int run_hangman(void) {
    GameWord *pool = NULL;
    size_t count = 0;

    if (!dictionary_load_game_words(&pool, &count)) return 0;

    GameWord *answer = choose_random_word(pool, count, WORD_MAX - 1);
    if (!answer) {
        puts("There are no suitable words for Project Hangman.");
        free(pool);
        return 0;
    }

    int guessed[26] = {0};
    int wrong_guesses = 0;
    int won = 0;
    time_t started = time(NULL);

    puts("\nProject Hangman");
    puts("Guess one letter at a time. You have 6 wrong guesses.");
    printf("Clue: %s\n", answer->meaning);
    fputs(hangman_stages[6], stdout);

    while (wrong_guesses < 6) {
        int word_revealed = 1;
        printf("Word: ");

        for (const char *letter = answer->word; *letter; ++letter) {
            if (guessed[*letter - 'a']) {
                putchar((char)toupper((unsigned char)*letter));
            } else {
                putchar('_');
                word_revealed = 0;
            }
        }
        putchar('\n');

        if (word_revealed) {
            won = 1;
            break;
        }

        char input[LINE_MAX];
        if (!read_line("Guess one letter: ", input, sizeof(input))) {
            free(pool);
            return 0;
        }
        if (strlen(input) != 1) {
            puts("Please enter one letter.");
            continue;
        }

        char guess = (char)tolower((unsigned char)input[0]);
        if (guess < 'a' || guess > 'z') {
            puts("Please enter an English letter.");
            continue;
        }
        if (guessed[guess - 'a']) {
            puts("You already tried that letter.");
            continue;
        }
        guessed[guess - 'a'] = 1;

        if (strchr(answer->word, guess)) {
            puts("Good guess!");
        } else {
            ++wrong_guesses;
            puts("That letter is not in the word.");
            fputs(hangman_stages[6 - wrong_guesses], stdout);
        }
    }

    if (won) printf("You win! The word was '%s'.\n", answer->word);
    else printf("Game over. The word was '%s'.\n", answer->word);

    unsigned long elapsed = (unsigned long)(time(NULL) - started);
    save_activity_result("Project Hangman", won ? 1 : 0, 1,
                         won ? 10 : 0, elapsed);
    free(pool);
    return 1;
}
