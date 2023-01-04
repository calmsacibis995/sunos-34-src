/*	@(#)funny.h 1.3 87/04/17 SMI	*/

/*
 *	Copyright (c) 1986 Sun Microsystems, Inc.
 */

/*
 *	This file contains all stuff that is architecture dependent
 */

/*
 *	This macro provides a *fast* (inline) string compare
 *
 *	Arguments
 *		string_a, string_b	The strings to compare
 *		length			The length of the strings
 *		exit_label		Goto this is strings differs
 *		cap, cbp		char * vars to use in macro
 */
#ifdef mc68020
#define compare_string(string_a, string_b, length, exit_label, cap, cbp) \
	{ register int len; \
	cap= (string_a); \
	cbp= (string_b); \
	if ((*cap != *cbp) && (length > 0)) \
		goto exit_label; \
	for (len= (length) / sizeof(int); --len >= 0;) \
		if (*((int *) cap)++ != *((int *) cbp)++) \
			goto exit_label; \
	if ((length) & sizeof(short)) \
		if (*((short *) cap) ++!= *((short *) cbp)++) \
			goto exit_label; \
	if ((length) & 0x1) \
		if (*cap != *cbp) \
			goto exit_label;}
#else
#define compare_string(string_a, string_b, length, exit_label, cap, cbp) \
	if (!is_equaln((string_a), (string_b), (length))) \
		goto exit_label;
#endif

/*
 *	This macro provides a *fast* (inline) string copy
 *
 *	Arguments
 *		destination		Pointer to destination string
 *		source			Pointer to source string
 *		length			The length of the strings
 *		cap, cbp		char * vars to use in macro
 */
#ifdef mc68020
#define copy_string(destination, source, length, cap, cbp) \
	{ register int len; \
	cap= (destination); \
	cbp= (source); \
	for (len= (length) / sizeof(int); --len >= 0;) \
		*((int *) cap)++= *((int *) cbp)++; \
	if ((length) & sizeof(short)) \
		*((short *) cap)++= *((short *) cbp)++; \
	if ((length) & 0x1) \
		*cap= *cbp;}
#else
#define copy_string(destination, source, length, cap, cbp) \
	(void)strncpy((destination), (source), (length));
#endif

/*
 *	This macro provides a *fast* (inline) string length
 *
 *	Arguments
 *		source			String to measure
 *		length			Length returned here
 *		cap			char * var to use in macro
 */
#ifdef mc68020
#define length_string(source, length, cap) \
	length= 0; \
	for (cap= (source); *cap != 0; (length)++, cap++)
#else
#define length_string(source, length, cap) \
	(length)= strlen(source)
#endif
