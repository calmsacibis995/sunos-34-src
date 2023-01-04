#
#	@(#)Makefile 1.1 86/09/25 (C) 1986 SMI
#
# Makefile for rpc protocol compiler
# Copyright (C) 1986, Sun Microsystems, Inc.
#
OBJS= rpc_main.o rpc_hout.o rpc_cout.o rpc_parse.o rpc_scan.o rpc_util.o \
	rpc_svcout.o
SRCS= rpc_main.c rpc_hout.c rpc_cout.c rpc_parse.c rpc_scan.c rpc_util.c \
	rpc_svcout.c
HDRS= rpc_util.h rpc_parse.h rpc_scan.h

GOAL=rpcgen
CFLAGS = -O
DESTDIR=

$(GOAL): $(OBJS) 
	cc $(CFLAGS) $(OBJS) -o $@

install: $(GOAL)
	install -s rpcgen $(DESTDIR)/usr/bin
	

$(GOAL).lint: $(SRCS) $(HDRS)
	lint $(SRCS) > $@

clean:
	rm -f $(GOAL) $(OBJS)

rpc_util.c: rpc_util.h rpc_scan.h
rpc_scan.c: rpc_util.h rpc_scan.h
rpc_parse.c: rpc_util.h rpc_scan.h rpc_parse.h
rpc_cout.c: rpc_util.h rpc_parse.h
rpc_hout.c: rpc_util.h rpc_parse.h
rpc_svcout.c: rpc_util.h rpc_parse.h
rpc_main.c: rpc_util.h rpc_parse.h rpc_scan.h

