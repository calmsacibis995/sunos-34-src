/*	@(#)mtab.h 1.1 86/09/24 SMI; from UCB 4.4 83/05/28	*/

/*
 * Mounted device accounting file.
 */
struct mtab {
	char	m_path[32];		/* mounted on pathname */
	char	m_dname[32];		/* block device pathname */
	char	m_type[4];		/* read-only, quotas */
};
