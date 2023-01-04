#ifndef	lint
static char sccsid[] = "@(#)hash_test.c 1.3 87/01/07 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * This program will let a user interactively test the hast table package.
 */

#include <stdio.h>	/* Standard I/O definitions. */
#include <sunwindow/sun.h>	/* Get common things like TRUE and FALSE defined */
#include "hash.h"	/* Hash table definitions */

/* Internally used routine. */
Bool	str_equal();
int	strcmp();

/*
 * main() will permit a user to interactively test and use the routines in
 * the hash package.
 */

main(){
	char	command[100];	/* Command to perform */
	Bool	flag;		/* Boolean flag */
	Hash	hash;		/* Hash table to manipulate */
	FILE	*in_file;	/* Input file */
	long	key;		/* Hash key */
	FILE	*out_file;	/* Output file */
	int	size;		/* Array size */
	long	value;		/* Hash value */
	
	hash = hash_create(10, 0, 0, 0, 0, 0, 0, 7);
	while (TRUE){
		/* Perform each command. */
		fprintf(stdout, "Cmd> ");
		if (parse_symbol(stdin, command) == NULL) {
			printf("Bad command\n");
			continue;
		}
		if (str_equal(command, "insert")){
			key = parse_int(stdin, NULL);
			value = parse_int(stdin, NULL);
			flag = hash_insert(hash, key, value);
			printf("Insert:%s\n", flag ? "True" : "False");
		}
		if (str_equal(command, "lookup")){
			key = parse_int(stdin, NULL);
			value = hash_lookup(hash, key);
			printf("Lookup:%d\n", value);
		}
		if (str_equal(command, "quit"))
			break;
		if (str_equal(command, "replace")){
			key = parse_int(stdin, NULL);
			value = parse_int(stdin, NULL);
			flag = hash_insert(hash, key, value);
			printf("Replace:%s\n", flag ? "True" : "False");
		}
		if (str_equal(command, "size"))
			printf("Size:%d\n", hash_size(hash));
		parse_eol(stdin);
		hash_show(hash);
	}
	
	exit(0);
}

/*
 * str_equal(str1, str2) will return TRUE if str1 equals str2 insensitive to
 * case.
 */

static Bool
str_equal(str1, str2)
	char *str1;		/* First string */
	char *str2;		/* Second string */
{
	return (Bool)(strcmp(str1, str2) == 0);
}

