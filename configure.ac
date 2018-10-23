
AC_PREREQ(2.59)
AC_INIT(vmixer, 0.19.1, krafft1981@mail.ru, vmixer, http://google.ru)
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_HEADER([config.h])
AC_PROG_LIBTOOL
AC_PROG_CC_STDC
AC_PROG_YACC
AC_PROG_LEX

AC_CHECK_LIB(pthread, pthread_create)

AC_CHECK_HEADERS(sys/socket.h sys/select.h)

AC_CONFIG_FILES([
	Makefile
	include/Makefile
	src/Makefile
	vmixer.pc
])

AC_OUTPUT
