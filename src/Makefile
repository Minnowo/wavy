CC = gcc
CFLAGS = 
LDFLAGS = -lm

SRC_DIR = .
BIN_DIR = .

SRC_FILES = $(wildcard $(SRC_DIR)/*.c)
BIN_TARGET = $(BIN_DIR)/a.out


all: main

main:
	$(CC) $(CFLAGS) $(LDFLAGS) $(SRC_FILES) -o $(BIN_TARGET) 


clean:
	rm $(BIN_TARGET)
