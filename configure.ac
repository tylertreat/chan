AC_INIT([chan], [0.0.4], [ttreat31@gmail.com], [chan], [https://github.com/tylertreat/chan])
AM_INIT_AUTOMAKE([foreign subdir-objects])
LT_INIT

CFLAGS+=" -std=c99 "
AC_PROG_CC
AC_CHECK_LIB([pthread], [pthread_mutex_init], [], [AC_MSG_ERROR([pthread not found])])
AC_CHECK_LIB([rt], [clock_gettime])

AC_OUTPUT([Makefile])

