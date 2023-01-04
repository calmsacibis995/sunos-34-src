#ifndef lint
static	char sccsid[] = "@(#)mail.c 1.4 87/01/07 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Mailtool - Mail subprocess handling
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "glob.h"
#include <sunwindow/sun.h>

char	*strcpy();

int	mt_mailpid;			/* pid of mail */
FILE	*mt_mailin, *mt_mailout;	/* input to mail, output from mail */

int	mt_curmsg;			/* number of current message */
int	mt_maxmsg;
int	mt_scandir;			/* scan direction, forward to start */
char	*mt_mailbox;			/* name of user's mailbox [256] */
char	*mt_folder;			/* name of current folder [256] */
char	*mt_info;			/* info from save, file, etc. [256] */
static	char *line;		/* tmp line buffer, used everywhere [LINE_SIZE] */
#define	LINE_SIZE	1024
static	char *strtab;		/* string table for folder names [15*MAXFOLDERS] */
static	char **folder_list;	/* pointers to folder names [MAXFOLDERS+1] */

enum	mstate { S_UNKNOWN, S_NOMAIL, S_MAIL } mt_mstate;

struct	msg *mt_message;			/* [MAXMSGS] */

char	*mt_mail_get_line();

/*
 * Dynamically allocate all the large arrays for mailtool
 * so that when we are bound with suntools (using toolmerge)
 * we don't take up a lot of data space that will not be used
 * by the other tools merged into suntools.
 */
mt_init_mail_storage()
{
	extern	char * calloc();

	mt_message = (struct msg *)(LINT_CAST(calloc(MAXMSGS, sizeof (struct msg))));
	strtab = calloc(15*MAXFOLDERS, sizeof (char));
	line = calloc(LINE_SIZE, sizeof (char));
	folder_list = (char **)(LINT_CAST(calloc(MAXFOLDERS+1, sizeof (char *))));
	mt_mailbox = calloc(256, sizeof (char));
	mt_folder = calloc(256, sizeof (char));
	mt_info = calloc(256, sizeof (char));
	mt_scandir = 1;
	mt_mstate = S_UNKNOWN;
}

mt_release_mail_storage() {} /* No-op for now */

/*
 * Start the Mail subprocess.
 */
mt_start_mail()
{
	int in[2], out[2];
	int i;
	FILE *f;
	static char *args[6] = {"Mail","-N", "-B", "-f", 0, 0};

	f = fopen(mt_dummybox, "w");
	(void)chmod(mt_dummybox, 0600);
	args[4] = mt_dummybox;
	putc('\n', f);
	(void)fclose(f);
	(void)pipe(in);	/* input to mail */
	(void)pipe(out);	/* output from mail */
	if ((mt_mailpid = vfork()) == 0) {
		(void)dup2(in[0], 0);
		(void)dup2(out[1], 1);
		(void)dup2(out[1], 2);
		for (i = getdtablesize(); i > 2; i--)
			(void)close(i);
		execvp("Mail", args, 0);
		exit(-1);
	}
	(void)close(in[0]);
	(void)close(out[1]);
	mt_mailin = fdopen(in[1], "w");
	mt_mailout = fdopen(out[0], "r");
	(void)strcpy(mt_folder, "[None]");
}

mt_idle_mail()
{
	FILE *f;

	f = fopen(mt_dummybox, "w");
	(void)chmod(mt_dummybox, 0600);
	putc('\n', f);
	(void)fclose(f);
	(void) mt_set_folder(mt_dummybox);
}

mt_stop_mail(doabort)
	int doabort;
{

	mt_mail_cmd(doabort ? "x" : "quit");
	(void)fclose(mt_mailin);
	(void)fclose(mt_mailout);
	(void)unlink(mt_dummybox);
	/* XXX - should wait for process to die? */
}

/*
 * Flag new mail when it arrives and check to see if the
 * mail has been seen but not necessarily read or deleted.
 * After mail is seen the flag goes down.
 */
