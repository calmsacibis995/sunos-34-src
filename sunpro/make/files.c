#ifndef lint
static char sccsid[]= "@(#)files.c 1.3 87/04/17 Copyr 1986 Sun Micro";
#endif lint

/*
 *	Copyright (c) 1986 by Sun Microsystems, Inc	[Remotely from S5R2]
 */

#include "defs.h"
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/dir.h>
#include <errno.h>
#include <ctype.h>

/*
 *	vpath_exists() is called if exists() fails and there is a VPATH defined
 */
Timetype
vpath_exists(target)
	register Name		target;
{
	char			file_name[MAXPATHLEN];
	char			*name_p;
	char			*vpath;
	Name			alias;

	vpath= getvar(cached_names.vpath)->string;

	while (*vpath != NUL) {
		name_p= file_name;
		while ((*vpath != COLON) && (*vpath != NUL))
			*name_p++= *vpath++;
		*name_p++= SLASH;
		(void)strcpy(name_p, target->string);
		alias= getname(file_name, FIND_LENGTH);
		if (exists(alias) != FILE_DOESNT_EXIST) {
			target->stat.is_file= true;
			target->stat.mode= alias->stat.mode;
			target->stat.size= alias->stat.size;
			target->stat.is_dir= alias->stat.is_dir;
			target->stat.time= alias->stat.time;
			target->stat.is_sym_link= alias->stat.is_sym_link;
			maybe_append_prop(target, vpath_alias_prop)->
						body.vpath_alias.alias= alias;
			target->has_vpath_alias_prop= true;
			return(alias->stat.time);
		};
		while ((*vpath != NUL) && ((*vpath == COLON) || isspace(*vpath)))
			vpath++;
	};
	return(target->stat.time);
}

/*
 *	exists() calls stat() on one file.
 *	It uses lstat() first to check if the file is a symbolic link.
 */
Timetype
exists(target)
	register Name		target;
{
	struct	stat             buf;
	register int		result;

	/* We cache stat information */
	if (target->stat.time != FILE_NO_TIME)
		return(target->stat.time);

	/* If the target is a member we have to extract the time from the archive */
	if ((target->member_class != not_member) && (get_prop(target->prop, member_prop) != NULL))
		return(read_archive(target));

	if (debug_level > 1)
		(void)printf("%*sstat(%s)\n", BLANKS, target->string);

	result= lstat_vroot(target->string, &buf, NULL, VROOT_DEFAULT);
	if ((result != -1) && ((buf.st_mode & S_IFMT) == S_IFLNK)) {
		/* If the file is a symbolic link we remember that and then we */
		/* get the status for the refd file */
		target->stat.is_sym_link= true;
		result= stat_vroot(target->string, &buf, NULL, VROOT_DEFAULT);
	} else
		target->stat.is_sym_link= false;

	if (result < 0) {
		target->stat.time= FILE_DOESNT_EXIST;
		target->stat.errno = errno;
		if ((errno == ENOENT) &&
		    is_true(vpath_defined) &&
		    (index(target->string, SLASH) == NULL))
			return(vpath_exists(target));
	} else {
		/* Save all the information we need about the file */
		target->stat.errno = 0;
		target->stat.is_file= true;
		target->stat.mode= buf.st_mode & 0777;
		target->stat.size= buf.st_size;
		target->stat.is_dir= boolean((buf.st_mode & S_IFMT) == S_IFDIR);
		if (is_true(target->stat.is_dir))
			target->stat.time= FILE_IS_DIR;
		else
			target->stat.time= buf.st_mtime;
	};
	return(target->stat.time);
}

/*
 *	This is a regular shell type wildcard pattern matcher
 *	It is used when xpanding wildcards in dependency lists
 */
extern Boolean		amatch();

static Boolean
star_match(string, pattern)
	register char		*string;
	register char		*pattern;
{
	register int		pattern_ch;

	switch (*pattern) {
	    case 0:
		return(succeeded);
	    case BRACKETLEFT:
	    case QUESTION:
	    case ASTERISK:
		while (*string)
			if (amatch(string++, pattern))
				return(succeeded);
		break;
	    default:
		pattern_ch= *pattern++;
		while (*string)
			if ((*string++ == pattern_ch) && amatch(string, pattern))
				return(succeeded);
		break;
	};
	return(failed);
}

