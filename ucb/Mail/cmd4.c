#ifndef lint
static	char *sccsid = "@(#)cmd4.c 1.1 86/09/25 SMI"; /* from S5R2 1.1 */
#endif

#include "rcv.h"
#include <errno.h>

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * More commands..
 */

/*
 * pipe messages to cmd.
 */

dopipe(str)
	char str[];
{
	register int *ip, mesg;
	register struct message *mp;
	char *cp, *cmd;
	int f, *msgvec, lc, t, nowait=0;
	long cc;
	register int pid;
	int page, s, pivec[2];
	char *Shell;
	FILE *pio;

	msgvec = (int *) salloc((msgCount + 2) * sizeof *msgvec);
	if ((cmd = snarf(str, &f, 0)) == NOSTR) {
		if (f == -1) {
			printf("pipe command error\n");
			return(1);
			}
		if ( (cmd = value("cmd")) == NOSTR) {
			printf("\"cmd\" not set, ignored.\n");
			return(1);
			}
		}
	if (!f) {
		*msgvec = first(0, MMNORM);
		if (*msgvec == NULL) {
			printf("No messages to pipe.\n");
			return(1);
		}
		msgvec[1] = NULL;
	}
	if (f && getmsglist(str, msgvec, 0) < 0)
		return(1);
	if (*(cp=cmd+strlen(cmd)-1)=='&'){
		*cp=0;
		nowait++;
		}
	if ((cmd = expand(cmd)) == NOSTR)
		return(1);
	printf("Pipe to: \"%s\"\n", cmd);
	flush();

					/*  setup pipe */
	if (pipe(pivec) < 0) {
		perror("pipe");
		return(0);
	}

	if ((pid = vfork()) == 0) {
		close(pivec[1]);	/* child */
		close(0);
		dup(pivec[0]);
		close(pivec[0]);
		if ((Shell = value("SHELL")) == NOSTR || *Shell=='\0')
			Shell = SHELL;
		execlp(Shell, Shell, "-c", cmd, 0);
		perror(Shell);
		_exit(1);
	}
	if (pid == -1) {		/* error */
		perror("fork");
		close(pivec[0]);
		close(pivec[1]);
		return(0);
	}

	close(pivec[0]);		/* parent */
	pio=fdopen(pivec[1],"w");

					/* send all messages to cmd */
	page = (value("page")!=NOSTR);
	cc = 0L;
	lc = 0;
	for (ip = msgvec; *ip && ip-msgvec < msgCount; ip++) {
		mesg = *ip;
		touch(mesg);
		mp = &message[mesg-1];
		if ((t = msend(mp, pio, (int)value("alwaysignore"),
		     fputs)) < 0) {
			perror(cmd);
			return(1);
		}
		lc += t;
		cc += mp->m_size;
		if (page) putc('\f', pio);
	}

	fflush(pio);
	if (ferror(pio))
	      perror(cmd);
	fclose(pio);

					/* wait */
	if (!nowait){
		while (wait(&s) != pid);
		s &= 0377;
		if (s != 0) {
			printf("Pipe to \"%s\" failed\n", cmd);
			goto err;
		}
	}

	printf("\"%s\" %d/%ld\n", cmd, lc, cc);
	return(0);

err:
	return(0);
}

/*
 * Load the named message from the named file.
 */
loadmsg(str)
	char str[];
{
	char *file;
	int f, *msgvec;
	register int c, lastc = '\n';
	int blank;
	int lines;
	long ms;
	FILE *ibuf;
	struct message *mp;
	off_t fsize(), size;

	msgvec = (int *) salloc((msgCount + 2) * sizeof *msgvec);
	if ((file = snarf(str, &f, 1)) == NOSTR)
		return(1);
	if (f==-1)
		return(1);
	if (!f) {
		*msgvec = first(0, MMNORM);
		if (*msgvec == NULL) {
			printf("No message to load into.\n");
			return(1);
		}
		msgvec[1] = NULL;
	}
	if (f && getmsglist(str, msgvec, 0) < 0)
		return(1);
	if (msgvec[1] != NULL) {
		printf("Can only load into a single message.\n");
		return(1);
	}
	if ((file = expand(file)) == NOSTR)
		return(1);
	printf("\"%s\" ", file);
	flush();
	if ((ibuf = fopen(file, "r")) == NULL) {
		perror("");
		return(1);
	}
	mp = &message[*msgvec-1];
	mp->m_flag |= MODIFY;
	mp->m_flag &= ~MSAVED;		/* should probably turn off more */
	fseek(otf, (long) 0, 2);
	size = fsize(otf);
	mp->m_block = blockof(size);
	mp->m_offset = offsetof(size);
	ms = 0L;
	lines = 0;
	while ((c = getc(ibuf)) != EOF) {
		if (c == '\n') {
			lines++;
			blank = lastc == '\n';
		}
		lastc = c;
		putc(c, otf);
		if (ferror(otf))
			break;
		ms++;
	}
	if (!blank) {
		putc('\n', otf);
		ms++;
		lines++;
	}
	mp->m_size = ms;
	mp->m_lines = lines;
	if (ferror(otf))
		perror("/tmp");
	fclose(ibuf);
	printf("[Loaded] %d/%ld\n", lines, ms);
	return(0);
}
