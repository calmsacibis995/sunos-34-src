/*	shio.c	1.1	86/09/25	*/
	/*  shio 3.4  1/30/80  17:44:43  */

#include "uucp.h"
#include <signal.h>


/*******
 *	shio(cmd, fi, fo, user)	execute shell of command with
 *	char *cmd, *fi, *fo;	fi and fo as standard input/output
 *	char *user;		user name
 *
 *	return codes:
 *		0  - ok
 *		non zero -  failed  -  status from child
 */

shio(cmd, fi, fo, user)
char *cmd, *fi, *fo, *user;
{
	int status, f;
	int uid, pid, ret;
	char path[MAXFULLNAME];

	if (fi == NULL)
		fi = "/dev/null";
	if (fo == NULL)
		fo = "/dev/null";

	DEBUG(3, "shio - %s\n", cmd);
	if ((pid = fork()) == 0) {
		signal(SIGINT, SIG_IGN);
		signal(SIGHUP, SIG_IGN);
		signal(SIGQUIT, SIG_IGN);
		signal(SIGKILL, SIG_IGN);
		close(Ifn);
		close(Ofn);
		close(0);
		if (user == NULL
		|| (gninfo(user, &uid, path) != 0)
		|| setuid(uid))
			setuid(getuid());
		f = open(fi, 0);
		if (f != 0)
			exit(f);
		close(1);
		f = creat(fo, 0666);
		if (f != 1)
			exit(f);
		execl(SHELL, "sh", "-c", cmd, 0);
		exit(100);
	}
	while ((ret = wait(&status)) != pid && ret != -1);
	DEBUG(3, "status %d\n", status);
	return(status);
}
