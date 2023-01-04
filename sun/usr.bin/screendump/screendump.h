/*      @(#)screendump.h 1.3 87/01/08 SMI      */

/*
 * Copyright 1984, 1986 by Sun Microsystems, Inc.
 */

/*
 * Common definitions for
 *	screendump, screenload, rastrepl, rasfilter8to1
 */

#include <stdio.h>
#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/memvar.h>
#include <pixrect/pr_util.h>
#include <pixrect/pr_io.h>
#include <rasterfile.h>

extern char *malloc();

extern char *optarg;
extern int getopt(), optind, opterr;

extern char *basename();

extern char *Progname;
extern void error();