mt_check_mail_box()
{
	struct stat stat_buf;
	static time_t mbox_time;	/* time mailbox last changed state */

	/*
	 * Stat the mailbox, if it doesn't exist then we don't have any mail.
	 */
	if (stat(mt_mailbox, &stat_buf) < 0) {
		mt_set_state(S_NOMAIL);
		return;
	}

	/*
	 * If the mailbox has been accessed (read) but not modified (written)
	 * since the last time we have checked, or is empty, then there is
	 * no new mail.  Update mbox_time to the mailbox last accessed time.
	 */
	if ((stat_buf.st_atime > stat_buf.st_mtime &&
	    stat_buf.st_atime > mbox_time) || stat_buf.st_size == 0) {
		mbox_time = stat_buf.st_atime;
		mt_set_state(S_NOMAIL);
		return;
	}

	/*
	 * If the mailbox size is non-zero (it has some mail in it) and
	 * has been modified (written) since the last time we checked then
	 * there is new mail.
	 * Update mbox_time to the mailbox last modified time.
	 */
	if (stat_buf.st_mtime > mbox_time) {
		mbox_time = stat_buf.st_mtime;
		mt_set_state(S_MAIL);
		return;
	}

	/*
	 * If none of the above, nothing has changed.
	 */
}

/*
 * Set the icon and window label to reflect the
 * internal state (mail or no mail).
 */
mt_set_state(s)
	enum mstate s;
{
	char *new;

	if (mt_mstate == s)
		return;
	mt_mstate = s;
	switch (mt_mstate) {
	case S_UNKNOWN:
		mt_icon_ptr = &mt_unknown_icon;
		new = "[UNKNOWN!]";
		break;
	case S_NOMAIL:
		mt_icon_ptr = &mt_nomail_icon;
		new = "";
		break;
	case S_MAIL:
		mt_icon_ptr = &mt_mail_icon;
		new = "[New Mail]";
		mt_announce_mail();
		break;
	}
	mt_set_icon(mt_icon_ptr);
	mt_set_namestripe(mt_folder, new);
}

/*
 * Get the headers for all messages from the Mail subprocess.
 */
mt_get_headers()
{
	int n = 0;
	register struct msg *mp;

	mt_mail_start_cmd("from 1-$");
	while (mt_mail_get_line()) {
		n = atoi(&line[2]);
		if (n >= MAXMSGS) {
			mt_confirm("Maximum number of messages exceeded. Split this folder.");
			n--;
			while (mt_mail_get_line()) {};
			break;
		}
		/* if this message is the current message, remember it */
		if (line[0] == '>') {
			mt_curmsg = n;
			line[0] = ' ';
		}
		mp = &mt_message[n];
		if (mp->m_header)
			free(mp->m_header);
		mp->m_header = mt_savestr(line);
		mp->m_next = NULL;
		mp->m_deleted = 0;
	}
	mt_maxmsg = n;
	mt_message[mt_maxmsg+1].m_deleted = 1;
}

/*
 * Get the name of the current folder from Mail.
 */
mt_get_folder()
{
	char *p;
	extern char *index();

	mt_mail_cmd("folder");
	if (mt_info[0] == '"' && (p = index(&mt_info[1], '"'))) {
		*p = '\0';
		(void)strcpy(mt_folder, &mt_info[1]);
	}
}

/*
 * Get the names of all the user's folders.
 * XXX - SHOULD CHECK FOR OVERFLOW OF strtab.
 */
char **
mt_get_folder_list(acp)
	int *acp;
{
	char *p, *s;
	char **av;
	int ac, n = 0;

	/* reserve the first slot for use by the folder menu code */
	av = &folder_list[1];
	ac = 0;
	p = strtab;
	*p = '\0';
	mt_mail_start_cmd("folders");
	while (mt_mail_get_line()) {
		n++;
		if (n > MAXFOLDERS) {
			mt_confirm("Maximum number of folders exceeded. Combine several folders.");
			while (mt_mail_get_line()) {};
			break;
		}
		*av++ = p;
		ac++;
		*p++ = '+';
		for (s = line; *s != '\n';)
			*p++ = *s++;
		*p++ = '\0';
	}
	*av = NULL;
	*acp = ac;
	return (&folder_list[1]);
}

/*
 * Get the variables (and their values) that Mail knows about.
 */
mt_get_vars()
{
	char *p, *s;

	mt_mail_start_cmd("set");
	while (mt_mail_get_line()) {
		s = &line[strlen(line) - 1];
		*s-- = '\0';
		if ((p = index(line, '=')) != NULL) {
			*p++ = '\0';
			if (*p == '"' && *s == '"') {
				p++;
				*s = '\0';
			}
			mt_assign(line, p);
		} else
			mt_assign(line, "");
	}
}

/*
 * Set (or unset) a Mail variable.
 */
