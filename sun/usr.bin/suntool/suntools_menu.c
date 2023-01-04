#ifndef lint
static  char sccsid[] = "@(#)suntools_menu.c 1.3 87/01/07";
#endif

/*
 * Sun Microsystems, Inc.
 */

/*
 * Suntools: Handle creation of walking menus
 */

#include <suntool/tool_hs.h>
#include <stdio.h>
#include <sys/stat.h>
#include <ctype.h>

#include <suntool/wmgr.h>
#include <suntool/icon_load.h>
#include <suntool/walkmenu.h>

#define Pkg_private 
#define Private static

Pkg_private int walk_getrootmenu(), walk_handlerootmenuitem();
Pkg_private int wmgr_menufile_changes();
Pkg_private char *wmgr_savestr(), *wmgr_save2str();

extern char	     *strcpy(),
		     *index();
void		     expand_path();
int		     wmgr_forktool();


Pkg_private Menu wmgr_rootmenu;

struct stat_rec {
    char             *name;	   /* Dynamically allocated menu file name */
    time_t            mftime;      /* Modified file time */
};

#define	MAX_FILES	40
#define MAXPATHLEN	1024

Pkg_private struct stat_rec wmgr_stat_array[];
Pkg_private int            wmgr_nextfile;


/* ======================== Walking menu support ============================ */


/* ARGSUSED */
Pkg_private int
walk_getrootmenu(mn, menu, file, mi, mis, maxitems)
	char *file;			/* File is the active parameter. */
	caddr_t menu, mn, mi, mis, maxitems;	/* the rest are ignored. */
{ 
    Menu m;
    
    if (wmgr_menufile_changes() == 0) return 1;
    wmgr_free_changes_array();
    m = menu_create(MENU_NOTIFY_PROC, menu_return_item, 0);
    if (walk_menufile(file, m) < 0) {
	menu_destroy(m);
    } else {
	menu_destroy(wmgr_rootmenu);
	wmgr_rootmenu = m;
    }
    return wmgr_rootmenu ? 1 : -1;
}

Private
walk_menufile(file, menu)
	char *file;
	Menu menu;
{   
    FILE *mfd;
    int	lineno = 1; /* Needed for recursion */
    struct stat statb;
    char full_file[MAXPATHLEN];

    expand_path(file, full_file);
    if ((mfd = fopen(full_file, "r")) == NULL) {
	(void)fprintf(stderr, "suntools: can't open menu file %s\n", full_file);
	return -1;
    }
    if (wmgr_nextfile >= MAX_FILES - 1) {
	(void)fprintf(stderr, "suntools: max number of menu files is %D\n", MAX_FILES);
	(void)fclose(mfd);
	return -1;
    }
    if (stat(full_file, &statb) < 0) {
	(void)fprintf(stderr, "suntools(rootmenu): ");
	perror(full_file);
	(void)fclose(mfd);
	return -1;
    }
    
    wmgr_stat_array[wmgr_nextfile].mftime = statb.st_mtime;
    wmgr_stat_array[wmgr_nextfile].name = wmgr_savestr(full_file);
    wmgr_nextfile++;

    if (walk_getmenu(menu, full_file, mfd, &lineno) < 0) {
	free(wmgr_stat_array[--wmgr_nextfile].name);
	(void)fclose(mfd);
	return -1;
    } else {
	(void)fclose(mfd);
	return 1;
    }
}

