
AM_YFLAGS		=	-d -v

#noinst_PROGRAMS		=	tester sipuac
sbin_PROGRAMS		=	vmixer
bin_PROGRAMS		=	sender

#tester_LDADD		=	
#tester_CFLAGS		=	-I../include
#tester_LDFLAGS		=	-s
#tester_SOURCES		=	tester.c \
#				logger.c \
#				vector.c \
#				rbtree.c \
#				propes.c \
#				thread.c \
#				addres.c \
#				loader.c \
#				client.c \
#				jsonlx.l \
#				jsonpr.y \
#				crypto.c

sender_LDADD		=	
sender_CFLAGS		=	-I../include
sender_LDFLAGS		=	-s
sender_SOURCES		=	sender.c \
				logger.c \
				vector.c \
				rbtree.c \
				addres.c \
				client.c \
				jsonlx.l \
				jsonpr.y

#sipuac_LDADD		=	
#sipuac_CFLAGS		=	$(GSTREAMER_CFLAGS) $(GSTREAMER_RTP_CFLAGS) $(GSTREAMER_SDP_CFLAGS) $(SOFIA_SIP_UA_CFLAGS) -I../include
#sipuac_LDFLAGS		=	$(GSTREAMER_LIBS) $(GSTREAMER_RTP_LIBS) $(GSTREAMER_SDP_LIBS) $(SOFIA_SIP_UA_LIBS) -s
#sipuac_SOURCES		=	sipuac.c \
#				logger.c \
#				addres.c \
#				vector.c \
#				rbtree.c \
#				propes.c \
#				jsonlx.l \
#				jsonpr.y \
#				thread.c

vmixer_LDADD		=	
vmixer_CFLAGS		=	-I../include
vmixer_LDFLAGS		=	-rdynamic -fPIC -DPIC -s
vmixer_SOURCES		=	vmixer.c \
				logger.c \
				vector.c \
				rbtree.c \
				propes.c \
				thread.c \
				addres.c \
				loader.c \
				client.c \
				crypto.c \
				jsonlx.l \
				jsonpr.y
