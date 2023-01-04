/*	@(#)typedef.h 1.1 86/09/25 SMI; from UCB X.X XX/XX/XX	*/

/*
 * A linked list of typedef names.
 */
struct type_def {
	char	*type_name;			/* typedef name */
	struct	type_def *next;			/* linked list */
};
static	struct	type_def *typedef_hdr;		/* linked list of typedef's */