Private
walk_getmenu(m, file, mfd, lineno)
	Menu m;
	char *file;
	FILE *mfd;
	int *lineno;
{   
    char line[256], tag[32], prog[256], args[256];
    register char *p;
    Menu nm;
    Menu_item mi;
    char *nqformat, *qformat, *iformat, *format;
    char err[IL_ERRORMSG_SIZE], icon_file[MAXPATHLEN];
    struct pixrect *mpr;

    nqformat = "%[^ \t\n]%*[ \t]%[^ \t\n]%*[ \t]%[^\n]\n";
    qformat = "\"%[^\"]\"%*[ \t]%[^ \t\n]%*[ \t]%[^\n]\n";
    iformat = "\<%[^\>]\>%*[ \t]%[^ \t\n]%*[ \t]%[^\n]\n";

    for (; fgets(line, sizeof(line), mfd); (*lineno)++) {

	if (line[0] == '#') continue;

	for (p = line; isspace(*p); p++)
	    ;

	if (*p == '\0') continue;

	args[0] = '\0';
	format =  *p == '"' ? qformat : *p == '<' ? iformat : nqformat;
	if (sscanf(p, format, tag, prog, args) < 2) {
	    (void)fprintf(stderr, "suntools: format error in %s: line %d\n",
		    file, *lineno);
	    return -1;
	}

	if (strcmp(prog, "END") == 0) return 1;

	if (format == iformat) {
	    expand_path(tag, icon_file);
	    if ((mpr = icon_load_mpr(icon_file, err)) == NULL) {
		(void)fprintf(stderr, "suntools: icon file format error: %s:\n", err);
		return -1;
	    }
	} else mpr = NULL;

	if (strcmp(prog, "MENU") == 0) {
	    nm = menu_create(MENU_NOTIFY_PROC, menu_return_item, 0);
	    if (args[0] == '\0') {
		if (walk_getmenu(nm, file, mfd, lineno) < 0) {
		    menu_destroy(nm);
		    return -1;
		}
	    } else {
		if (walk_menufile(args, nm) < 0) {
		    menu_destroy(nm);
		    return -1;
		}
	    }
	    if (mpr)
		mi = menu_create_item(MENU_IMAGE, mpr,
				      MENU_PULLRIGHT, nm,
				      MENU_RELEASE, 
				      MENU_RELEASE_IMAGE, 
				      0);
	    else
		mi = menu_create_item(MENU_STRING, wmgr_savestr(tag), 
				      MENU_PULLRIGHT, nm,
				      MENU_RELEASE, 
				      MENU_RELEASE_IMAGE, 
				      0);
	} else {
	    if (mpr)
		mi = menu_create_item(MENU_IMAGE, mpr,
				      MENU_CLIENT_DATA,
				        wmgr_save2str(prog, args),
				      MENU_RELEASE, 
				      MENU_RELEASE_IMAGE, 
				      0);
	    else
		mi = menu_create_item(MENU_STRING, wmgr_savestr(tag),
				      MENU_CLIENT_DATA,
				        wmgr_save2str(prog, args),
				      MENU_RELEASE, 
				      MENU_RELEASE_IMAGE, 
				      0);
	}
	(void)menu_set(m, MENU_APPEND_ITEM, mi, 0);
    }
    return 1;
}

/* ARGSUSED */
Pkg_private
walk_handlerootmenuitem(menu, mi, rootfd)
	Menu menu;
	Menu_item mi;
	int	rootfd;	/* Ignored */
{
	int	returncode = 0;
	struct	rect recticon, rectnormal;
	char	*prog, *args;
	char full_prog[MAXPATHLEN];

	/*
	 * Get next default tool positions
	 */
	rect_construct(&recticon, WMGR_SETPOS, WMGR_SETPOS,
	    WMGR_SETPOS, WMGR_SETPOS);
	rectnormal = recticon;
	prog = (char *)menu_get(mi, MENU_CLIENT_DATA);
	args = index(prog, '\0') + 1;
	if (strcmp(prog, "EXIT") == 0) {
		returncode = wmgr_confirm(rootfd,
		    "Press the left mouse button to confirm Exit.  \
To cancel, press the right mouse button now.");
	} else if (strcmp(prog, "REFRESH") == 0) {
		wmgr_refreshwindow(rootfd);
	} else {
		suntools_mark_close_on_exec();
                expand_path(prog, full_prog);
		(void)wmgr_forktool(full_prog, args, &rectnormal, &recticon, 0);
	}
	return (returncode);
}
