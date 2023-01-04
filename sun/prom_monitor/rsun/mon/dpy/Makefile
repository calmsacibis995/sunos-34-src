#
# @(#)Makefile 2.2 84/04/26 SMI
#
# (C) Copyright 1984 by Sun Microsystems, Inc.
#
# Makefile for Sun monitor /dpy directory
#
# By John Gilmore, 18Jun82.
#
# This makefile controls the making of  gallmash.c , the mashed-down
# font for the framebuffer terminal support.  It is made by compiling
# the program "makefont", which #include's the original unmashed font
# "gallant.c".  When run, makefont produces an output file which is a
# set of C variable declarations which define the mashed font.
#
# This makefile also permits the testing of the expand and compress
# algorithms.  "testexp.c" includes both the mashed and original
# fonts, expands the mashed one, and compares the two, printing
# differences to stdout.
#
test: testexp
	testexp

gallmash.c: gallant.c makefont 
	makefont gallmash.c

makefont: makefont.c gallant.c ../h/dpy.h
	cc -o makefont makefont.c

testexp: testexp.c expand.c gallmash.c gallant.c ../h/dpy.h
	cc -o testexp testexp.c expand.c

clean:
	rm -f testexp makefont *.o
