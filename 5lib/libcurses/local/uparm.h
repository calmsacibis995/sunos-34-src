/*	@(#)uparm.h 1.1 86/09/24 SMI; from S5R2	*/

/*
 * Local configuration of various files.  Used if you can't put these
 * things in the standard places or aren't the super user, so you
 * don't have to modify the source files.  Thus, you can install updates
 * without having to re-localize your sources.
 */

/* Path to library files */
#ifdef S5EMUL
#define libpath(file) "/usr/5lib/file"
#else
#define libpath(file) "/usr/lib/file"
#endif

/* Path to local library files */
#define loclibpath(file) "/usr/local/lib/file"

/* Path to binaries */
#ifdef S5EMUL
#define binpath(file) "/usr/5bin/file"
#else
#define binpath(file) "/usr/bin/file"
#endif

/* Path to things under /usr (e.g. /usr/preserve) */
#define usrpath(file) "/usr/file"

/* Location of termcap file */
#define E_TERMCAP	"/etc/termcap"

/* Location of terminfo source file */
#define E_TERMINFO	"./terminfo.src"

/* Location of terminfo binary directory tree */
#ifdef S5EMUL
#define termpath(file)	"/usr/5lib/terminfo/file"
#else
#define termpath(file)	"/usr/lib/terminfo/file"
#endif

/* Location of the C shell */
#define B_CSH
