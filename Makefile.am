LDADD = $(LIBS)

lib_LTLIBRARIES = libchan.la
libchan_la_SOURCES = src/chan.c src/queue.c
pkginclude_HEADERS = src/chan.h src/queue.h

check_PROGRAMS = src/chan_test
src_chan_test_SOURCES = src/chan_test.c
src_chan_test_LDADD = libchan.la
src_chan_test_LDFLAGS = -no-install

TESTS = src/chan_test

noinst_PROGRAMS = examples/buffered \
				  examples/close \
				  examples/select \
				  examples/unbuffered
examples_buffered_SOURCES = examples/buffered.c
examples_buffered_LDADD = libchan.la
examples_buffered_LDFLAGS = -no-install
examples_close_SOURCES = examples/close.c
examples_close_LDADD = libchan.la
examples_close_LDFLAGS = -no-install
examples_select_SOURCES = examples/select.c
examples_select_LDADD = libchan.la
examples_select_LDFLAGS = -no-install
examples_unbuffered_SOURCES = examples/unbuffered.c
examples_unbuffered_LDADD = libchan.la
examples_unbuffered_LDFLAGS = -no-install