mt_set_var(var, val)
	char *var, *val;
{

	if (val == 0)
		mt_mail_cmd("unset %s", var);
	else if (strlen(val) == 0)
		mt_mail_cmd("set %s", var);
	else
		mt_mail_cmd("set %s=\"%s\"", var, val);
}

/*
 * Get Mail's working directory.
 */
mt_get_mailwd(dir)
	char *dir;
{

	mt_mail_cmd("!pwd");
	mt_info[strlen(mt_info) - 1] = '\0';
	(void)strcpy(dir, mt_info);
}

/*
 * Set Mail's working directory.
 */
mt_set_mailwd(dir)
	char *dir;
{

	mt_mail_cmd("cd %s", dir);
}

/*
 * Reply to the specified message.
 * Put reply in "file".
 * If "all", reply to all recipients.
 * If "orig", include original message.
 */
mt_reply_msg(msg, file, all, orig)
	int msg;
	char *file;
	int all;
	int orig;
{
	FILE *replyf;
	char *escp;

	if ((escp = mt_value("escape")) == (char *)NULL)
		escp = "~";
	replyf = fopen(file, "w");
	(void)chmod(file, 0600);
	(void)fprintf(mt_mailin, "%s %d\n", all ? "Reply" : "reply", msg);
	if (orig)
		(void)fprintf(mt_mailin, "%cm\n", *escp);
	(void)fprintf(mt_mailin, "%cp\n%cq\n", *escp, *escp);
	(void)fflush(mt_mailin);
	while (fgets(line, LINE_SIZE,  mt_mailout))
		if (strcmp(line, "Message contains:\n") == 0)
			break;
	while (fgets(line, LINE_SIZE, mt_mailout)) {
		if (strcmp(line, "(continue)\n") == 0)
			break;
		fputs(line, replyf);
	}
	while (fgets(line, LINE_SIZE, mt_mailout))
		if (strcmp(line, "Interrupt\n") == 0)
			break;
	(void)fclose(replyf);
}

/*
 * Send a message.
 * Put in all blank fields if "all" set.
 * Include original message if "orig" set.
 */
mt_send_msg(msg, file, all, orig)
	int msg;
	char *file;
	int all, orig;
{
	FILE *replyf;

	replyf = fopen(file, "w");
	(void)chmod(file, 0600);
	(void)fprintf(replyf, "To: \n");
	if (mt_value("asksub"))
		(void)fprintf(replyf, "Subject: \n");
	if (mt_value("askcc") || all)
		(void)fprintf(replyf, "Cc: \n");
	(void)fprintf(replyf, "\n");
	if (orig) {
		(void)fprintf(replyf, "\n\n----- Begin Forwarded Message -----\n\n");
		(void)fclose(replyf);
		mt_mail_cmd("copy %d %s", msg, file);
		replyf = fopen(file, "a");
		(void)fprintf(replyf, "\n----- End Forwarded Message -----\n");
	}
	(void)fclose(replyf);
}

/*
 * Reload the specified message from the specified file.
 */
mt_load_msg(msg, file)
	int msg;
	char *file;
{

	mt_mail_cmd("load %d %s", msg, file);
}

/*
 * Save a message in a file or folder.
 * Return 0 on failure, 1 on success.
 */
mt_copy_msg(msg, file)
	int msg;
	char *file;
{
	register char *p;
	extern char *index();

	mt_mail_cmd("copy %d %s", msg, file);

	/* look for '"file" [Appended]'  or '"file" [New file]' */
	if ((p = index(mt_info, '[')) && ((strncmp(++p, "Appended", 8) == 0) ||
					(strncmp(p, "New file", 8) == 0)))
		return (1);
	  else
		return (0);
}

/*
 * Print a message.
 */
mt_print_msg(msg, file, ign)
	int msg;
	char *file;
	int ign;
{

	if (ign) {
		/*
		 * If "alwaysignore" isn't set, turn it on and off
		 * around the "copy" command so it will have the
		 * same effect as the "print" command.
		 */
		if (!mt_value("alwaysignore"))
			mt_mail_cmd(
			    "set alwaysignore\ncopy %d %s\nunset alwaysignore",
			    msg, file);
		else
			mt_mail_cmd("copy %d %s", msg, file);
	} else {
		/*
		 * If "alwaysignore" is set, turn it off and on
		 * around the "copy" command so it will have the
		 * same effect as the "Print" command.
		 */
		if (mt_value("alwaysignore"))
			mt_mail_cmd(
			    "unset alwaysignore\ncopy %d %s\nset alwaysignore",
			    msg, file);
		else
			mt_mail_cmd("copy %d %s", msg, file);
	}
	(void)chmod(file, 0600);
}

