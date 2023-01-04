/*	@(#)param.h 1.1 86/09/24 SMI	*/

/* Maximum number of digits in any integer (long) representation */
#define	MAXDIGS	11

/* Largest (normal length) positive integer */
#ifdef	vax
#define	MAXINT	2147483647
#endif
#ifdef	pdp11
#define	MAXINT	32767
#endif
#ifdef mc68000
#define	MAXINT	2147483647
#endif


/* A long with only the high-order bit turned on */
#define	HIBIT	0x80000000L

/* Convert a digit character to the corresponding number */
#define	tonumber(x)	((x)-'0')

/* Convert a number between 0 and 9 to the corresponding digit */
#define	todigit(x)	((x)+'0')

/* Data type for flags */
typedef	char	bool;

/* Maximum total number of digits in E format */
#define	MAXECVT	17

/* Maximum number of digits after decimal point in F format */
#define	MAXFCVT	60

/* Maximum significant figures in a floating-point number */
#define	MAXFSIG	17

/* Maximum number of characters in an exponent */
#define	MAXESIZ	4

/* Maximum (positive) exponent or greater */
#define	MAXEXP	40
