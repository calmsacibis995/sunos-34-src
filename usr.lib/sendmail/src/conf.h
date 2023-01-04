/*
**  CONF.H -- All user-configurable parameters for sendmail
**
**	@(#)conf.h 1.1 86/09/25 SMI; from UCB 4.3 4/28/85
*/

/*
 * Reject messages to large mailing lists that have no body.
 */
# define RECIP_THRESHOLD	10		/* minimum recipients for ..*/
# define REJECT_MIN		10		/* minimum bytes in body */

/*
**  Table sizes, etc....
**	There shouldn't be much need to change these....
*/

# define MAXLINE	256		/* max line length */
# define MAXNAME	128		/* max length of a name */
# define MAXFIELD	2500		/* max total length of a hdr field */
# define MAXPV		40		/* max # of parms to mailers */
# define MAXHOP		15		/* max value of HopCount */
# define MAXATOM	100		/* max atoms per address */
# define MAXMAILERS	25		/* maximum mailers known to system */
# define MAXRWSETS	50		/* max # of sets of rewriting rules */
# define MAXPRIORITIES	25		/* max values for Precedence: field */
# define MAXTRUST	30		/* maximum number of trusted users */

/*
**  Compilation options.
*/

#define DBM		1	/* use DBM library (requires -ldbm) */
#define DEBUG		1	/* enable debugging */
#define LOG		1	/* enable logging */
#define SMTP		1	/* enable user and server SMTP */
#define QUEUE		1	/* enable queueing */
#define UGLYUUCP	1	/* output ugly UUCP From lines */
#define DAEMON		1	/* include the daemon (requires IPC) */
#define FLOCK		1	/* use flock file locking */

# define YELLOW		1	/* Call yellow pages for aliases */
# define ALIAS_MAP	"mail.aliases"	/* yp map for aliases */
# define FreezeMode 	0644	/* creation mode for Freeze file: */
				/* Must be public read if using NFS */
