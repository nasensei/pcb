CC = gcc
CFLAGS = -g -Wall -Wextra -Iinclude
LIBS = -lm
TARGET = pcb_check

SRC_DIR = src
OBJ_DIR = build
INC_DIR = include

# List of source files (include "bitmap.c")
_SRC = main.c bitmap.c
_DEPS = bitmap.h
_OBJS = $(_SRC:.c=.o)  # Automatically generate object file names

DEPS = $(patsubst %,$(INC_DIR)/%,$(_DEPS))
SRCS = $(patsubst %,$(SRC_DIR)/%,$(_SRC))
OBJS = $(patsubst %,$(OBJ_DIR)/%,$(_OBJS))

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LIBS)

.PHONY: clean
clean:
	$(RM) $(TARGET) $(OBJS)

