#ifndef lint
static  char sccsid[] = "@(#)d2m_main.c 1.4 87/01/07 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
  */

  /*
   * defaults_to_mailrc -- convert defaults to mailrc
    */
# include <stdio.h>
# include <ctype.h>
# include <sys/time.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <sys/file.h>
# include <errno.h>
# include <sunwindow/rect.h>
# include <sunwindow/defaults.h>
# include "m2d_def.h"

# define ISENUM	"/$Enumeration"
# define D2M_SET	0
# define D2M_ALIAS	1

# define D2M_NOBRANCH	(-1)
# define D2M_OK		0
# define D2M_NOTUSER	1

static char	*pref = "/Mail/";
static char	fullpath[256];
extern	int	m2d_invoker;

static FILE	*make_temp_at_home();
static FILE	*temprc_fd;

#ifdef STANDALONE
main()
#else
d2m_main()
#endif
{
	char		*where, mailrc[1024], temprc[1024];
	FILE		*mailrc_fd;
	struct stat	statbuf;


	m2d_invoker = DEFAULTS_TO_MAILRC;
	if ((where = getenv("MAILRC")) != NULL)
		(void)sprintf(mailrc, "%s", where);
	else
		(void)sprintf(mailrc, "%s%s", getenv("HOME"), "/.mailrc");
	if ((mailrc_fd = fopen(mailrc, "r")) == (FILE *)NULL){
		fputs("defaults to mail conversion: cannot open .mailrc file", stderr);
		_exit(-1);
	}

	/* create a temporary file in the same directory as .mailrc file */
	/* it must in the same file system for rename to work later */
	/* having it in the same directory insures it is on the same fs */
	(void)stat(mailrc, &statbuf);
	if ((temprc_fd = make_temp_at_home(mailrc, temprc, (int)statbuf.st_mode)) == (FILE *)NULL){
		fputs("defaults to mail conversion: cannot create temporary file in home directory", stderr);
		_exit(-1);
	}

	/* add the set options to temporary file */
	get_sets_from_defaults(temprc_fd);
	get_aliases_from_defaults(temprc_fd);
	/* now add in the rest of the .mailrc file */
	d2m_commands(mailrc_fd);	/* eventually calls d2m_special_cmd for each line */

	(void)fclose(mailrc_fd);
	(void)fclose(temprc_fd);

	/* rename temporary file to .mailrc file */
	/* rename is necessary: is "guaranteed" to be atomic */
	if (rename(temprc, mailrc) != 0){
		fputs("defaults to mail conversion: cannot rename temp file", stderr);
		_exit(-1);
	}
	_exit(0);
}

d2m_special_cmd(line, known)
char	*line;
int	known;
{
	/* simply copy unrecognized lines */
	if (known == EX_UNKNOWN) {
		fputs(line, temprc_fd);
		fputs("\n", temprc_fd);
	}
}

static
get_sets_from_defaults(temprc_fd_local)
FILE	*temprc_fd_local;
{

	char		name[30], value[2048];
	int		boolean, status;

	status = get_first(D2M_SET, name, value, &boolean);
	for(; status != D2M_NOBRANCH; status = get_next(D2M_SET, name, value, &boolean)){
		if (name[0] == '$') continue;	/* special non-mail option */
		if (status == D2M_NOTUSER) continue; /* not set by user */
		if (strcmp(value, DEFAULTS_UNDEFINED) == 0) continue;
		if (boolean)			/* yes/no option */
			(void)fprintf(temprc_fd_local, "set %s%s\n", 
				(strcmp(value, "Yes") == 0) ? "" : "no", name);
		else if (isdigit(value[0]))	/* numeric option */
			(void)fprintf(temprc_fd_local, "set %s=%d\n", name, atoi(value));
		else				/* string valued option */
			(void)fprintf(temprc_fd_local, "set %s=%s\n", name, value);
	}
}

static
get_aliases_from_defaults(temprc_fd_local)
FILE	*temprc_fd_local;
{

	char		name[30], value[2048];
	int		boolean, status;

	status = get_first(D2M_ALIAS, name, value, &boolean);
	for(; status != D2M_NOBRANCH; status = get_next(D2M_ALIAS, name, value, &boolean)){
		if (name[0] == '$') continue;	/* special non-mail option */
		(void)fprintf(temprc_fd_local, "alias %s %s\n", name, value);
	}
}

static
get_first(type, name, value, boolean)
int		type;
char		*name, *value;
int		*boolean;
{
	char	*temp;

	(void)sprintf(fullpath, "%s%s", pref, (type == D2M_SET)?"Set":"Alias");
	temp = defaults_get_child(fullpath, (int *)NULL);
	return(get_value(type, temp, name, value, boolean));
}

static
get_next(type, name, value, boolean)
int		type;
char		*name, *value;
int		*boolean;
{
	char	*temp;

	temp = defaults_get_sibling(fullpath, (int *)NULL);
	return(get_value(type, temp, name, value, boolean));

}

static
get_value(type, branch, name, value, boolean)
int	type;
char	*branch, *name, *value;
int	*boolean;
{
	char	*temp;
	char	*main_temp;
	int	savelen;

	if (branch == NULL)
		return(D2M_NOBRANCH);
	(void)sprintf(fullpath, "%s%s/%s", pref, (type == D2M_SET)?"Set":"Alias", branch);
	(void)strcpy(name, branch);
	if (type == D2M_SET) {
		/* find out if a boolean option */
		savelen = strlen(fullpath);
		(void)strcat(fullpath, ISENUM);
		*boolean = (int) defaults_exists(fullpath, (int *)NULL);
		fullpath[savelen] = '\0';
	}
	/* get value of option */
	if (type == D2M_SET && *boolean)
		temp = defaults_get_boolean(fullpath, True, (int *)NULL)?"Yes":"No";
	else
		temp = defaults_get_string(fullpath, (char *)NULL, (int *)NULL);
	main_temp = defaults_get_default(fullpath, (char *)NULL, (int *)NULL);
	if (strcmp(temp, main_temp) == 0) {
		value[0] = '\0';
		return(D2M_NOTUSER);
	}
	else {
		(void)strcpy(value, temp);
		return(D2M_OK);
	}
}

/*
**	make_temp_at_home -- create a temp file in home directory
*/
static FILE *
make_temp_at_home(mailrc, name, mode)
char		*mailrc;
char		*name;
int		mode;
{
	struct timeval 	now;
	struct timezone	here;
	int		secs, i;
	char		mailrc_dir[1024];
	int		fd;

	(void)gettimeofday(&now, &here);
	secs = now.tv_sec;
	get_dir(mailrc, mailrc_dir);
	for (i = 0; i < 10; i++) {
		(void)sprintf(name, "%s/TEMP_MAILRC_%d", mailrc_dir, secs++);
		/* O_EXCL: if file name exists return error */
		if ((fd = open(name, O_CREAT | O_EXCL | O_WRONLY, mode)) > 0)
			return(fdopen(fd, "w"));
	}
	return(NULL);
}

static
get_dir(file_name, dir)
char	*file_name;
char	*dir;
{
	int	l;

	l = strlen(file_name);
	for (; l-- >= 0 && file_name[l] != '/'; );
	(void)strncpy(dir, file_name, l + 1);
}
