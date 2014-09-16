SRC ?= src
BUILD ?= build
EXAMPLES ?= examples
UNAME := $(shell uname)

AR ?= ar
CC ?= gcc
CFLAGS = -Ideps -pedantic -std=c99 -v -Wall -Wextra -lpthread

ifeq ($(UNAME), Linux)
	CFLAGS += -lrt
endif

ifeq ($(APP_DEBUG), true)
	CFLAGS += -g -O0
else
	CFLAGS += -O2
endif

PREFIX ?= /usr/local

SRCS += $(wildcard $(SRC)/*.c)

OBJS += $(SRCS:.c=.o)

all: build

%.o: %.c
	$(CC) $< $(CFLAGS) -c -o $@

build: $(BUILD)/lib/libchan.a
	mkdir -p $(BUILD)/include/chan
	cp -f $(SRC)/chan.h $(BUILD)/include/chan/chan.h
	cp -f $(SRC)/queue.h $(BUILD)/include/chan/queue.h

$(BUILD)/lib/libchan.a: $(OBJS)
	mkdir -p $(BUILD)/lib
	$(AR) -crs $@ $^

clean:
	rm -rf *.o $(BUILD) $(SRC)/*.o test

check: test
	@./test

test: $(SRC)/chan_test.o $(OBJS)
	$(CC) $^ -o $@ $(CFLAGS)

example: build
	mkdir -p $(BUILD)/examples
	$(CC) $(CFLAGS) -I$(build)/include -o $(BUILD)/examples/buffered $(EXAMPLES)/buffered.c -Lbuild/lib -lchan
	$(CC) $(CFLAGS) -I$(build)/include -o $(BUILD)/examples/unbuffered $(EXAMPLES)/unbuffered.c -Lbuild/lib -lchan
	$(CC) $(CFLAGS) -I$(build)/include -o $(BUILD)/examples/close $(EXAMPLES)/close.c -Lbuild/lib -lchan
	$(CC) $(CFLAGS) -I$(build)/include -o $(BUILD)/examples/select $(EXAMPLES)/select.c -Lbuild/lib -lchan

install: all
	mkdir -p $(PREFIX)/include/chan
	mkdir -p $(PREFIX)/lib
	cp -f $(SRC)/chan.h $(PREFIX)/include/chan/chan.h
	cp -f $(SRC)/queue.h $(PREFIX)/include/chan/queue.h
	cp -f $(BUILD)/lib/libchan.a $(PREFIX)/lib/libchan.a

uninstall:
	rm -rf $(PREFIX)/include/chan/chan.h
	rm -rf $(PREFIX)/include/chan/queue.h
	rm -rf $(PREFIX)/lib/libchan.a

.PHONY: build check example clean install uninstall
