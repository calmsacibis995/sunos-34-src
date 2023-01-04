/*	@(#) token.h 1.1 9/25/86 Copyright Sun Microsystems Inc	*/
#ifndef TOKEN_BUF_SIZE
#define TOKEN_BUF_SIZE	256
#endif

extern char	**token;
extern char	*tokens[TOKEN_BUF_SIZE];
extern char	*testname;

#define NULL		((char *) 0)
#define DEFAULT		'.'
#define FOREVER		'*'
#define SEPARATOR	';'

struct menu {
	char	t_char, *t_name;
	int	(*t_call)();
	char	*t_help;
};
