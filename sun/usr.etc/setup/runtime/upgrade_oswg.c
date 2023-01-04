#ifndef lint
static  char sccsid[] = "@(#)upgrade_oswg.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "setup_runtime.h"
#include "install.h"


/*
 * Find the oswg's that were selected before and select them again.
 */
read_setup_info(ws)
Workstation	ws;
{
	Oswg		oswg;
	FILE		*fp;
	char		file[BUF];
	char		title[BUF];
	int		c;

	sprintf(file, "%s%s", ROOT_DIR, SETUP_INFO);
	fp = fopen(file, "r");
	if (fp == NULL) {
		return;
	}
	while ((c = skipspace(fp)) != EOF) {
		if (c == '%') {
			fscanf(fp, "%s", title);
			if (streq(title, "Optional_software")) {
				read_upgrade_oswg(ws, fp);
			} else if (streq(title, "Client_cpu_types")) {
				read_client_cpu_types(ws, fp);
			} else {
				while ((c = getc(fp)) != '\n')
					;
			}
		}
	}
	fclose(fp);
}

/*
 * Read the oswg lines out of the setup.info file to
 * see which optional software categories where selected before.
 */
static
read_upgrade_oswg(ws, fp)
Workstation	ws;
FILE		*fp;
{
	Arch_served	arch;
	Arch_type	arch_type;
	Arch_type	type;
	char		line[BUF];
	char		archstr[20];
	int		oswg_index;
	int		map;
	int		ix;
	int		c;

	map = 0;
	fscanf(fp, "%s", archstr);
	arch_type = archstr_to_type(archstr);
	SETUP_FOREACH_OBJECT(ws, WS_ARCH_SERVED_ARRAY, ix, arch) {
                type = (Arch_type) setup_get(arch, ARCH_SERVED_TYPE);
                if (arch_type == type) {
			break;
                }
        } SETUP_END_FOREACH

	for (;;) {
		c = skipspace(fp);
		if (c == EOF) {
			break;
		}
		ungetc(c, fp);
		if (c == '%') {
			break;
		}
		if (fgets(line, sizeof(line), fp) == NULL) {
			break;
		}
		*rindex(line, '\n') = '\0';
		oswg_index = get_oswg_index(ws, line);
		if (oswg_index >= 0) {
			map |= (1 << oswg_index);
		}
	}

	setup_set(arch, 
	    ARCH_OSWG, map,
	    0);
}

/*
 * Convert a string containing the name of an arch to an arch_type.
 */
Arch_type
archstr_to_type(str)
char		*str;
{
	Arch_type	arch_type;

	if (streq(str, "MC68010")) {
		arch_type = MC68010;
	} else if (streq(str, "MC68020")) {
		arch_type = MC68020;
	} else {
		arch_type = MC68010;	/* What to do? */
	}
	return(arch_type);
}

/*
 * Given a description, find the index of a matching oswg;
 */
static
get_oswg_index(ws, desc)
Workstation	ws;
char		*desc;
{
	Oswg		oswg;
	char		buf[BUF];
	char		*oswg_desc;
	char		*cp;
	int		i;

        SETUP_FOREACH_OBJECT(ws, WS_OSWG, i, oswg) {
		oswg_desc = setup_get(oswg, OSWG_DESCRIPTION);
		strcpy(buf, oswg_desc);
		cp = rindex(buf, '(');
		cp--;
		*cp = '\0';
		if (streq(buf, desc)) {
			return(i);
		}
	} SETUP_END_FOREACH
	return(-1);
}

/*
 * Cleanup routine.  Called when everything is done so that
 * setup can write a file of information to be used for
 * furture upgrades.  Currently, the info contains only
 * the optional software.
 */
save_oswg(ws)
Workstation	ws;
{
	FILE		*fp;
	Arch_served	arch;
	Oswg		oswg;
	char		file[BUF];
	char		archstr[BUF];
	char		buf[BUF];
	char		*oswg_desc;
	char		*cp;
	int		map;
	int		i, ix;

	sprintf(file, "%s%s", ROOT_DIR, SETUP_INFO);
	fp = fopen(file, "a");
	if (fp == NULL) {
		runtime_message(SETUP_EOPEN_FAILED, file, "appending"); 
		message(ws, setup_msgbuf); 
		return;
	}
	SETUP_FOREACH_OBJECT(ws, WS_ARCH_SERVED_ARRAY, ix, arch) {
		get_archname(arch, archstr);
		fprintf(fp, "%% Optional_software %s\n", archstr);
		map = (int) setup_get(arch, ARCH_OSWG);
		SETUP_FOREACH_OBJECT(ws, WS_OSWG, i, oswg) {
			if (is_in_map(map, i)) {
				oswg_desc = setup_get(oswg, OSWG_DESCRIPTION);
				strcpy(buf, oswg_desc);
				cp = rindex(buf, '(');
				cp--;
				*cp = '\0';
				fprintf(fp, "%s\n", buf);
			}
		} SETUP_END_FOREACH
	} SETUP_END_FOREACH
	fclose(fp);
}
