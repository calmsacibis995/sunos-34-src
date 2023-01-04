#ifndef lint
static	char strtodsccsid[] = "@(#)strtod.c 1.1 86/09/24 SMI"; /* from S5R2 1.2 */
#endif

/*LINTLIBRARY*/
/*
 *	C library - ascii to floating (atof) and string to double (strtod)
 *
 *	This version compiles both atof and strtod depending on the value
 *	of STRTOD, which is set in the file and may be overridden on the
 *	"cc" command line.  The only difference is the storage of a pointer
 *	to the character which terminated the conversion.
 *	Long-integer accumulation is used, except on the PDP11, where
 *	"long" arithmetic is simulated, so floating-point is much faster.
 */
#ifndef STRTOD
#define STRTOD	1
#endif
#include "atof.c"
