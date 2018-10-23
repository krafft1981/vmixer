
AM_YFLAGS		=	-d -v

sbin_PROGRAMS		=	vmixer
noinst_PROGRAMS		=	tester

vmixer_LDADD		=	$(LIBEV_LIBS)
vmixer_CFLAGS		=	$(LIBEV_CFLAGS) -I../include
vmixer_LDFLAGS		=	-rdynamic -fPIC -DPIC -s
vmixer_SOURCES		=	vmixer.c \
				logger.c \
				vector.c \
				rbtree.c \
				jsonlx.l \
				jsonpr.y \
				netrpc.c \
				thread.c \
				propes.c

tester_LDADD		=	$(LIBEV_LIBS)
tester_CFLAGS		=	$(LIBEV_CFLAGS) -I../include
tester_LDFLAGS		=	-rdynamic -fPIC -DPIC -s
tester_SOURCES		=	tester.c \
				logger.c \
				vector.c \
				rbtree.c \
				jsonlx.l \
				jsonpr.y \
				netrpc.c \
				thread.c \
				propes.c
