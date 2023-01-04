#ifndef lint
static	char sccsid[] = "@(#)passwd.c 1.1 86/09/24 SMI"; /* from UCB 4.4 82/07/10 */
#endif
/*
 * passwd
 *
 * Enter a password in the password file.
 * This program should be suid with an owner
 * with write permission on /etc/passwd.
 */
#include <sys/file.h>

#include <stdio.h>
#include <signal.h>
#include <pwd.h>
#include <errno.h>

char	*PASSWD = "/etc/passwd";
char	temp[]	 = "/etc/ptmp";
struct	passwd *pwd;
struct	passwd *getpwent();
int	endpwent();
char	*strcpy();
char	*crypt();
char	*getpass();
char	*getlogin();
char	*pw;
char	pwbuf[10];
char	hostname[256];
extern	int errno;

main(argc, argv)
	char *argv[];
{
	char *p;
	int i;
	char saltc[2];
	long salt;
	int u;
	int insist;
	int ok, flags;
	int c, pwlen, fd;
	FILE *tf;
	char *uname;

	insist = 0;
	uname = NULL;
	while (argc > 1) {
		if (argv[1][0] == '-') {
			if (argv[1][1] != 'f' || argc < 3) {
				usage();
				exit(1);
			}
			PASSWD = argv[2];
			argc--;
			argv++;
		}
		else {
			if (uname) {
				usage();
				exit(1);
			}
			uname = argv[1];
		}
		argc--;
		argv++;
	}
	if (uname == NULL) {
		if ((uname = getlogin()) == NULL) {
			usage();
			exit(1);
		}
		gethostname(hostname, sizeof(hostname));
		printf("Changing password for %s on %s\n", uname, hostname);
	}

	while (((pwd = getpwent()) != NULL) && strcmp(pwd->pw_name, uname))
		;
	u = getuid();
	if (pwd == NULL) {
		printf("Not in passwd file.\n");
		exit(1);
	}
	if (u != 0 && u != pwd->pw_uid) {
		printf("Permission denied.\n");
		exit(1);
	}
	endpwent();
	if (pwd->pw_passwd[0] && u != 0) {
		strcpy(pwbuf, getpass("Old password:"));
		pw = crypt(pwbuf, pwd->pw_passwd);
		if (strcmp(pw, pwd->pw_passwd) != 0) {
			printf("Sorry.\n");
			exit(1);
		}
	}
tryagain:
	strcpy(pwbuf, getpass("New password:"));
	pwlen = strlen(pwbuf);
	if (pwlen == 0) {
		printf("Password unchanged.\n");
		exit(1);
	}
	/*
	 * Insure password is of reasonable length and
	 * composition.  If we really wanted to make things
	 * sticky, we could check the dictionary for common
	 * words, but then things would really be slow.
	 */
	ok = 0;
	flags = 0;
	p = pwbuf;
	while (c = *p++) {
		if (c >= 'a' && c <= 'z')
			flags |= 2;
		else if (c >= 'A' && c <= 'Z')
			flags |= 4;
		else if (c >= '0' && c <= '9')
			flags |= 1;
		else
			flags |= 8;
	}
	if (flags >= 7 && pwlen >= 4)
		ok = 1;
	if ((flags == 2 || flags == 4) && pwlen >= 6)
		ok = 1;
	if ((flags == 3 || flags == 5 || flags == 6) && pwlen >= 5)
		ok = 1;
	if (!ok && insist < 2) {
		printf("Please use %s.\n", flags == 1 ?
			"at least one non-numeric character" :
			"a longer password");
		insist++;
		goto tryagain;
	}
	if (strcmp(pwbuf, getpass("Retype new password:")) != 0) {
		printf("Mismatch - password unchanged.\n");
		exit(1);
	}
	time(&salt);
	salt = 9 * getpid();
	saltc[0] = salt & 077;
	saltc[1] = (salt>>6) & 077;
	for (i = 0; i < 2; i++) {
		c = saltc[i] + '.';
		if (c > '9')
			c += 7;
		if (c > 'Z')
			c += 6;
		saltc[i] = c;
	}
	pw = crypt(pwbuf, saltc);
	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	(void) umask(0);
	fd = open(temp, O_WRONLY|O_CREAT|O_EXCL, 0644);
	if (fd < 0) {
		fprintf(stderr, "passwd: ");
		if (errno == EEXIST)
			fprintf(stderr, "password file busy - try again.\n");
		else
			perror(temp);
		exit(1);
	}
	signal(SIGTSTP, SIG_IGN);
	if ((tf = fdopen(fd, "w")) == NULL) {
		fprintf(stderr, "passwd: fdopen failed?\n");
		exit(1);
	}
	/*
	 * Copy passwd to temp, replacing matching lines
	 * with new password.
	 */
	while ((pwd = getpwent()) != NULL) {
		if (strcmp(pwd->pw_name,uname) == 0) {
			u = getuid();
			if (u && u != pwd->pw_uid) {
				fprintf(stderr, "passwd: permission denied.\n");
				unlink(temp);
				exit(1);
			}
			pwd->pw_passwd = pw;
			if (pwd->pw_gecos[0] == '*')
				pwd->pw_gecos++;
		}
		fprintf(tf,"%s:%s:%d:%d:%s:%s:%s\n",
			pwd->pw_name,
			pwd->pw_passwd,
			pwd->pw_uid,
			pwd->pw_gid,
			pwd->pw_gecos,
			pwd->pw_dir,
			pwd->pw_shell);
	}
	endpwent();
	if (rename(temp, PASSWD) < 0) {
		fprintf(stderr, "passwd: "); perror("rename");
		unlink(temp);
		exit(1);
	}
	fclose(tf);
	exit(0);
}

static char EMPTY[] = "";
static FILE *pwf = NULL;
static char line[BUFSIZ+1];
static struct passwd passwd;

setpwent()
{
	if( pwf == NULL )
		pwf = fopen( PASSWD, "r" );
	else
		rewind( pwf );
}

endpwent()
{
	if( pwf != NULL ){
		fclose( pwf );
		pwf = NULL;
	}
}

static char *
pwskip(p)
register char *p;
{
	while( *p && *p != ':' && *p != '\n' )
		++p;
	if( *p ) *p++ = 0;
	return(p);
}

struct passwd *
getpwent()
{
	register char *p;

	if (pwf == NULL) {
		if( (pwf = fopen( PASSWD, "r" )) == NULL )
			return(0);
	}
	p = fgets(line, BUFSIZ, pwf);
	if (p==NULL)
		return(0);
	passwd.pw_name = p;
	p = pwskip(p);
	passwd.pw_passwd = p;
	p = pwskip(p);
	passwd.pw_uid = atoi(p);
	p = pwskip(p);
	passwd.pw_gid = atoi(p);
	passwd.pw_quota = 0;
	passwd.pw_comment = EMPTY;
	p = pwskip(p);
	passwd.pw_gecos = p;
	p = pwskip(p);
	passwd.pw_dir = p;
	p = pwskip(p);
	passwd.pw_shell = p;
	while(*p && *p != '\n') p++;
	*p = '\0';
	return(&passwd);
}

usage()
{
	fprintf(stderr, "Usage: passwd [-f file] [user]\n");
}
