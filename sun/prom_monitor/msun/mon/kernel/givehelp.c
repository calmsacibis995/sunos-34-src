/*
 * @(#)givehelp.c 2.4 83/09/19 Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * give a little help
 * FIXME -- incredibly out of date.
 */

givehelp()
{
	printf("Commands:\n\
An,Dn,Mn,Pn,R - open A, D, SegMap, Page, or misc reg\n\
La,Ea,Oa - examine long, word, byte @a\n\
G [a] - go\n\
C - continue\n\
Z - set breakpoint\n\
T y/n/c cmd;cmd - trace yes/no/continuously\n\
U Uart/IO control; UT - transparent mode\n\
B dev file - bootstrap.  B? - possible devs\n\
Kn - various resets\n");
}
