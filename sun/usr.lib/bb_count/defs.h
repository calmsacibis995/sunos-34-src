/*	@(#)defs.h 1.1 86/09/25 SMI; from UCB X.X XX/XX/XX	*/

#ifndef __DEFS

enum boolean { False, True };

typedef enum	boolean Boolean;
typedef enum	control	Control;
typedef enum	token	Token;

char	*rindex();
char	*getident();
char	*filename();
Token	gettoken();
Boolean optional_decls();
Boolean	is_func();
Boolean is_type();
Boolean is_typedef();

#define __DEFS

#endif
