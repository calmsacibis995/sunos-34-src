/*	@(#)lpc.h 1.1 86/09/25 SMI; from UCB X.X 05/11/83	*/

/*
 * Line printer control program.
 */
struct	cmd {
	char	*c_name;		/* command name */
	char	*c_help;		/* help message */
	int	(*c_handler)();		/* routine to do the work */
	int	c_priv;			/* privileged command */
};
