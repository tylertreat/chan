BUILD_DIR = .build
SRC_DIR = chan
LIB = libchan.a
CC = gcc
CFLAGS = -Wall
AR = ar ruv

all: clean $(LIB)

$(LIB): build
	$(AR) $(BUILD_DIR)/$(LIB) $(BUILD_DIR)/chan.o $(BUILD_DIR)/queue.o

build:
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $(BUILD_DIR)/chan.o $(SRC_DIR)/chan.c
	$(CC) $(CFLAGS) -c -o $(BUILD_DIR)/queue.o $(SRC_DIR)/queue.c

clean:
	rm -f $(BUILD_DIR)/*.o
	rm -f $(BUILD_DIR)/$(LIB)

