#ifndef lint
static	char sccsid[] = "@(#)getpass.c 1.1 86/09/24 SMI"; /* from S5R2 1.4 */
#endif

/*LINTLIBRARY*/
#include <stdio.h>
#include <signal.h>
#include <termio.h>

extern void setbuf();
extern FILE *fopen();
extern int fclose(), fprintf(), findiop();
extern int kill(), ioctl(), getpid();
static int intrupt;

char *
getpass(prompt)
char	*prompt;
{
	struct termio ttyb;
	unsigned short flags;
	register char *p;
	register int c;
	FILE	*fi;
	static char pbuf[9];
	struct sigvec osv, sv;
	int	catch();

	if((fi = fopen("/dev/tty", "r")) == NULL)
		return((char*)NULL);
	else
		setbuf(fi, (char*)NULL);
	sv.sv_handler = catch;
	sv.sv_mask = 0;
	sv.sv_flags = SV_INTERRUPT;
	(void) sigvec(SIGINT, &sv, &osv);
	intrupt = 0;
	(void) ioctl(fileno(fi), TCGETA, &ttyb);
	flags = ttyb.c_lflag;
	ttyb.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL);
	(void) ioctl(fileno(fi), TCSETAF, &ttyb);
	(void) fputs(prompt, stderr);
	for(p=pbuf; !intrupt && (c = getc(fi)) != '\n' && c != EOF; ) {
		if(p < &pbuf[8])
			*p++ = c;
	}
	*p = '\0';
	(void) putc('\n', stderr);
	ttyb.c_lflag = flags;
	(void) ioctl(fileno(fi), TCSETA, &ttyb);
	(void) sigvec(SIGINT, &osv, (struct sigvec *)NULL);
	if(fi != stdin)
		(void) fclose(fi);
	if(intrupt)
		(void) kill(getpid(), SIGINT);
	return(pbuf);
}

static int
catch()
{
	++intrupt;
}
