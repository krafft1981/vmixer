
AM_YFLAGS		=	-d -v

sbin_PROGRAMS		=	vmixer
check_PROGRAMS		=	tester

vmixer_LDADD		=	
vmixer_CFLAGS		=	 -I../include
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

tester_LDADD		=	
tester_CFLAGS		=	 -I../include
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
