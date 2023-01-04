#ifndef lint
static  char sccsid[] = "@(#)ttysw_mapkey.c 1.7 87/01/07  Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <ctype.h>
#include <sys/file.h>
#include <sundev/kbd.h>
#include <suntool/tool_hs.h>
#include <suntool/selection_svc.h>
#include <suntool/ttysw.h>
#include "ttysw_impl.h"
#include "ttytlsw_impl.h"

extern	char	* strcpy();
extern	char	* strcat();
extern  char	* tool_get_attribute();

/*	static routines	*/

static char	*str_index(),
		*savestr(),
		*tdecode();

void	 	ttysw_display_capslock();
static void	 ttysw_add_caps();
static void	 ttysw_remove_caps();
/*
 * Read rc file.
 */
ttysw_readrc(ttysw)
    struct ttysubwindow *ttysw;
{
    char               *p;
    extern char        *getlogindir();
    char                rc[1025];
    FILE               *fp;
    char                line[1025], *av[4];
    int                 i, lineno = 0;

    if ((p = getlogindir()) == (char *) NULL)
	return;
    (void) strcpy(rc, p);
    (void) strcat(rc, "/.ttyswrc");
    if ((fp = fopen(rc, "r")) == (FILE *) NULL)
		return;
		
    while (fgets(line, sizeof(line), fp)) {
		register char *t;
		
		lineno++;
		if (line[strlen(line)-1] != '\n') {
			register char c;
			
			(void)printf("%s: line %d longer than 1024 characters\n", rc, lineno);
			while ((c = fgetc(fp)) != '\n' && c != EOF);
			continue;
		}
		for (t = line; isspace(*t); t++);
		if (*t == '#' || *t == '\0')
	    	continue;
	    	
		for (i = 0; i < 2; i++) {
			av[i] = t;
			while (!isspace(*t) && *t) t++;
			if (!*t)
				break;
			else
				*t++ = '\0';
			while (isspace(*t) && *t) t++;
			if (!*t) break;
		}
		if (*t) {
			i = 2;
			av[2] = t;
			av[2][strlen(av[2])-1] = '\0';
		}
			
		if (i == 2 && strcmp(av[0], "mapi") == 0)
	 		(void)ttysw_mapkey(ttysw, av[1], av[2], 0);
		else if (i == 2 && strcmp(av[0], "mapo") == 0)
	   		(void)ttysw_mapkey(ttysw, av[1], av[2], 1);
		else if (i == 1 && strcmp(av[0], "set") == 0)
	   		(void)ttysw_doset(ttysw, av[1]);
		else
	   		(void)printf("%s: unknown command on line %d\n", rc, lineno);
    }
    (void) fclose(fp);
}

ttysw_doset(ttysw, var)
    struct ttysubwindow *ttysw;
    char               *var;
{

    /* XXX - for now */
    if (strcmp(var, "pagemode") == 0)
	(void)ttysw_setopt((caddr_t) ttysw, TTYOPT_PAGEMODE, 1);
#ifdef TTYHIST
    else if (strcmp(var, "history") == 0)
	(void)ttysw_setopt(ttysw, TTYOPT_DOHIST, 1);
#endif
}

ttysw_mapkey(ttysw, key, to, output)
    struct ttysubwindow *ttysw;
    char               *key, *to;
    int                 output;
{
    int                 k;

    if ((k = ttysw_strtokey(key)) == -1)
	return (-1);
    ttysw->ttysw_kmtp->kmt_key = k;
    ttysw->ttysw_kmtp->kmt_output = output;
    ttysw->ttysw_kmtp->kmt_to = savestr(tdecode(to, to));
    ttysw->ttysw_kmtp++;
    return (k);
}

ttysw_mapsetim(ttysw)
    struct ttysubwindow *ttysw;
{
    struct keymaptab   *kmt;
    struct inputmask    imask;

