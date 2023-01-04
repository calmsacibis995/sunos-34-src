/*	uucpdefs.c	1.1	86/09/25	*/
	/*  @(#) uucpdefs.c 3.5@(#)  1/5/80  16:33:57  */
#include "uucp.h"

char Progname[10];
int Ifn, Ofn;
char Rmtname[32];
char User[32];
char Loginuser[32];
char Myname[32];
int Bspeed;
char Wrkdir[WKDSIZE];

char *Thisdir = THISDIR;
char *Spool = SPOOL;
int Debug = 0;
int Pkdebug = 0;
int Packflg = 0;
int Pkdrvon = 0;
long Retrytime;
short Usrf = 0;			/* Uustat global flag */
