# Compiler settings can be overridden on the make command line.
CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2
LDFLAGS ?=
TARGET = wordmate
SOURCES = main.c menu.c dictionary.c learning.c games.c results.c util.c
OBJECTS = $(SOURCES:.c=.o)

.PHONY: all clean prepare
all: $(TARGET)

# Create the folder used for saved words and activity history.
prepare:
	mkdir -p data

# Prepare saved-data storage before parallel compilation starts.
$(OBJECTS): | prepare

# Link all module objects into the single command-line program.
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJECTS)

%.o: %.c wordmate.h
	$(CC) $(CFLAGS) -c $< -o $@

# Remove generated build outputs without touching dictionary or user data files.
clean:
	rm -f $(TARGET) $(OBJECTS)
