/*	@(#)domain.h 1.1 86/09/25 SMI; from UCB 5.2 82/08/01	*/

/*
 * Structure per communications domain.
 */
struct	domain {
	int	dom_family;		/* AF_xxx */
	char	*dom_name;
	struct	protosw *dom_protosw, *dom_protoswNPROTOSW;
	struct	domain *dom_next;
};

#ifdef KERNEL
struct	domain *domains;
#endif
