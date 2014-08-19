BUILD_DIR = .build
SRC_DIR = chan
LIB = libchan.a
CC = gcc
CFLAGS = -Wall
AR = ar ruv

all: clean $(LIB)

$(LIB): build
	$(AR) $(BUILD_DIR)/$(LIB) \
		$(BUILD_DIR)/chan.o \
		$(BUILD_DIR)/queue.o \
		$(BUILD_DIR)/mutex.o \
		$(BUILD_DIR)/pipe_sem.o \
		$(BUILD_DIR)/reentrant_lock.o

build:
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $(BUILD_DIR)/chan.o $(SRC_DIR)/chan.c
	$(CC) $(CFLAGS) -c -o $(BUILD_DIR)/queue.o $(SRC_DIR)/queue.c
	$(CC) $(CFLAGS) -c -o $(BUILD_DIR)/mutex.o $(SRC_DIR)/mutex.c
	$(CC) $(CFLAGS) -c -o $(BUILD_DIR)/pipe_sem.o $(SRC_DIR)/pipe_sem.c
	$(CC) $(CFLAGS) -c -o $(BUILD_DIR)/reentrant_lock.o $(SRC_DIR)/reentrant_lock.c

clean:
	rm -f $(BUILD_DIR)/*.o
	rm -f $(BUILD_DIR)/$(LIB)

