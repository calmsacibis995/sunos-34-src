#ifndef	lint
static	char sccsid[] = "@(#)defaults_test.c 1.3 87/01/07 Copyr 1985 Sun Micro";
#endif
/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * This program provides a simplistic test of the defaults package.
 */

#include <sunwindow/sun.h>	/* True, False, ... */
#include <stdio.h>		/* NULL */

#include <sunwindow/defaults.h>	/* Defaults routine definitions */

void dump();
void main();

/*
 * main() tests out the defaults package.
 */
void
main()
{
	Bool	gprof_flag;	/* Kludge to fool compiler */

	defaults_special_mode();
	gprof_flag = False;
	defaults_get_string("/defaults/error_action", NULL);
	if (gprof_flag)
		return;

	printf("Some tests:\n");
	printf("Child:'%s'\n", defaults_get_child("/", NULL));
	printf("Click to type:'%s'\n",
	    defaults_get_enumeration("/sunview/click_to_type", "?", NULL));
	printf("Audible bell:'%s'\n",
	    defaults_get_string("/sunview/audible_bell", "?", NULL));
	printf("Scrollbar thickness:%d\n",
	    defaults_get_integer("/scrollbar/thickness", 29, NULL));

	defaults_set_enumeration("/sunview/click_to_type", "Disabled", NULL);
	defaults_set_string("/sunview/audible_bell", "no", NULL);
	defaults_set_integer("/scrollbar/thickness", 16, NULL);
	defaults_set_string("/Test/StringA",
		"\1\2\3\4\5\6\7\10\11\12\13\14\15\16\17", NULL);
	defaults_set_string("/Test/StringB",
		"\20\21\22\23\24\25\26\27\30\31\32\33\34\35\36\37", NULL);
	defaults_set_string("/Test/StringC",
		"\176\177\200\201\300\301\376\377");

/*	printf("\nEntire database:\n"); */
/*	defaults_remove("/text", NULL); */
/*	dump(stdout, "/"); */
/*	defaults_write_all("/", "/dev/tty", NULL); */

	printf("\nDatabase that has changed:\n");
	defaults_write_changed("/dev/tty", NULL);
	
	exit(0);
}

/*
 * dump(out_file, path_name) will print out Path_Name and all of its younger
 * relatives to Out_File.
 */
static void
dump(out_file, path_name)
	FILE	*out_file;	/* Output file */
	char	*path_name;	/* Path name to dump */
{
	char	temp[255];	/* Temporary name */
	char	*next;		/* Next name */

	fprintf(out_file, "%s \"%s\"\n", path_name,
		defaults_get_string(path_name, NULL, NULL));
	next = defaults_get_child(path_name, NULL, NULL);
	if (strcmp(path_name, "/") == 0)
		path_name = "";
	while (next != NULL){
		strcpy(temp, path_name);
		strcat(temp, "/");
		strcat(temp, next);
		/* fprintf(out_file, "%s\n", temp); */
		dump(out_file, temp);
		next = defaults_get_sibling(temp, NULL, NULL);
	}
}

