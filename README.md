# Dictionary_x_c_pmr

A small C dictionary program. It reads one A-Z sorted file, `azdictionary.txt`. The file has a visible `# A` section, then the `# B` section, and so on. Each definition is stored on one line as `word<TAB>meaning`.

The dictionary file is prepared ahead of time. The program reads that file directly; it does not create letter files or indexes.

## Build and run

```sh
make
./wordmate
```

## Menu features

- Search a word. If it is misspelled, spelling suggestions appear automatically. A correctly spelled result can be added to Memorized Words.
- Add, update, or delete a dictionary entry in `azdictionary.txt`.
- Show a random Word of the Day or list saved memorized words.
- Play Project Hangman. Guess one letter at a time using the word's meaning as a clue.
- View activity history.

## Files

- `azdictionary.txt`: all dictionary definitions, sorted by word and grouped into A-Z sections
- `main.c`, `menu.c`: command-line handling and the interactive menu
- `dictionary.c`: direct search, suggestions, dictionary edits and random word selection
- `learning.c`: memorized words
- `games.c`: Project Hangman
- `results.c`: activity history
- `util.c`: input and word helpers
- `wordmate.h`: shared types and function declarations
