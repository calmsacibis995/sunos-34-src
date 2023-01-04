/*	uucpname.h	1.1	86/09/25	*/

/* define UNAME if uname() should be used to get uucpname */

/* define UUNAME if /usr/lib/uucp/SYSTEMNAME should be read to get uucpname */

/* define GETHOST if gethostname() should be used to get uucpname (4.2bsd) */
#define	GETHOST

/* define WHOAMI if <whoami.h> should be included to define system name */