    /*  Update kbd input mask */
    (void)win_get_kbd_mask(ttysw->ttysw_wfd, &imask);
    /*  Added user defined mappings to kbd mask.  Note: this wouldn't always
     *  be the right thing for escape sequence generated events like TOP.
     */
    for (kmt = ttysw->ttysw_kmt; kmt < ttysw->ttysw_kmtp; kmt++)
	win_setinputcodebit(&imask, kmt->kmt_key);
    /*  Add in the keyboark related standard function keys.
     *  Note these are overridden by any user-defined mappings of these keys.
     */
    win_setinputcodebit(&imask, KEY_PUT);
    win_setinputcodebit(&imask, KEY_GET);
    win_setinputcodebit(&imask, KEY_FIND);
    win_setinputcodebit(&imask, KEY_DELETE);
    win_setinputcodebit(&imask, KEY_CAPS);
    imask.im_flags |= IM_NEGEVENT;
    (void)win_set_kbd_mask(ttysw->ttysw_wfd, &imask);
    /*  Update pick input mask */
    (void)win_get_pick_mask(ttysw->ttysw_wfd, &imask);
    win_setinputcodebit(&imask, KEY_FRONT);
    win_setinputcodebit(&imask, KEY_CLOSE);
    imask.im_flags |= IM_NEGEVENT;
    (void)win_set_pick_mask(ttysw->ttysw_wfd, &imask);
}

ttysw_domap(ttysw, ie)
    struct ttysubwindow  *ttysw;
    struct inputevent    *ie;
{
    int                   key;
    struct keymaptab     *kmt;
    int                   len;

    extern Notify_value		tool_input();
    struct ttytoolsubwindow	*ttytlsw;

    key = ie->ie_code;
    /*
     * The following switch is required to enforce the portion of the default
     * user interface that MUST coordinate with Selection Service. 
     */
    switch (key) {
      case KEY_DELETE:
      case KEY_FIND:
      case KEY_GET:
      case KEY_PUT:
	if (win_inputposevent(ie) && ie->ie_code == KEY_GET) {
	    ttysw->ttysw_caret.sel_made = FALSE;    /* backstop: be sure  */
	    ttysel_acquire(ttysw, SELN_CARET);	    /* to go to service  */
	}
	if (ttysw->ttysw_seln_client != (char *) NULL) {
	    seln_report_event(ttysw->ttysw_seln_client, ie);
	}
	return TTY_DONE;

      case KEY_CLOSE:
      case KEY_FRONT:
        ttytlsw = (struct ttytoolsubwindow *) LINT_CAST(ttysw->ttysw_client);
        (void)tool_input(ttytlsw->tool, ie, (Notify_arg)0, NOTIFY_SAFE);
	return TTY_DONE;

      default:
	break;
    }
    if (win_inputposevent(ie)) {
	for (kmt = ttysw->ttysw_kmt; kmt < ttysw->ttysw_kmtp; kmt++) {
	    if (kmt->kmt_key == key) {
		len = strlen(kmt->kmt_to);
		if (kmt->kmt_output)
		    (void) ttysw_output((Ttysubwindow)(LINT_CAST(
		    	ttysw)), kmt->kmt_to, len);
		else
		    (void) ttysw_input((caddr_t) ttysw, kmt->kmt_to, len);
		return (TTY_DONE);
	    }
	}
	if (key == KEY_CAPS) {
	    ttysw->ttysw_capslocked = !ttysw->ttysw_capslocked;
	    ttysw_display_capslock(ttysw);
	    return (TTY_DONE);
	}
    }
    return TTY_OK;
}

ttysw_strtokey(s)
	char *s;
{
	int i;

	if (strcmp(s, "LEFT") == 0)
		return (KEY_BOTTOMLEFT);
	else if (strcmp(s, "RIGHT") == 0)
		return (KEY_BOTTOMRIGHT);
	else if (isdigit(s[1])) {
		i = atoi(&s[1]);
		if (i < 1 || i > 16)
			return (-1);
		switch (s[0]) {
		case 'L':
			if (i == 1 || (i > 4 && i < 11)) {
				(void) fprintf(stderr,
					".ttyswrc error: %s cannot be mapped.\n",
					s);
				return (-1);
			} else
				return (KEY_LEFT(i));
		case 'R':
			return (KEY_RIGHT(i));
		case 'T':
		case 'F':
			return (KEY_TOP(i));
		}
	}
	return (-1);
}