static                  Boolean
amatch(string, pattern)
	register char		*string;
	register char		*pattern;
{
	register int		string_ch;
	register int		k;
	register int		pattern_ch;
	register int		lower_bound;

top:
	for (; 1; pattern++, string++) {
		lower_bound= 077777;
		string_ch= *string;
		switch (pattern_ch= *pattern) {
		    case BRACKETLEFT:
			k= 0;
			while (pattern_ch= *++pattern)
				switch (pattern_ch) {
				    case BRACKETRIGHT:
					if (!k)
						return(failed);
					string++;
					pattern++;
					goto top;
				    case HYPHEN:
					k |= (lower_bound <= string_ch) &&
						(string_ch <= (pattern_ch= pattern[1]));
				    default:
					if (string_ch == (lower_bound= pattern_ch))
						k++;
				};
			return(failed);
		    case ASTERISK:
			return(star_match(string, ++pattern));
		    case 0:
			return(boolean(!string_ch));
		    case QUESTION:
			if (string_ch == 0)
				return(failed);
			break;
		    default:
			if (pattern_ch != string_ch)
				return(failed);
			break;
		};
	};
#ifdef lint
	return(failed);
#endif
}

Name
enter_file_name(name_string, library)
	char		*name_string;
	char		*library;
{
	char		buffer[STRING_BUFFER_LENGTH];
	String		lib_name;
	Name		name;
	Property	prop;

	if (library == NULL)
		return getname_fn(name_string, FIND_LENGTH, FILE_TYPE);

	init_string_from_stack(lib_name, buffer);
	append_string(library, &lib_name, FIND_LENGTH);
	append_char(PARENLEFT, &lib_name);
	append_string(name_string, &lib_name, FIND_LENGTH);
	append_char(PARENRIGHT, &lib_name);

	name= getname_fn(lib_name.buffer.start, FIND_LENGTH, FILE_TYPE);
	name->member_class= old_member;
	prop= maybe_append_prop(name, member_prop);
	prop->body.member.library= getname_fn(library, FIND_LENGTH, FILE_TYPE);
	prop->body.member.entry= NULL;
	prop->body.member.member= getname_fn(name_string, FIND_LENGTH, FILE_TYPE);
	return name;
}

/*
 *	read_dir() is used to enter the contents of directories into
 *	makes namespace.
 *	Precence of a file is important when scanning for implicit rules.
 *	read_dir() is also used to expand wildcards in dependency lists.
 */
