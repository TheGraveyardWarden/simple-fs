CC=gcc
SRC_DIR=./src
UTILS_DIR=./utils
BIN_DIR=./bin
OBJ_DIR=./obj
INCLUDE_DIRS=-I$(SRC_DIR)
BIN=$(BIN_DIR)/simple-fs
SRCS=$(wildcard $(SRC_DIR)/*.c)
OBJS=$(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

.PHONY: mkdir all

all: mkdir mkfs $(OBJS)
	$(CC) $(INCLUDE_DIRS) $(OBJS) -o $(BIN)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(INCLUDE_DIRS) -c $^ -o $@

mkfs: $(OBJ_DIR)/fs.o
	$(CC) $(INCLUDE_DIRS) $^ $(UTILS_DIR)/mkfs.c -o $(BIN_DIR)/mkfs
	$(CC) $(INCLUDE_DIRS) $^ $(UTILS_DIR)/fstest.c -o $(BIN_DIR)/fstest

mkdir:
	@if [ ! -d bin ]; then mkdir bin; fi
	@if [ ! -d obj ]; then mkdir obj; fi

clean:
	@rm bin -r
	@rm obj -r
