/*	@(#)pathname.h 1.1 86/09/25 SMI	*/

/*
 * Pathname structure.
 * System calls which operate on path names gather the
 * pathname from system call into this structure and reduce
 * it by peeling off translated components.  If a symbolic
 * link is encountered the new pathname to be translated
 * is also assembled in this structure.
 */
struct pathname {
	char	*pn_buf;		/* underlying storage */
	char	*pn_path;		/* remaining pathname */
	u_int	pn_pathlen;		/* remaining length */
};

#define	pn_peekchar(PNP)	((PNP)->pn_pathlen>0?*((PNP)->pn_path):0)
#define pn_pathleft(PNP)	((PNP)->pn_pathlen)

extern int	pn_alloc();		/* allocat buffer for pathname */
extern int	pn_get();		/* allocate buf and copy path into it */
#ifdef notneeded
extern int	pn_getchar();		/* get next pathname char */
#endif
extern int	pn_set();		/* set pathname to string */
extern int	pn_combine();		/* combine to pathnames (for symlink) */
extern int	pn_getcomponent();	/* get next component of pathname */
extern void	pn_skipslash();		/* skip over slashes */
extern void	pn_free();		/* free pathname buffer */
