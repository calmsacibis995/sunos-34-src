/*LINTLIBRARY*/
#ifndef lint
static	char	sccsid[]= "@(#)report.c 1.1 87/01/08 Copyr 1986 Sun Micro";
#endif lint

/*
 *	Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "report.h"
#include <sys/param.h>
#include <sys/wait.h>

static	FILE	*report_file;
static	char	*target_being_reported_for;

extern char	*alloca();
extern char	**environ;
extern char	*getenv();
extern char	*index();
extern char	*malloc();
extern char	*sprintf();
extern char	*strcpy();

FILE *
get_report_file()
{
	return(report_file);
}

char *
get_target_being_reported_for()
{
	return(target_being_reported_for);
}

static int
close_report_file()
{
	(void)fputs("\n", report_file);
	(void)fclose(report_file);
}

void
report_dependency(name)
	register char	*name;
{
	register char	*filename;
	char		buffer[MAXPATHLEN+1];
	register char	*p;
	register char	*p2;

	if (report_file == NULL) {
		if ((filename= getenv(SUNPRO_DEPENDENCIES)) == NULL) {
			report_file= (FILE *)-1; return;};
		(void)strcpy(buffer, name); name= buffer;
		p= index(filename, ' '); *p= 0;
		if ((report_file= fopen(filename, "a")) == NULL) {
			if ((report_file= fopen(filename, "w")) == NULL) {
				report_file= (FILE *)-1; return;};};
		(void)on_exit(close_report_file, (char *)report_file);
		if ((p2= index(p+1, ' ')) != NULL)
			*p2= 0;
		target_being_reported_for= (char *)malloc((unsigned)(strlen(p+1)+1));
		(void)strcpy(target_being_reported_for, p+1);
		(void)fputs(p+1, report_file); (void)fputs(":", report_file);
		*p= ' ';
		if (p2 != NULL)
			*p2= ' ';};
	if (report_file == (FILE *)-1)
		return;
	(void)fputs(name, report_file);
	(void)fputs(" ", report_file);
}

#ifdef MAKE_IT
void
make_it(filename)
	register char	*filename;
{
	register char	*command;
	register char	*argv[6];
	register int	pid;
	union wait	foo;

	if (getenv(SUNPRO_DEPENDENCIES) == NULL) return;
	command= alloca(strlen(filename)+32);
	(void)sprintf(command, "make %s\n", filename);
	switch (pid= fork()) {
		case 0: /* child */
			argv[0]= "csh";
			argv[1]= "-c";
			argv[2]= command;
			argv[3]= 0;			
			(void)dup2(2, 1);
			execve("/bin/sh", argv, environ);
			perror("execve error");
			exit(1);
		case -1: /* error */
			perror("Fork error");
		default: /* parent */
			while (wait(&foo) != pid);};
}
#endif