void
read_dir(dir, pattern, line, library)
	Name			dir;
	char			*pattern;
	Property		line;
	char			*library;
{
	Name			file;
	char			file_name[MAXPATHLEN];
	char			*file_name_p= file_name;
	Name			plain_file;
	char			plain_file_name[MAXPATHLEN];
	char			*plain_file_name_p;
	register struct direct	*dp;
	DIR			*dir_fd;
	char			*vpath= NULL;
	char			*p;

	/* A directory is only read once unless we need to expand wildcards */
	if (pattern == NULL) {
		if (is_true(dir->has_read_dir))
			return;
		dir->has_read_dir= true;
	};
	/* Check if VPATH is active and setup list if it is */
	if (is_true(vpath_defined) && (dir == cached_names.dot)) {
		vpath= getvar(cached_names.vpath)->string;
	};

	/* Prepare the string where we build the full name of the files in the */
	/* directory */
	if ((dir->hash.length > 1) || (dir->string[0] != PERIOD)) {
		(void)strcpy(file_name, dir->string);
		(void)strcat(file_name, "/");
		file_name_p= file_name + strlen(file_name);
	};

	/* Open the directory */
    vpath_loop:
	dir_fd= opendir(dir->string);
	if (dir_fd == NULL)
		return;

	/* Read all the directory entries */
	while ((dp= readdir(dir_fd)) != NULL) {
		/* We ignore "." and ".." */
		if ((dp->d_fileno == 0) ||
		    ((dp->d_name[0] == PERIOD) &&
		     ((dp->d_name[1] == 0) ||
		      ((dp->d_name[1] == PERIOD) && (dp->d_name[2] == 0)))))
			continue;
		/* Build the full name of the file using whatever path supplied */
		/* to the function  */
		(void)strcpy(file_name_p, dp->d_name);
		file= enter_file_name(file_name, library);
		if ((pattern != NULL) && amatch(file_name, pattern)) {
			/* If we are expanding a wildcard pattern we enter the */
			/* file as a dependency for the target */
			if (debug_level > 0)
				(void)printf("'%s: %s' due to %s expansion\n",
					     line->body.line.target->string,
					     file->string, pattern);
			enter_dependency(line, file, false);
		} else
			/* If the file has an SCCS/s. file we will detect that */
			/* later on  */
			file->stat.has_sccs= no_sccs;
	};
	(void)closedir(dir_fd);
	if ((vpath != NULL) && (*vpath != NUL)) {
		while ((*vpath != NUL) && (isspace(*vpath) || (*vpath == COLON)))
			vpath++;
		p= vpath;
		while ((*vpath != COLON) && (*vpath != NUL))
			vpath++;
		if (vpath > p) {
			dir= getname(p, vpath - p);
			goto vpath_loop;
		};
	};

/* Now read the SCCS directory */
/* Files in the SCSC directory are considered to be part of the set of files in */
/* the plain directory. They are also entered in their own right */
/* Prepare the string where we build the true name of the SCCS files */
	(void)strncpy(plain_file_name, file_name, file_name_p - file_name);
	plain_file_name[file_name_p - file_name]= 0;
	plain_file_name_p= plain_file_name + strlen(plain_file_name);
	(void)strcpy(file_name_p, "SCCS");
	/* Internalize the constructed SCCS dir name */
	dir= getname(file_name, FIND_LENGTH);
	/* Just give up if the directory file doesnt exist. */
	if (is_false(dir->stat.is_file))
		return;
	/* Open the directory */
	dir_fd= opendir(dir->string);
	if (dir_fd == NULL)
		return;
	(void)strcat(file_name, "/");
	file_name_p= file_name + strlen(file_name);

	while ((dp= readdir(dir_fd)) != NULL) {
		if ((dp->d_fileno == 0) ||
		    ((dp->d_name[0] == PERIOD) &&
		     ((dp->d_name[1] == 0) ||
		      ((dp->d_name[1] == PERIOD) && (dp->d_name[2] == 0)))))
			continue;
		/* Construct and internalize the true name of the SCCS file */
		(void)strcpy(file_name_p, dp->d_name);
		file= getname_fn(file_name, FIND_LENGTH, FILE_TYPE);
		file->stat.has_sccs= no_sccs;
		/* If this is a s. file we also enter it as if it existed in */
		/* the plain directory */
		if ((dp->d_name[0] == 's') && (dp->d_name[1] == PERIOD)) {
			(void)strcpy(plain_file_name_p, dp->d_name + 2);
			plain_file= getname_fn(plain_file_name, FIND_LENGTH, FILE_TYPE);
			plain_file->stat.has_sccs= has_sccs;
			/* Enter the s. file as a dependency for the plain file */
			maybe_append_prop(plain_file, sccs_prop)->body.sccs.file= file;
			if ((pattern != NULL) && amatch(plain_file_name, pattern)) {
				if (debug_level > 0)
					(void)printf("'%s: %s' due to %s expansion\n",
						     line->body.line.target->string,
						     plain_file->string,
						     pattern);
				enter_dependency(line, plain_file, false);
			};
		};
	};
	(void)closedir(dir_fd);
}

void
lock_file(file_name)
	char		*file_name;
{
	char		lock_file[MAXPATHLEN];
	int		fd;
	int		timer = 0;

	(void)sprintf(lock_file, "%s.lock", file_name);
    try_again:
	if ((fd = open(lock_file, O_WRONLY|O_CREAT|O_EXCL, 0777)) == -1) {
		if (errno == EEXIST) {
			sleep(2);
			if (timer++ > 10) {
				warning("Waiting for lock on file `%s' to be lifted", file_name);
				timer = 0;
			};
			goto try_again;
		} else {
			fatal("Lock file %s could not be created", lock_file);
		};
	};
	(void)close(fd);
	lock_file_name = strcpy(Malloc(strlen(lock_file)+1), lock_file);
}