/*
 * Preserve the specified message.
 */
mt_pre_msg(msg)
	int msg;
{

	mt_mail_cmd("preserve %d", msg);
}

/*
 * Delete the specified message.
 */
mt_del_msg(msg)
	int msg;
{
	int i, len;

	(void)unlink(mt_msgfile);
	mt_mail_cmd("delete %d", msg);
	mt_message[msg].m_deleted = 1;
	len = mt_message[msg+1].m_start - mt_message[msg].m_start;
	for (i = msg + 1; i <= mt_maxmsg + 1; i++)
		mt_message[i].m_start -= len;
}

/*
 * Undelete the specified message.
 */
mt_undel_msg(msg)
	int msg;
{
	int i, len;

	mt_mail_cmd("undelete %d", msg);
	mt_message[msg].m_deleted = 0;
	len = strlen(mt_message[msg].m_header);
	for (i = msg + 1; i <= mt_maxmsg + 1; i++)
		mt_message[i].m_start += len;
}

/*
 * Switch Mail to the specified folder.
 * Return number of messages in folder, -1 on failure.
 */
mt_set_folder(file)
	char *file;
{
	register char *p;
	int n = -1;

	mt_mail_start_cmd("file %s", file);
	while (mt_mail_get_line()) {
		/* look for '"file" complete' */
		if ((p = index(line, '\0') - strlen("complete\n")) >= line &&
		    strcmp(p, "complete\n") == 0)
			continue;
		/* look for '"file" removed' */
		if ((p = index(line, '\0') - strlen("removed\n")) >= line &&
		    strcmp(p, "removed\n") == 0)
			continue;
		/* look for '"file": n messages' */
		if (line[0] == '"' && (p = index(&line[1], '"')) && *++p == ':')
			n = atoi(++p);
		(void)strcpy(mt_info, line);
	}
	return (n);
}

/*
 * Find the next message after msg.  If none after,
 * use specified message if not deleted.  Otherwise,
 * find first one before msg.  If none, return 0.
 */
mt_next_msg(msg)
	int msg;
{
	register int i;

	for (i = msg + mt_scandir; i <= mt_maxmsg && i > 0; i += mt_scandir)
		if (!mt_message[i].m_deleted)
			return (i);
	if (!mt_message[msg].m_deleted && msg != mt_curmsg)
		return (msg);
	for (i = msg - mt_scandir; i <= mt_maxmsg && i > 0; i -= mt_scandir)
		if (!mt_message[i].m_deleted) {
			if (mt_value("allowreversescan"))
				mt_scandir = -mt_scandir;
			return (i);
		}
	return (0);
}

/*
 * Delete a folder.
 */
mt_del_folder(file)
	char *file;
{

	mt_mail_cmd("echo %s", file);
	mt_info[strlen(mt_info) - 1] = '\0';
	(void)unlink(mt_info);
}

/*
 * Do a simple Mail command.
 */
/* VARARGS1 */
mt_mail_cmd(cmd, a1, a2, a3)
	char *cmd, *a1, *a2, *a3;
{
	register int first = 1;

	(void)fprintf(mt_mailin, cmd, a1, a2, a3);
	(void)fprintf(mt_mailin, "\necho \004\n");
	(void)fflush(mt_mailin);
	while (fgets(line, LINE_SIZE, mt_mailout)) {
		if (strcmp(line, "\004\n") == 0)
			break;
		if (first) {
			(void)strcpy(mt_info, line);
			first = 0;
		}
	}
}

/*
 * Start a long Mail command.
 */
/* VARARGS1 */
mt_mail_start_cmd(cmd,a1,a2,a3)
	char *cmd, *a1, *a2, *a3;
{
	
	(void)fprintf(mt_mailin, cmd, a1, a2, a3);
	(void)fprintf(mt_mailin, "\necho \004\n");
	(void)fflush(mt_mailin);
}

/*
 * Read one line of the response from a long
 * Mail command, checking for end of output.
 */
char *
mt_mail_get_line()
{

	if (fgets(line, LINE_SIZE, mt_mailout) == NULL)
		return (NULL);
	if (strcmp(line, "\004\n") == 0)
		return (NULL);
	return (line);
}
