
#ifndef lint
static	char sccsid[] = "@(#)card_parser.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <ctype.h>
#include <strings.h>
#include "setup_runtime.h"


typedef enum {
	DEFAULT_CARD,
	END_DEFAULT_CARDS,
	USER_DEFINED_CARD,
	LAST_CARD,
} Card_type;

typedef struct  xsect   Sect;

struct  xsect   {
        char    	*ty_name;       /* name of the section */
        Card_type	type;           /* number */
};

Sect	card_classes[] = {
	{ "default_card",		DEFAULT_CARD },
	{ "end_default_cards",		END_DEFAULT_CARDS },
	{ "user_defined_card",		USER_DEFINED_CARD },
	{ 0,				LAST_CARD }
};

Card_type	find_cardtype();


read_card_file(ws, filename)
Workstation	ws;
char		*filename;
{
	FILE		*fp;
	Card_type	t;
	int		c;

	fp = fopen(filename, "r");
	if (fp == NULL) {
		runtime_error("Could not open %s.", filename);
	}
	while ((c = skipspace(fp)) != EOF) {
		if (c == '%') {
			fscanf(fp, "%s", scratch_buf);
			while ((c = getc(fp)) != '\n')
				;

			t = find_cardtype(scratch_buf);

			switch(t) {

			case DEFAULT_CARD:
				read_card(ws, fp, DEFAULT_CARD);
				break;

			case USER_DEFINED_CARD:
				read_card(ws, fp, USER_DEFINED_CARD);
				break;

			case END_DEFAULT_CARDS:
				break;

			default:
				runtime_error("Unrecognized type `%s' in %s.",
					scratch_buf, filename);
			}
		}
	}
	fclose(fp);

	/*
	 * set the default card to the first card
	 */
	setup_set(ws, 
		WS_DEFAULT_CARD, (Card)setup_get(ws, WS_CARD, 0),
		0);
}


/*
 * Found a "% type" line, determine the type
 */
static	Card_type
find_cardtype(str)
char	*str;
{
	Sect	*tp;

	for (tp = card_classes; tp->ty_name != NULL; tp++) {
		if (strcmp(str, tp->ty_name) == 0) {
			return(tp->type);
		}
	}
	runtime_error("Unknown keyword type \"%s\".", str);
}


static
read_card(ws, fp, type)
Workstation     ws;
FILE           *fp;
Card_type	type;
{
	int     c;
	Card	card;
	char	name[32];
	char	args[4][32];
	int	root_size;
	int	swap_size;

	for (;;) {
		c = skipspace(fp);
		if (c == EOF) {
			return;
		}
		ungetc(c, fp);
		if (c == '%') {
			return;
		}

		if (fgets(name, sizeof(name), fp) == NULL) {
			return;
		}
		*(rindex(name, '\n')) = NULL;

		if (fgets(scratch_buf, sizeof(scratch_buf), fp) == NULL) {
			return;
		}
		*(rindex(scratch_buf, '\n')) = NULL;
		args[5][0] = NULL;
		sscanf(scratch_buf, 
		    "cpu=%[^;]; root=%[^,], %d; swap=%[^,], %d; %s",
		    &args[0][0], &args[1][0], &root_size,
		    &args[2][0], &swap_size, &args[3][0]);

		card = (Card) setup_create(CARD,
			CLIENT_NAME, name,
			CLIENT_ARCH, lookup_arch(ws, &args[0][0]),
			CLIENT_ROOT_PARTITION_INDEX, lookup_hp(ws, &args[1][0]),
			CLIENT_ROOT_SIZE, root_size,
			CLIENT_SWAP_PARTITION_INDEX, lookup_hp(ws, &args[2][0]),
			CLIENT_SWAP_SIZE, swap_size,
			0);

		if (streq(&args[3][0], "3COM")) {
			setup_set(card,
				CLIENT_3COM_INTERFACE, TRUE,
				0);
		}

		if (type == DEFAULT_CARD) {
			setup_set(card, 
				SETUP_NOTCHANGEABLE, TRUE,
				0);
		}

		setup_set(ws, WS_CARD, 
			SETUP_APPEND, card, 
			0);

	}
}


lookup_arch(ws, arch)
Workstation     ws;
char	*arch;
{
	int	n;
	char	*string;

	SETUP_FOREACH_CHOICE(ws, CONFIG_CPU, n, string) {
		if (streq(arch, string)) {
			return(n + CARD_TO_CLIENT_CPU);
		}
	} SETUP_END_FOREACH

	return(CARD_UNSPECIFIED_CPU);
}


lookup_hp(ws, partition)
Workstation     ws;
char	*partition;
{
	Controller	cont;
	Disk		disk;
	Hard_partition	hp;
	int		nd_index;
	int		i, j, k;
	char		*hp_name;

	if (streq(partition, "First Fit")) { 
		return(CARD_FIRST_FIT_ND);
	}

	nd_index = 0;
	SETUP_FOREACH_OBJECT(ws, WS_CONTROLLER, i, cont) {
		SETUP_FOREACH_OBJECT(cont, CONTROLLER_DISK, j, disk) {
			SETUP_FOREACH_OBJECT(disk, DISK_HARD_PARTITION, k, hp) {
				hp_name = (char *) setup_get(hp, HARD_NAME);
				if (streq(partition, hp_name)) {
					return(nd_index + CARD_TO_CLIENT_ND);
				}
				nd_index++;
			} SETUP_END_FOREACH
		} SETUP_END_FOREACH
	} SETUP_END_FOREACH

	return(CARD_UNSPECIFIED_ND);
}