static char *
savestr(s)
	char *s;
{
	char *p;
	extern char *malloc();

	p = malloc((unsigned) (strlen(s) + 1));
	if (p == (char *)NULL) {
		(void) fprintf(stderr, "Out of memory for key strings\n");
		return ((char *)NULL);
	}
	(void) strcpy(p, s);
	return (p);
}

/*
 * Interpret escape sequences in src,
 * while copying to dst.  Stolen from
 * termcap.
 */
static char *
tdecode(src, dst)
	register char *src;
	char *dst;
{
	register char *cp;
	register int c;
	register char *dp;
	int i;

	cp = dst;
	while (c = *src++) {
		switch (c) {

		case '^':
			c = *src++ & 037;
			break;

		case '\\':
			dp = "E\033^^\\\\::n\nr\rt\tb\bf\f";
			c = *src++;
nextc:
			if (*dp++ == c) {
				c = *dp++;
				break;
			}
			dp++;
			if (*dp)
				goto nextc;
			if (isdigit(c)) {
				c -= '0', i = 2;
				do
					c <<= 3, c |= *src++ - '0';
				while (--i && isdigit(*src));
			}
			break;
		}
		*cp++ = c;
	}
	*cp++ = 0;
	return (dst);
}


/*	These next routines do stuff that's supposed to be private
 *	to the ttytlsw module -- but since that's basically write-only,
 *	fuck it.
 */
void
ttysw_display_capslock(ttysw)
    struct ttysubwindow *ttysw;
{
    struct tool         *tool;
    struct ttytoolsubwindow *ttytlsw;
    char                 label[1024];
    char                *label_ptr;


    ttytlsw = (struct ttytoolsubwindow *) LINT_CAST(ttysw->ttysw_client);
    if (ttytlsw == NULL || ttytlsw->tool == NULL)
	return;
    tool = ttytlsw->tool;
    label_ptr = tool_get_attribute(tool, (int)(LINT_CAST(WIN_LABEL)));
    if (label_ptr == (char *) NULL)
	return;
    if (ttysw->ttysw_capslocked) {
	ttysw_add_caps(label, label_ptr);
    } else {
	ttysw_remove_caps(label, label_ptr);
    }
    (void)tool_set_attributes(tool, WIN_LABEL, label, 0);
    (void)tool_free_attribute((int)(LINT_CAST(WIN_LABEL)), label_ptr);
}

static char         caps_flag[] = "[CAPS] ";

#define CAPS_FLAG_LEN	(sizeof(caps_flag) - 1)

static char *
str_index(domain, pat)
    char               *domain;
    char               *pat;
{
    register int        i, patlen;

    patlen = strlen(pat);
    while (*domain != '\0') {
	for (i = 0; i <= patlen; i++) {
	    if (pat[i] == '\0')		/* exhausted pattern: win	 */
		return domain;
	    if (domain[i] == '\0')	/* exhausted domain: lose: 	 */
		return (char *) NULL;
	    if (pat[i] == domain[i])	/* partial match continues	 */
		continue;
	    break;			/* partial match failed	 */
	}
	domain++;
    }
    return (char *) NULL;
}

static void
ttysw_add_caps(label, label_ptr)
    char               *label;
    char               *label_ptr;
{
    if (str_index(label_ptr, caps_flag) == (char *) NULL) {
	bcopy(caps_flag, label, CAPS_FLAG_LEN);
	label += CAPS_FLAG_LEN;
    }
    (void) strcpy(label, label_ptr);
}

static void
ttysw_remove_caps(label, label_ptr)
    char               *label;
    char               *label_ptr;
{
    char               *flag_ptr;
    register int        len;

    if ((flag_ptr = str_index(label_ptr, caps_flag)) != (char *) NULL) {
	len = flag_ptr - label_ptr;
	    bcopy(label_ptr, label, len);
	label_ptr = flag_ptr + CAPS_FLAG_LEN;
	label += len;
    }
    (void) strcpy(label, label_ptr);
}
