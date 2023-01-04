/*	@(#)messages.h 1.1 86/09/24 SMI; from S5R2 1.4	*/

#ifdef FLEXNAMES
#	define	NUMMSGS	137
#else
#	define	NUMMSGS	136
#endif

#ifndef WERROR
#    define	WERROR	werror
#endif

#ifndef UERROR
#    define	UERROR	uerror
#endif

#ifndef MESSAGE
#    define	MESSAGE(x)	msgtext[ x ]
#endif

extern char	*msgtext[ ];
