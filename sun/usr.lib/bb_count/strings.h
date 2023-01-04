/*	@(#)strings.h 1.1 86/09/25 SMI; from UCB X.X XX/XX/XX	*/

#define strdup(str)	strcpy(malloc(strlen(str) + 1), str)

char	*malloc();
char	*strcpy();
