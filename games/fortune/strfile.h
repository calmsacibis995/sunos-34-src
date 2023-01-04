/*	@(#)strfile.h 1.1 86/09/24 SMI; from UCB 1.2 05/14/81	*/

# define	MAXDELIMS	3

struct	strfile {		/* information table			*/
	int	str_numstr;		/* number of strings in the file */
	int	str_longlen;		/* length of longest string	*/
	int	str_shortlen;		/* length of shortest string	*/
	long	str_delims[MAXDELIMS];	/* delimiter markings		*/
	int	str_unused;		/* reserve space for later needs */
};

typedef struct strfile	STRFILE;
