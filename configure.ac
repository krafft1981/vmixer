
AC_PREREQ(2.59)
AC_INIT(vmixer, 1.0.0, krafft1981@mail.ru, vmixer, NONE)
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_HEADER([config.h])
AC_PROG_CC_STDC
AC_PROG_YACC
AC_PROG_LEX

# use libltld in future
AC_SEARCH_LIBS([dlopen], [dl dld], [], [AC_MSG_ERROR([unable to find the dlopen() function])])

AC_CHECK_LIB(pthread, pthread_create)

AC_CHECK_HEADERS(sys/socket.h sys/select.h)

#PKG_CHECK_MODULES(GSTREAMER, gstreamer-1.0 >= 1.4.0)
#AC_SUBST([GSTREAMER_CFLAGS])
#AC_SUBST([GSTREAMER_LIBS])

#PKG_CHECK_MODULES(GSTREAMER_SDP, gstreamer-sdp-1.0 >= 1.4.0)
#AC_SUBST([GSTREAMER_SDP_CFLAGS])
#AC_SUBST([GSTREAMER_SDP_LIBS])

#PKG_CHECK_MODULES(GSTREAMER_RTP, gstreamer-rtp-1.0 >= 1.4.0)
#AC_SUBST([GSTREAMER_RTP_CFLAGS])
#AC_SUBST([GSTREAMER_RTP_LIBS])

#PKG_CHECK_MODULES(SOFIA_SIP_UA, sofia-sip-ua >= 1.11.0)
#AC_SUBST(SOFIA_SIP_UA_LIBS)
#AC_SUBST(SOFIA_SIP_UA_CFLAGS)

AC_DEFINE(HEADER_MSG_SIZE, 4, Define io header msg size)
AC_DEFINE(IO_BUFFER_SIZE, 131072, Define io server buffer size)
AC_DEFINE(VALUE_LEN_MAX, 2048, max property value len)
AC_DEFINE(ENABLE_TCP, 1, enable tcp protocol)
AC_DEFINE(ENABLE_UDP, 1, enable udp protocol)
AC_DEFINE(ENABLE_SCTP, 1, enable sctp protocol)
#AC_DEFINE(ENABLE_UNIX, 1, enable unix protocol)

AC_CONFIG_FILES([
	Makefile
	include/Makefile
	src/Makefile
	Doxyfile
	vmixer.pc
])

AC_OUTPUT
