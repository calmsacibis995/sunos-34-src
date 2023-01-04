#ifndef lint
static	char sccsid[] = "@(#)defaults.c 1.5 87/02/24 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * This file contains routines for reading in and accessing a defaults
 * database.  Any errors are printed on the console as warning messeges.
 */

/*
 * Overview of data structures:
 *
 * All of the relevent data structures are stored in a defaults object.
 * The defaults object contains two hash tables.  The symbols hash table
 * is used to cannoncalize (sp?) strings.  Every time a string (symbol)
 * is encountered, it is looked up in the symbols hash table to get a
 * unique pointer to a string.  This means that two symbols can be tested
 * for equality by simply comparing their pointers.  The second hash table
 * is the nodes hash table.  The nodes hash table is indexed by a name,
 * where a name is a null-terminated list of symbol pointers
 * (i.e. ["CShell", "Path", "Help", NULL]).  The nodes are threaded together
 * so that each node knows its parent, eldest child, and next youngest
 * sibling.  In addition, each node has a name (i.e. the null-terminated
 * list of string pointers) and a string value.
 */

#include <stdio.h>		/* Standard I/O library */
#include <pwd.h>		/* Password file stuff */
#include <sys/types.h>		/* Get u_long defined for dir.h */
#include <sys/dir.h>		/* Directory access routines */
#include <sys/stat.h>		/* File status information */
#include <sunwindow/sun.h>	/* Define True, False, etc... */
#include "hash.h"		/* Hash table definitions */
#include "parse.h"		/* Parsing routine */
#include <strings.h>

#define	SEP_CHR		'/'	/* Separator character */
#define	SEP_STR		"/"	/* Separator string */

#define	HUGE_STRING	10000	/* Huge strings for Warren Teitelman */
#define	MAX_ERRORS	10	/* Maximum number of defaults errors */
#define	MAX_STRING	255	/* Maximum string length */
#define	MAX_NAME	100	/* Maximum name length */
#define	VERSION_NUMBER	2	/* Version number */
#define	VERSION_TAG	"SunDefaults_Version"	/* Version number tag */

#define	getchr(fp)	((int)getc(fp))	/* Simplify character at a time debugging */
#define	ungetchr	ungetc	/* Simplify character at a time debugging */

typedef int (*INT_FUNC)();
typedef char (*CHAR_FUNC)();
typedef char *Symbol;		/* A null-terminated string */
typedef Symbol *Name;		/* A null-terminated list of symbols */
typedef struct _defaults *Defaults;	/* Defaults object */
typedef struct _node *Node;	/* Node object */


/* Data structure defintions: */
struct _defaults {		/* Defaults data structure */
	char	*database_dir;	/* Database directory; NULL: .defaults only */
	int	errors;		/* Number of errors encountered */
	Hash	nodes;		/* Nodes hash table */
	Name	prefix;		/* Prefix symbol */
	char	*private_dir;	/* Private directory database */
	Node	root;		/* Root node of entire tree */
	Hash	symbols;	/* Symbols hash table */
	Bool	test_mode;	/* TRUE => database inaccessable */
};

struct _node {			/* Node object */
	Node	child;		/* Eldest child of parent */
	char	*default_value;	/* Default value associated with node */
	Bool	deleted;	/* TRUE => node does not really exist */
	Bool	file;		/* TRUE => file associated with node */
	Name	name;		/* Full name */
	Node	next;		/* Next node on same level (brother/sister) */
	Node	parent;		/* Parent node */
	Bool	private;	/* TRUE => Private database node */
	char	*value;		/* Value associated with node */
};

/* Global variables: */
static Defaults defaults;	/* = NULL; Global defaults object */
static char     defaults_file[MAX_STRING];

/* Exported routine definitions. */
#include <sunwindow/defaults.h>	/* Defaults routines */

/* Internally used routines: */
void	bomb();
Bool	chr_digit();
Bool	chr_letter();
void	clear_status();
void	create_defaults();
void	defaults_create();
Bool	get_bool();
long	*get_data();
char	*get_memory();
char	*get_login_directory();
void	name_cat();
Name	name_copy();
Bool	name_equal();
int	name_hash();
int	name_parse();
void	name_quick_parse();
char	*name_unparse();
Node	node_create();
void	node_delete();
void	node_delete_private();
Node	node_find_name();
Node	node_lookup_name();
Node	node_lookup_path();
void	node_write();
void	node_write1();
Node	path_lookup();
Bool	read();
void	read_lazy();
void	read_master();
void	read_master_database();
void	read_master_database1();
void	read_private_database();
void	set();
char	*slash_append();
void	str_write();
Symbol	symbol_copy();
Bool	symbol_equal();
int	symbol_hash();
Symbol	symbol_lookup();
void	warn();

/* Externally used routines: */
extern char		*getlogin();
extern char		*getenv();
extern struct passwd	*getpwnam();
extern struct passwd	*getpwuid();
extern char		*malloc();

/*****************************************************************/
/* Exported routines:                                            */
/*****************************************************************/

/*
 * NOTE: Any returned string pointers should be considered temporary at best.
 * If you want to hang onto the data, make your own private copy of the string!
 */

/*
 * defaults_exists(path_name, status) will return TRUE if Path_Name exists
 * in the database.
 */
Bool
defaults_exists(path_name, status)
	char	*path_name;	/* Node name to test for existence */
	int	*status;	/* Status flag */
{
	clear_status(status);
	return (Bool)(node_lookup_path(path_name, False, status) != NULL);
}

/*
 * defaults_get_boolean(path_name, default, status) will lookup Path_Name
 * in the defaults database and return TRUE if the value is "True", "Yes",
 * "On", "Enabled", "Set", "Activated", or "1".  FALSE will be returned if
 * the value is "False", "No", "Off", "Disabled", "Reset", "Cleared",
 * "Deactivated", or "0".  If the value is none of the above, a warning
 * message will be displayed and Default will be returned.
 */
Bool
defaults_get_boolean(path_name, default_bool, status)
	char	*path_name;	/* Path name */
	Bool	default_bool;	/* Default value */
	int	*status;	/* Status flag */
{
	static Defaults_pairs bools[] = {
		"True", (int)True,
		"False", (int)False,
		"Yes", (int)True,
		"No", (int)False,
		"On", (int)True,
		"Off", (int)False,
		"Enabled", (int)True,
		"Disabled", (int)False,
		"Set", (int)True,
		"Reset", (int)False,
		"Cleared", (int)False,
		"Activated", (int)True,
		"Deactivated", (int)False,
		"1", (int)True,
		"0", (int)False,
		NULL, -1,
	};
	char		*string_value;	/* String value */
	register Bool	value;		/* Value to return */

	clear_status(status);
	string_value = defaults_get_string(path_name, (char *)NULL, status);
	if (string_value == NULL){
		return default_bool;
	}
	value = (Bool)defaults_lookup(string_value, bools);
	if ((int)value == -1){
		warn("%s is '%s'; which is an unrecognized boolean value",
			path_name, string_value);
		value = default_bool;
	}
#ifdef TESTDEFAULTS
	if (defaults->database_dir && value != default_bool) {
		defaults->errors = 0;
		warn("%s: .d=%d .c=%d", path_name, value, default_bool);
	}
#endif
	return value;
}

/*
 * defaults_get_character(path_name, default, status) will lookup Path_Name in
 * the defaults database and return the resulting character value.
 * Default will be returned if any error occurs.
 */
int
defaults_get_character(path_name, default_character, status)
	char	*path_name;		/* Full database node path name */
	char	default_character;	/* Default return value */
	int	*status;		/* Status flag */
{
	Node	node;			/* Resulting node */

	clear_status(status);
	node = path_lookup(path_name, status);
	if (node == NULL)
		return default_character;
	if (strlen(node->value) != 1){
		warn("%s -- More than one character in character constant",
		    path_name);
		return default_character;
	}
#ifdef TESTDEFAULTS
	if (defaults->database_dir && node->value[0] != default_character) {
		defaults->errors = 0;
		warn("%s: .d=%c .c=%c", path_name, node->value[0], default_character);
	}
#endif
	return node->value[0];
}

/*
 * defaults_get_child(path_name, status) will return a pointer to the simple
 * name assoicated with the first child of Path_Name.  NULL will be returned,
 * if Path_Name does not exist or if Path_Name does not have a first child.
 */
char *
defaults_get_child(path_name, status)
	char		*path_name;	/* Full name of parent node */
	int		*status;	/* Status flag */
{
	register Node	node;		/* Node from database */

	clear_status(status);
	/* Find the node child. */
	node = node_lookup_path(path_name, False, status);
	if (node == NULL)
		return NULL;
	node = node->child;
	while ((node != NULL) && (node->deleted))
		node = node->next;
	return (node == NULL) ? NULL : node->name[name_length(node->name) - 1];
}

/*
 * defaults_get_default(path_name, default, status) will return the value
 * associated with Path_Name prior to being overridden by the clients private
 * database.  Default is returned if any error occurs.
 */
char *
defaults_get_default(path_name, default_value, status)
	char		*path_name;	/* Full name of node */
	char		*default_value;	/* Default node name */
	int		*status;	/* Status flag */
{
	register Node	node;		/* Node from database */

	clear_status(status);
	node = path_lookup(path_name, status);
	if (node == NULL)
		return default_value;
	return node->default_value;
}

/*
 * defaults_get_enum(path_name, pairs, status) will lookup the value associated
 * with Path_Name and scan the Pairs table and return the associated value.
 * If no match, can be found an error will be generated and the value
 * associated with last entry (i.e. the NULL entry) will be returned.  See,
 * defaults_lookup().
 */
int
defaults_get_enum(path_name, pairs, status)
	char		*path_name;	/* Full database path name */
	Defaults_pairs	*pairs;		/* Pairs table */
	int		*status;	/* Status flag */
{
	return defaults_lookup(defaults_get_string(path_name, (char *)NULL, status),
									pairs);
}

/*
 * defaults_get_enumeration(path_name, default, status) will lookup
 * "Path_Name/VALUE(Path_Name)" in the defaults database and return the
 * resulting string value.  Default will be returned if any error occurs.
 */
char *
defaults_get_enumeration(path_name, default_enumeration, status)
	char		*path_name;		/* Full database node name */
	char		*default_enumeration;	/* Default enumerate name */
	int		*status;		/* Status flag */
{
	register Node	node;			/* Resulting node */
	char		name[MAX_STRING];	/* Temporary name */
	register char	*name_pointer;		/* Name pointer */
	register Node	temp_node;		/* Temporary node */

	clear_status(status);
	node = path_lookup(path_name, status);
	if (node == NULL)
		return default_enumeration;
	name_pointer = name;
	(void)strcpy(name_pointer, path_name);
	(void)strcat(name_pointer, SEP_STR);
	(void)strcat(name_pointer, "$Enumeration");
	temp_node = node_lookup_path(name_pointer, True, status);
	if (temp_node == NULL){
		warn("%s is not an enumeration type", path_name);
		return default_enumeration;
	}
	(void)strcpy(name_pointer, path_name);
	(void)strcat(name_pointer, SEP_STR);
	(void)strcat(name_pointer, node->value);
	temp_node = node_lookup_path(name_pointer, True, status);
	if (temp_node == NULL)
	{
		warn("%s is not a valid enumeration value for %s",
			node->value, path_name);
		return default_enumeration;
	}
#ifdef TESTDEFAULTS
	if (defaults->database_dir
	    && temp_node->value && default_enumeration
	    && strcmp(temp_node->value, default_enumeration) != 0) {
		defaults->errors = 0;
		warn("%s: .d=%s .c=%s", path_name, temp_node->value, default_enumeration);
	}
#endif
	return temp_node->value;
}

/*
 * defaults_get_integer(path_name, default, status) will lookup Path_Name in
 * the defaults database and return the resulting integer value.
 * Default will be returned if any error occurs.
 */
int
defaults_get_integer(path_name, default_integer, status)
	char		*path_name;	/* Full database node name */
	int		default_integer; /* Default return value */
	int		*status;	/* Status flag */
{
	register char 	chr;		/* Temporary character */
	Bool		error;		/* TRUE => an error has occurred */
	Bool		negative;	/* TRUE => Negative number */
	register Node	node;		/* Resulting node */
	register int	number;		/* Resultant value */
	register char	*pointer;	/* Pointer into value */ 

	/* Lookup the node */
	clear_status(status);
	node = path_lookup(path_name, status);
	if (node == NULL)
		return default_integer;

	/* Convert string into integer (with error chacking) */
	error = False;
	pointer = node->value;
	negative = False;
	number = 0;
	chr = *pointer++;
	if (chr == '-'){
		negative = True;
		chr = *pointer++;
	}
	if (chr == '\0')
		error = True;
	while (chr != '\0'){
		if ((chr < '0') || (chr > '9')){
			error = True;
			break;
		}
		number = number * 10 + chr - '0';
		chr = *pointer++;
	}
	if (error){
		warn("%s contains an incorrect integer", path_name);
		return default_integer;
	}
	if (negative)
		number = -number;
#ifdef TESTDEFAULTS
	if (defaults->database_dir && number != default_integer) {
		defaults->errors = 0;
		warn("%s: .d=%d .c=%d", path_name, number, default_integer);
	}
#endif
	return number;
}

/*
 * defaults_get_integer_check(path_name, default, mininum, maximum, status)
 * will lookup Path_Name in the defaults database and return the resulting
 * integer value. If the value in the database is not between Minimum and
 * Maximum (inclusive), an error message will be printed.  Default will be
 * returned if any error occurs.
 */
int
defaults_get_integer_check(path_name, default_int, minimum, maximum, status)
	char	*path_name;	/* Full path name of node */
	int	default_int;	/* Default return value */
	int	minimum;	/* Minimum value */
	int	maximum;	/* Maximum value */
	int	*status;	/* Status flag */
{
	int	value;		/* Return value */

	clear_status(status);
	value = defaults_get_integer(path_name, default_int, status);
	if ((minimum <= value) && (value <= maximum))
		return value;
	else {
		warn("The value of %s is %d which is not between %d and %d",
			path_name, value, minimum, maximum);
		return default_int;
	}
}

/*
 * defaults_get_sibling(path_name, status) will return a pointer to the simple
 * name assoicated with the next sibling of Path_Name.  NULL will be returned,
 * if Path_Name does not exist or if Path_Name does not have an
 * next sibling.
 */
char *
defaults_get_sibling(path_name, status)
	char		*path_name;	/* Full name of parent node */
	int		*status;	/* Status flag */
{
	register Node	node;		/* Node from database */

	clear_status(status);
	node = node_lookup_path(path_name, False, status);
	if (node == NULL){
		warn("Could not find '%s'", path_name);
		return NULL;
	}
	do	node = node->next;
	    while ((node != NULL) && node->deleted);
	return (node == NULL) ? NULL : node->name[name_length(node->name) - 1];
}

/*
 * defaults_get_string(path_name, default, status) will lookup and return the
 * string value assocatied with Path_Name in the defaults database.
 * Default will be returned if any error occurs.
 */
char *
defaults_get_string(path_name, default_string, status)
	char		*path_name;	/* Full database node name */
	char		*default_string; /* Default return value */
	int		*status;	/* Status flag */
{
	register Node	node;		/* Resulting node */

	clear_status(status);
	node = path_lookup(path_name, status);
#ifdef TESTDEFAULTS
	if (defaults->database_dir && node != NULL
	    && node->value && default_string
	    && strcmp(node->value, default_string) != 0) {
		defaults->errors = 0;
		warn("%s: .d=%s .c=%s", path_name, node->value, default_string);
	}
#endif
	return (node == NULL) ? default_string : node->value;
}

/*
 * defaults_lookup(name, pairs) will linearly scan the Pairs data structure
 * looking for Name.  The value associated with Name will be returned.
 * If Name can not be found in Pairs, the value assoicated with NULL will
 * be returned.  (The Pairs data structure must be terminated with NULL.)
 */
int
defaults_lookup(name, pairs)
	register char		*name;	/* Name to look up */
	register Defaults_pairs	*pairs;	/* Default */
{
	register Defaults_pairs	*pair;	/* Current pair */

	for (pair = pairs; pair->name != NULL; pair++){
		if (name == NULL)
			continue;
		if (symbol_equal(name, pair->name))
			break;
	}
	return pair->value;
}

/*
 * defaults_move(to_path_name, from_path_name, before_flag, status) will
 * rearrange the defaults database so that From_Path_Name will internally
 * be next to To_Path_Name.  If Before_Flag is True, a call to
 * defaults_get_sibling(To_Path_Name) will return From_Path_Name.
 * If Before_Flag is False, a call to defaults_get_child(From_Path_Name)
 * will return To_Path_Name.
 */
void
defaults_move(to_path_name, from_path_name, before_flag, status)
	char		*to_path_name;		/* Destination path name */
	char		*from_path_name;	/* Source path_name */
	Bool		before_flag;		/* True => Insert before */
	int		*status;		/* Status flag */
{
	Node		anchor;			/* Node behind last one */
	Node		from_node;		/* From node */
	Node		parent;			/* Parent node */
	Node		temp;			/* Temporay node */
	Node		to_node;		/* To node */

	/* Check to be sure the nodes can be moved. */
	clear_status(status);
	from_node = node_lookup_path(from_path_name, False, status);
	if (from_node == NULL)
		return;
	to_node = node_lookup_path(to_path_name, False, status);
	if (to_node == NULL)
		return;
	if (from_node == to_node){
		warn("%s and %s are the same node",
			to_path_name, from_path_name);
		return;
	}
	if (from_node->parent != to_node->parent){
		warn("%s and %s are not siblings of one another",
			to_path_name, from_path_name);
		return;
	}
	parent = from_node->parent;


	/* Remove from node from database tree. */
	if (parent->child == from_node)
		parent->child = from_node->next;
	else {
		temp = parent->child;
		do {	anchor = temp;
			temp = temp->next;
		} while ((temp != NULL) && (temp != from_node));
		if (temp == NULL){
			warn("Data structure around %s and %s is messed up",
				to_path_name, from_path_name);
			return;
		}
		anchor->next = temp->next;
	}
	from_node->next = NULL;

	/* Re-insert the node. */
	if (!before_flag){
		from_node->next = to_node->next;
		to_node->next = from_node;
		return;
	}
	if (parent->child == to_node)
		parent->child = from_node;
	else {
		temp = parent->child;
		do {	anchor = temp;
			temp = temp->next;
		} while ((temp != NULL) && (temp != to_node));
		if (temp == NULL){
			warn("Data structure around %s and %s is messed up",
				to_path_name, from_path_name);
			return;
			}
		anchor->next = from_node;
	}
	from_node->next = to_node;
}

/*
 * defaults_remove(path_name, status) will remove Path_Name and its children
 * from the defaults database.
 */
void
defaults_remove(path_name, status)
	char		*path_name;	/* Full path name of node to remove */
	int		*status;	/* Status flag */
{
	register Node	node;		/* Node to delete */

	clear_status(status);
	node = path_lookup(path_name, status);
	if (node != NULL)
		node_delete(node);
}

/*
 * defaults_remove_private(path_name, status) will remove Path_Name and its
 * children from the defaults database.
 */
void
defaults_remove_private(path_name, status)
	char		*path_name;	/* Full path name of node to remove */
	int		*status;	/* Status flag */
{
	register Node	node;		/* Node to delete */

	clear_status(status);
	node = path_lookup(path_name, status);
	if (node != NULL)
		node_delete_private(node);
}

/*
 * defaults_reread(path_name, status) will reread the portion of the database
 * associated with Path_Name.
 */
/*ARGSUSED*/
void
defaults_reread(path_name, status)
	char		*path_name;	/* Path name to reread */
	int		*status;	/* Status flag */
{
#ifdef WILL_IMPLEMENT
	Symbol		name[MAX_NAME];	/* Temporary name */
	register Symbol	*name_pointer;	/* Name pointer */
	register Node	node;		/* Parent node */
#endif

	defaults = NULL;
}

/*
 * defaults_set_character(path_name, value, status) will set Path_name to
 * Value.  Value is a character.
 */
void
defaults_set_character(path_name, value, status)
	char		*path_name;	/* Name to look up */
	char		value;		/* Character to set */
	int		*status;	/* Status flag */
{
	register char	*new_value;	/* New value */

	clear_status(status);
	new_value = get_memory(2);
	new_value[0] = value;
	new_value[1] = '\0';
	set(path_name, new_value, status);
}

/*
 * defaults_set_enumeration(path_name, value, status) will set Path_Name to
 * Value.  Value is a pointer to a string.
 */
void
defaults_set_enumeration(path_name, value, status)
	char		*path_name;	/* Full node name */
	char		*value;		/* Enumeration value */
	int		*status;	/* Status flag */
{
	Symbol		name[MAX_NAME];	/* Temporary name */
	register Symbol	*name_pointer;	/* Name pointer */
	register Node	node;		/* Temporary Node */
	register int	size;		/* Name size */ 
	register Node	temp_node;	/* Temporary node */

	clear_status(status);
	name_pointer = name;
	name_quick_parse(path_name, name_pointer);
	node = node_find_name(name_pointer, True, status);
	if (node == NULL){
		warn("Can not insert '%s' into database", path_name);
		return;
	}
	/* type check the enum if we've got the master database */
	if (defaults->database_dir){
		size = name_length(name_pointer);
		/* is it an enum? */
		name_pointer[size] = symbol_lookup("$Enumeration");
		name_pointer[size + 1] = NULL;
		temp_node = node_lookup_name(name_pointer, True, status);
		if (temp_node == NULL){
			warn("'%s' is not an enumeration node", path_name);
			return;
		}
		/* is it a valid one? */
		name_pointer[size] = symbol_lookup(value);
		temp_node = node_lookup_name(name_pointer, True, status);
		if (temp_node == NULL){
			warn("%s can not have %s assigned as its value",
							path_name, value);
			return;
		}
	}
	node->value = symbol_copy(value);
	node->deleted = False;
	node->private = True;
}

/*
 * defaults_set_integer(path_name, value, status) will set Path_Name to Value.
 * Value is an integer.
 */
void
defaults_set_integer(path_name, value, status)
	char	*path_name;	/* Full node name */
	int	value;		/* Integer value */
	int	*status;	/* Status flag */
{
	char	temp_value[MAX_STRING];	/* Temporary string value */

	clear_status(status);
	(void)sprintf(temp_value, "%d", value);
	set(path_name, symbol_copy(temp_value), status);
}

/*
 * defaults_set_prefix(prefix, status) will cause all subsequent node lookup
 * to first a node under Prefix first.  For example, if the prefix has been
 * set to "/Mumble/Frotz" and the user accesses "/Fee/Fie/Fo", first
 * "/Mumble/Frotz/Fee/Fie/Fo" will looked at and then if it is not available
 * "/Fee/Fie/Fo" will be looked at.  This is used to permit individual programs
 * to permit overriding of defaults.  If Prefix is NULL, the prefix will be
 * cleared.
 */
void
defaults_set_prefix(prefix, status)
	register char	*prefix;	/* Prefix to use */
	int		*status;	/* Status flag */
{
	Symbol		name[MAX_NAME];	/* Temporary name */

	clear_status(status);
	if (defaults == NULL)
		create_defaults();
	if (prefix == NULL){
		defaults->prefix = NULL;
		return;
	}
	name_quick_parse(prefix, name);
	defaults->prefix = name_copy(name);
	defaults->test_mode =
		defaults_get_boolean("/Defaults/Test_Mode", False, (int *)NULL);
}

/*
 * defaults_set_string(path_name, value, status) will set Path_Name to Value.
 * Value is a poitner to a string.
 */
void
defaults_set_string(path_name, value, status)
	char	*path_name;	/* Full node name */
	char	*value;		/* New string value */
	int	*status;	/* Status flag */
{
	set(path_name, symbol_copy(value), status);
}

/*
 * defaults_special_mode() will cause the database to behave as if the entire
 * master database has been read into memory prior to reading in the private
 * database.
 */
void
defaults_special_mode()
{
	char		*database_dir;	/* Database directory */
	register Node	node;		/* Current node to process */
	char		*private_dir;	/* Private directory */

	/* Create database. */
	defaults = NULL;
	defaults_create();

	/* Read in env var DEFAULTS_FILE to find out where database 
	 *directories are. 
	 */
	read_private_database();
	node = node_lookup_path("/Defaults/Directory", False, (int *)NULL);
	database_dir = (node == NULL) ? NULL : slash_append(node->value);
	node = node_lookup_path("/Defaults/Private_Directory", False, (int *)NULL);
	private_dir = (node == NULL) ? NULL : slash_append(node->value);

	/* Drop the entire private database on the floor! */
	defaults = NULL;
	defaults_create();
	defaults->database_dir = database_dir;
	defaults->private_dir = private_dir;

	/* Read in master database before private database so that node order
	 * is preserved in master database order rather than private order. */
	read_master_database();
	read_private_database();
	for (node = defaults->root->child; node != NULL; node = node->next)
		if (node->file)
			read_master(node->name[0]);
	defaults->test_mode =
		defaults_get_boolean("/Defaults/Test_Mode", False, (int *)NULL);
}

/*
 * defaults_write_all(path_name, file_name, status) will write the all of the
 * database nodes from Path_Name and below into File_Name.  Out_File is the
 * string name of the file to create.  If File_Name is NULL, env var 
 * DEFAULTS_FILE will be used.
 */
void
defaults_write_all(path_name, file_name, status)
	char	*path_name;	/* Path name */
	char	*file_name;	/* Output file name */
	int	*status;	/* Status flag */
{
	node_write(path_name, file_name, status, 0);
}

/*
 * defaults_write_changed(file_name, status) will write out all of the private
 * database entries to File_Name.  Any time a database node is set it becomes
 * part of the private database.  Out_File is the string  name of the file to
 * create.  If File_Name is NULL, env var DEFAULTS_FILE will be used.
 */
void
defaults_write_changed(file_name, status)
	char	*file_name;	/* Output file name */
	int	*status;	/* Status flag */
{
	node_write("/", file_name, status, 1);
}

/*
 * defaults_write_differences(file_name, status) will write out all of the
 * database entries that differ from the master database.  Out_File is the
 * string name of the file to create.  If File_Name is NULL, env var 
 * DEFAULTS_FILE be used.
 */
void
defaults_write_differences(file_name, status)
	char	*file_name;	/* Output file name */
	int	*status;	/* Status flag */
{
	node_write("/", file_name, status, 2);
}

/*****************************************************************/
/* Internal routines:                                            */
/*****************************************************************/

/*
 * bomb() will cause the program to bomb.
 */
static void
bomb()
{
	(void)fflush(stderr);
	sleep(5);	/* Wait for error message to show up */
	abort();
}

/*
 * chr_digit(chr) returns True if CHr is a valid digit.
 */
static Bool
chr_digit(chr)
	char	chr;		/* Character to test */
{
	return (Bool)(('0' <= chr) && (chr <= '9'));
}

/*
 * chr_letter(chr) returns True if Chr is a valid letter.
 */
static Bool
chr_letter(chr)
	register char	chr;		/* Character to test */
{
	return (Bool)((' ' < chr) && (chr <= '~') && (chr != '/'));
}

/*
 * clear_status(status) will clear the status flag if it is non-NULL.
 */
static void
clear_status(status)
	int	*status;	/* Status flag */
{
	if (status != NULL)
		warn("Somebody passed a non-NULL status code");
}


/*
 * create_defaults() will initialize the global defaults database.
 */
static void
create_defaults()
{
	if (defaults == NULL){
		defaults_create();
		read_private_database();
		/* Don't use master database unless "special" user */
		if (!defaults_get_boolean("/Defaults/Private_only", True, (int *)NULL)) {
			read_master_database();
		}
		defaults->test_mode =
		    defaults_get_boolean("/Defaults/Test_Mode", False, (int *)NULL);
	}
}

/*
 * defaults_create() will intialize the global defaults object.
 */
static void
defaults_create()
{
	Symbol			name[1];	/* Empty name */
	register Defaults	new_defaults;	/* New defaults object */
	extern Hash hash_create();

	/* Create hash tables. */
	new_defaults = (Defaults)get_memory(sizeof *defaults);
	defaults = new_defaults;
	new_defaults->database_dir = NULL;
	new_defaults->errors = 0;
	new_defaults->nodes = hash_create(250,
		NULL, (CHAR_FUNC)name_equal, name_hash, (INT_FUNC)NULL, NULL, (INT_FUNC)NULL, 7);
	new_defaults->prefix = NULL;
	new_defaults->private_dir = NULL;
	new_defaults->root = node_create();
	new_defaults->symbols = (Hash)hash_create(250,
		NULL, (CHAR_FUNC)symbol_equal, symbol_hash, (INT_FUNC)NULL, NULL, (INT_FUNC)NULL, 7);
	new_defaults->test_mode = False;
	name[0] = NULL;
	(void)hash_insert((Hash)new_defaults->nodes, (caddr_t)name_copy(name), (caddr_t)new_defaults->root);
}

/*
 * get_data(size) will get size words of memory.
 */
static long *
get_data(size)
	int	size;		/* Number of words to allocate */
{
	long	*data;		/* Pointer to beginning of data */

	data = (long *)get_memory(size<<2);	/* Blech! Machine dependent! */
	return data;
}

/*
 * get_memory(size) will get size bytes of memory or die trying.
 */
static char *
get_memory(size)
	int		size;	/* Number of bytes to allocate */
{
	register char	*data;	/* New data */

	data = malloc((unsigned) size);
	if (data == NULL){
		(void)fprintf(stderr, "%s%d%s\n", "SunDefaults: Can not allocate ",
			size, " bytes of memory (Out of swap space?)");
		bomb();
		/*NOTREACHED*/
	} else	return data;
}

/*
 * get_login_dirdirectory() will return the login directory.  The HOME
 * environment variable is tryed first followed by the password file.
 * An error message is printed if the login directory can not be found.
 * This routine (actually getlogindir) was copied from the suntools library
 * so that everything would link properly with Unix's stupid one pass linker.
 */
static char *
get_login_directory()
{
	static char		*directory;	/* Implicite = NULL */
	register struct passwd	*password;
	register char		*login_name;

	if (directory != NULL)
		return directory;
	directory = getenv("HOME");
	if (directory != NULL){
		directory = symbol_copy(directory);
		return directory;
	}
	login_name = getlogin();
	if (login_name == NULL)
		password = getpwuid(getuid());
	else
		password = getpwnam(login_name);
	if (password == NULL) {
		warn("Could not find user in password file.");
		directory = "";
		return directory;
	}
	if (password->pw_dir == NULL) {
		warn("No home directory in password file.");
		directory = "";
		return directory;
	}
	directory = symbol_copy(password->pw_dir);
	return directory;
}

/*
 * name_cat(dest_name, source_name) will concatonate Source_Name onto the
 * end of Dest_Name.
 */
static void
name_cat(dest_name, source_name)
	Symbol	*dest_name;	/* Destination name */
	Symbol	*source_name;	/* Source name */
{
	name_move(dest_name + name_length(dest_name), source_name);
}

/*
 * name_copy(name) will make and return a copy of Name.
 */
static Name
name_copy(name)
	Name		name;		/* Name to copy */
{
	Name		new;		/* New name */

	new = (Name)get_data(name_length(name) + 1);
	name_move(new, name);
	return new;
}

/*
 * name_equal(name1, name2) will return True if Name1 equals Name2.
 */
static Bool
name_equal(name1, name2)
	register Name	name1;		/* First name */
	register Name	name2;		/* Second name */
{
	register Symbol	symbol1;	/* Symbol from first name */
	register Symbol	symbol2;	/* Symbol from second name */

	while (True){
		symbol1 = *name1++;
		symbol2 = *name2++;
		if (symbol1 != symbol2)
			return False;
		if (symbol1 == NULL)
			return True;
	}
}

/*
 * name_hash(name) will return a hash of Name.
 */
static int
name_hash(name)
	register Name	name;	/* Name to hash */
{
	register Symbol	symbol;	/* Symbol from name */
	register int	hash;	/* Hash value */

	hash = 0;
	do {	symbol = *name++;
		hash += (int)symbol;
	} while (symbol != NULL);
	return hash;
}

/*
 * name_length(name) will return the number of symbols in Name.
 */
static int
name_length(name)
	register Name	name;	/* Name to return length of */
{
	register int	length;	/* Length of name */

	length = 0;
	while (*name++ != NULL)
		length++;
	return length;
}

/*
 * name_move(dest, source) will the name Source over to the name Dest.
 */
static int
name_move(dest, source)
	register Name	dest;		/* Destination name */
	register Name	source;		/* Source Name */
{
	while (*source != NULL)
		*dest++ = *source++;
	*dest = NULL;
}

/*
 * name_parse(in_file, symbols, default_name, name) will parse a node name
 * read from In_File into its component symbols and store the resulting
 * parts into Name.  Default symbols will be read from Default_Name.  The
 * symbol hash table is Symbols.  The number of simple node names in the full
 * node name will be returned.  0 will be returned on an error.
 */
/*ARGSUSED*/
static int
name_parse(in_file, symbols, default_name, name)
	register FILE	*in_file;	/* Input file */
	Hash		symbols;	/* Symbol hash table */
	Name		default_name;	/* Default name */
	register Name	name;		/* Place to store resultant name */
{
	register int	chr;		/* Temporary character */
	register int	default_size;	/* Size of default name */
	register Bool	fully;		/* TRUE => fully specified node name */
	register char	*symbol;	/* Current symbol */
	register int	size;		/* Number of parts in name */
	char		temp[MAX_STRING]; /* Temporary symbol name */

	size = 0;
	chr = parse_whitespace(in_file);
	fully = (Bool)(chr == SEP_CHR);
	if (fully){
		/* Fully specified node name. */
		(void) getchr(in_file);	/* Dispose of leading '/' */
		/* Process leading default_names. */
		while (True){
			chr = getchr(in_file);
			if (chr != SEP_CHR)
				break;
			if (*default_name == NULL){
				(void)strcpy(temp, SEP_STR);
				(void)strcat(temp, SEP_STR);
				warn("No default available for '%s'", temp);
				*name = NULL;
				return size;
			}
			*name++ = *default_name++;
			size++;
		}
		(void)ungetchr(chr, in_file);
	} else {
		/* Partially specified node name. */
		/* Copy over as many default_names as are available. */
		default_size = 0;
		while (*default_name != NULL){
			*name++ = *default_name++;
			default_size++;
		}
	}

	/* Process each simple node name. */
	while (chr_letter(chr) || chr_digit(chr)){
		if (parse_symbol(in_file, &temp[0]) == NULL)
			return 0;
		symbol = symbol_lookup(&temp[0]);
		*name++ = symbol;
		size++;
		if (fully){
			*default_name++ = symbol;
			*default_name = NULL;
		}
		chr = getchr(in_file);
		if ((chr == ' ') || (chr == '\t') ||
		    (chr == '\n') || (chr == EOF))
			break;
		if (chr == SEP_CHR){
			chr = getchr(in_file);
			(void)ungetchr(chr, in_file);
		}
		else {
			warn("'%c' found in node name", chr);
			break;
		}
	}
	(void)ungetchr(chr, in_file);
	*name = NULL;
	if (size > 0)
		size += default_size;
	return size;
}

/*
 * name_quick_parse(path_name, name) will quickly parse Path_Name and store
 * the result into Name.
 */
static void
name_quick_parse(path_name, name)
	char		*path_name;	/* Full name string */
	Name		name;		/* Place to store parsed name */
{
	register char	chr;		/* Temporary character */
	register char	*full_pointer;	/* Pointer into full name */
	register Name	name_pointer;	/* Pointer into name */
	char		temp[MAX_STRING]; /* Temporary name */
	register char	*temp_pointer;	/* Pointer into temporary name */

	/* Build up the name. */
	full_pointer = path_name;
	if (*full_pointer++ != SEP_CHR){
		warn("'%s' does not start with '%c'.", path_name, SEP_CHR);
		name[0] = NULL;
		return;
	}
	name_pointer = name;
	while (True){
		temp_pointer = &temp[0];
		while (True){
			chr = *full_pointer++;
			if ((chr == SEP_CHR) || (chr == '\0'))
				break;
			if (chr_letter(chr) || chr_digit(chr))
				*temp_pointer++ = chr;
			else {
				warn("'%c' is not permitted in %s",
							    chr, path_name);
				name[0] = NULL;
				return;
			}
		}
		*temp_pointer = '\0';
		*name_pointer++ = symbol_lookup(&temp[0]);
		if (chr == '\0')
			break;
	}
	*name_pointer = NULL;
}

/*
 * name_unparse(name, data) will write the string corresponding to Name
 * into Data.  If Data is NULL, a new string will be allocated.
 */
static char *
name_unparse(name, data)
	register Name	name;		/* Name to unparse */
	register char	*data;		/* Data to unparse */
{
	register int	size;		/* Size of string */
	register Name	temp;		/* Temporary name */

	if (name == NULL){
		if (data == NULL)
			data = get_memory(2);
		(void)strcpy(data, SEP_STR);
		return data;
	}
	if (data == NULL){
		size = 0;
		temp = name;
		while (*temp != NULL)
			size += strlen(*temp++) + 1;
		data = get_memory(size + 1);
	}
	data[0] = '\0';
	while (*name != NULL){
		(void)strcat(data, SEP_STR);
		(void)strcat(data, *name++);
	}
	return data;
}

/*
 * node_create() will create and return an empty node.
 */
static Node
node_create()
{
	register Node	node;	/* New node */

	node = (Node)get_memory(sizeof *node);
	node->child = NULL;
	node->default_value = DEFAULTS_UNDEFINED;
	node->deleted = False;
	node->file = False;
	node->name = NULL;
	node->next = NULL;
	node->parent = NULL;
	node->private = False;
	node->value = DEFAULTS_UNDEFINED;
	return node;
}

/*
 * node_delete(node) will cause Node and its descendentes to be "deleted".
 * The nodes will only be marked as deleted.  No storage will be deallocated.
 */
static void
node_delete(node)
	register Node	node;	/* Node to delete */
{
	node->deleted = True;
	for (node = node->child; node != NULL; node = node->next)
		node_delete(node);
}

/*
 * node_delete_private(node) will cause Node and its descendentes to be
 * "deleted".  The nodes will only be marked as deleted.  No storage will be
 * deallocated.
 */
static void
node_delete_private(node)
	register Node	node;	/* Node to delete */
{
	
	if (node->private)
		node->deleted = True;
	for (node = node->child; node != NULL; node = node->next)
		node_delete_private(node);
}

/*
 * node_find_name(name, multiple, status) will return the node associated with
 * Name from the defaults database.  The important side effect of this routine
 * is that it makes for sure that all the nodes between Name and the root node
 * both exist and are properly threaded together.  If Multiple is TRUE,
 * multiple lookups will be performed to prefixes.
 */
/*ARGSUSED*/
static Node
node_find_name(name, multiple, status)
	register Name	name;		/* Name to lookup/enter */
	Bool		multiple;	/* TRUE => multiple prefix lookups */
	int		*status;	/* Status flag */
{
	register int	length;		/* Name length */
	register Node	next;		/* Next node at same level */
	register Node	node;		/* Node that is looked up */
	char		path_name[MAX_STRING]; /* Path name */
	Node		parent;		/* Parent node */
	Symbol		symbol;		/* Node symbol */

	if (status != NULL){
		(void)name_unparse(name, path_name);
		warn("Status is non-NULL in %s", path_name);
	}

	if (defaults == NULL)
		create_defaults();

	if (defaults->test_mode)
		return NULL;

	/* The root node is special. */
	if (name[0] == NULL)
		return defaults->root;

	/* Just return it if it is already in the Nodes table. */
	read_lazy(name);
	node = (Node)hash_lookup(defaults->nodes, (caddr_t)name);
	if (node != NULL){
		node->deleted = False;
		return node;
	}

	/* Get the parent node. */
	length = name_length(name);
	symbol = name[length - 1];
	name[length - 1] = NULL;
	parent = node_find_name(name, False, status);
	name[length - 1] = symbol;

	/* Create the new node. */
	node = node_create();
	name = name_copy(name);
	node->name = name;
	node->parent = parent;

	/* Thread this node onto the end of the parent next list. */
	next = parent->child;
	if (next == NULL)
		parent->child = node;
	else {
		while (next->next != NULL)
			next = next->next;
		next->next = node;
	}

	/* Insert the node into the Nodes hash table. */
	(void)hash_insert(defaults->nodes, name, node);
	return node;
}

/*
 * node_lookup_name(name, multiple, status) will lookup Name in the defaults
 * database and return the associated node.  If Path_Name is not in the
 * database, NULL will be returned.
 */
static Node
node_lookup_name(name, multiple, status)
	register Name	name;		/* Name to lookup */
	Bool		multiple;	/* TRUE => multiple lookups */
	int		*status;	/* Status flag */
{
	register Node	node;		/* Resultant node */
	char		path_name[MAX_STRING];	/* Path name string */
	Symbol		temp[MAX_NAME];	/* Temporay name */

	if (defaults == NULL)
		create_defaults();
	if (status != NULL){
		(void)name_unparse(name, path_name);
		warn("Status non-NULL in %s", path_name);
	}

	if (defaults->test_mode)
		return NULL;

	read_lazy(name);
	if (multiple && (defaults->prefix != NULL)){
		temp[0] = NULL;
		name_cat(temp, defaults->prefix);
		name_cat(temp, name);
		node = (Node)hash_lookup(defaults->nodes, temp);
		if ((node != NULL) && (!node->deleted))
			return node;
	}
	node = (Node)hash_lookup(defaults->nodes, name);
	if ((node != NULL) && (!node->deleted))
		return node;
	else	return NULL;
}

/*
 * node_lookup_path(path_name, multiple, status) will lookup Path_name in the
 * defaults database and return the associated node.  If Path_Name is not in
 * the database, NULL will be returned.
 */
static Node
node_lookup_path(path_name, multiple, status)
	char	*path_name;	/* Full path name */
	Bool	multiple;	/* TRUE => multiple lookups */
	int	*status;	/* Status flag */
{
	Symbol	name[MAX_NAME];	/* Node name */

	if (defaults == NULL)
		create_defaults();
	if (strcmp(path_name, SEP_STR) == 0)
		return defaults->root;
	name_quick_parse(path_name, name);
	return node_lookup_name(name, multiple, status);
}

/*
 * node_write(path_name, file_name, status, flag) will write Path_Name to
 * File_Name.  If File_Name is NULL, env var DEFAULTS_FILE will be used.
 */
static void
node_write(path_name, file_name, status, flag)
	char		*path_name;	/* Path name */
	char		*file_name;	/* File name */
	int		*status;	/* Status flag */
	int		flag;		/* Control flag */
{
	char		name[MAX_STRING]; /* Temporary name */
	register Node	node;		/* Node to output */
	register FILE	*out_file;	/* Output file */

	clear_status(status);
	node = path_lookup(path_name, status);
	if (node == NULL)
		return;
	if (file_name == NULL){
		if ((file_name = getenv("DEFAULTS_FILE")) == NULL)  {
			file_name = name;
			(void)strcpy(file_name, get_login_directory());
			(void)strcat(file_name, "/.defaults");
		}   else   {
			strcpy(name,file_name);
			file_name = name;
		}
	}
	out_file = fopen(file_name, "w");
	if (out_file == NULL){
		warn("Could not open %s", file_name);
		return;
	}
	(void)fprintf(out_file, "%s %d\n", VERSION_TAG, VERSION_NUMBER);
	node_write1(out_file, node, flag);
	(void)fclose(out_file);
}

/*
 * node_write1(out_file, node, flag) will write out the contents of Node to
 * Out_File.  Flag is 0, if the entire database is to be written out.
 * Flag is 1 if only the changed entries are to be written out.  Flag is 2,
 * if only differences from the master database are to be written out.
 */
static void
node_write1(out_file, node, flag)
	register FILE	*out_file;	/* Output file */
	register Node	node;		/* Node to output */
	register int	flag;		/* TRUE => entire database */
{
	char	temp[MAX_STRING];	/* Temporary name */

	if (!node->deleted && ((flag == 0) || ((flag == 2) && node->private) ||
	    !symbol_equal(node->value, node->default_value))){
		(void)name_unparse(node->name, temp);
		(void)fprintf(out_file, "%s", temp);
		if (strcmp(node->value, DEFAULTS_UNDEFINED) != 0){
			(void)fprintf(out_file, "\t");
			str_write(out_file, node->value);
		}
		putc('\n', out_file);
	}
	for (node = node->child; node != NULL; node = node->next)
		node_write1(out_file, node, flag);
}

/*
 * path_lookup(path_name, status) will lookup Path_Name in the defaults
 * database and return the associated node.  If Path_Name is not in the
 * database, a warning message will be printed on the console and NULL will
 * be returned.
 */
static Node
path_lookup(path_name, status)
	char		*path_name;	/* Full name */
	int		*status;	/* Status flag */
{
	register Node	node;		/* Resultant node */

	node = node_lookup_path(path_name, True, status);
	if (node == NULL && defaults->database_dir)
		warn("'%s' is not in defaults database", path_name);
	return node;
}

/*
 * read(file_name, prefix, private) will read in data into the defaults
 * database from the file File_Name.  Only path names that begin with Prefix
 * will be read into the database.  If Private is True, the file being read
 * corresponds to a private database file.  True will be returned if File_Name
 * could not be opened.  Otherwise, False will be returned.
 */
static Bool
read(file_name, prefix, private)
	char		*file_name;	/* File name to read */
	Name		prefix;		/* Prefix path that must match first */
	Bool		private;	/* TRUE => mark nodes as private */
{
	register int	chr;		/* Temporary character */
	Symbol		default_name[MAX_NAME];/* Default name */
	Bool		error;		/* TRUE => error has occured */
	char		huge[HUGE_STRING]; /* Huge string for W. Teitelman */
	register FILE	*in_file;	/* Input file name */
	register int	line;		/* Current line number */
	Symbol		name[MAX_NAME];	/* Full name of node */
	register Name	name_pointer;	/* Pointer into name */
	register Node	node;		/* Node associated with name */
	register Name	prefix_pointer;	/* Pointer into prefix */
	Hash		symbols;	/* String table */
	char		temp[MAX_STRING];/* Temporary symbol */
	char		*value;		/* Node value */
	int		version;	/* Version number */

	in_file = fopen(file_name, "r");
	if (in_file == NULL)
		return True;
	line = 1;

	/* Make sure the version number is correct. */
	error = False;
	if (parse_symbol(in_file, temp) == NULL)
		error = True;
	else {	version = parse_int(in_file, &error);
		(void)parse_eol(in_file);
	}
	if (error || !symbol_equal(temp, VERSION_TAG)){
		warn("Line 1 in %s is not a correctly formatted defaults file",
		    file_name);
		(void)fclose(in_file);
		return False;
	}
	if (version != VERSION_NUMBER){
		warn("Line 1 in %s has version number %d rather than %d",
			file_name, version, VERSION_NUMBER);
		(void)fclose(in_file);
		return False;
	}
	if (defaults == NULL)
		create_defaults();
	symbols = defaults->symbols;
	default_name[0] = NULL;
	while (True){
		/* Dispatch on first printing character. */
		chr = parse_whitespace(in_file);
		if ((chr == '\n') || (chr == ';')){
			(void)parse_eol(in_file);
			line++;
			continue;
		}
		if (chr == EOF)
			break;

		/* Grab the name */
		if (name_parse(in_file, symbols, default_name, name) == 0){
			warn("Could not parse a node name on line %d from %s",
			    line, file_name);
			(void)parse_eol(in_file);
			line++;
			continue;
		}

		/* See whether name matches prefix. */
		name_pointer = name;
		prefix_pointer = prefix;
		while (*prefix_pointer == *name_pointer){
			prefix_pointer++;
			name_pointer++;
		}
		if (*prefix_pointer != NULL){
			/* Prefix does not match. */
			(void)parse_eol(in_file);
			line++;
			continue;
		}

		/* Prefix matches, so look up node and enter value. */
		node = node_find_name(name, False, (int *)NULL);
		if (node == NULL){
			/* This really should not happen. */
			(void)name_unparse(name, temp);
			warn("Problem entering '%s' into database on line %d",
			    temp, line);
			(void)parse_eol(in_file);
			line++;
			break;
		}
		chr = parse_whitespace(in_file);
		value = NULL;
		if (chr == '\n')
			value = DEFAULTS_UNDEFINED;
		else if (chr == '"'){
			value = parse_string(in_file, huge, HUGE_STRING);
			if (value != NULL)
				value = strdup(huge);
		}
		if (value == NULL){
			(void)name_unparse(name, temp);
			warn("%s Could not be assigned a value", temp);
			value = "";
		}
		if (private){
			node->private = True;
			node->value = value;
		} else {
			node->default_value = value;
			if (!node->private)
				node->value = value;
		}
		(void)parse_eol(in_file);
		line++;
	}
	(void)fclose(in_file);
	return False;
}

/*
 * read_lazy(name) will cause all the nodes associated with Name to be read
 * in, if they have not already been read in.
 */
static void
read_lazy(name)
	Name		name;		/* Name to use */
{
	register Node	node;		/* Temporary node */
	Symbol		temp_name[2];	/* Temporary name */

	temp_name[0] = name[0];
	temp_name[1] = NULL;
	node = (Node)hash_lookup(defaults->nodes, temp_name);
	if ((node != NULL) && (node->file))
		read_master(name[0]);
}

/*
 * read_master(file_name) will read in File_Name from the master database.
 */
static void
read_master(file_name)
	char		*file_name;		/* File name to read in */
{
	register Node	node;			/* Database node */
	char		name[MAX_STRING];	/* Temporary file name */
	Symbol		prefix[2];		/* Empty prefix */

	if (defaults->database_dir == NULL)
		return;
	prefix[0] = symbol_lookup(file_name);
	prefix[1] = NULL;
	node = (Node)hash_lookup(defaults->nodes, prefix);
	if (node != NULL)
		node->file = False;

	/* Read from private database. */
	if (defaults->private_dir != NULL){
		(void)strcpy(name, defaults->private_dir);
		(void)strcat(name, file_name);
		(void)strcat(name, ".d");
		prefix[0] = NULL;
		if (!read(name, prefix, False))
			return;
	}

	/* Read from master database. */
	(void)strcpy(name, defaults->database_dir);
	(void)strcat(name, file_name);
	(void)strcat(name, ".d");
	prefix[0] = NULL;
	if (read(name, prefix, False))
		warn("Could not open database file '%s'", name);
}

/*
 * read_master_database() will find the master database directory and enter
 * nodes into the database for each file in the master database directory.
 */
static void
read_master_database()
{
	char	*database_dir;		/* Database directory name */
	Node	node;			/* Current node */
	char	*private_dir;		/* Private database directory */

	/* Find the defaults directory. */
	database_dir = defaults->database_dir;
	if (database_dir == NULL){
		node = node_lookup_path("/Defaults/Directory", True, (int *)NULL);
		if (node == NULL)
			database_dir = "/usr/lib/defaults/";
		else 	database_dir = slash_append(node->value);
		defaults->database_dir = database_dir;
	}
	read_master_database1(database_dir);

	/* Find the private defaults directory. */
	private_dir = defaults->private_dir;
	if (private_dir == NULL){
		node = node_lookup_path("/Defaults/Private_Directory",
								True, (int *)NULL);
		if (node == NULL)
			private_dir = NULL;
		else	private_dir = slash_append(node->value);
		defaults->private_dir = private_dir;
	}
	if (private_dir != NULL)
		read_master_database1(private_dir);
}

/*
 * read_master_database1(dir_name) will read in the *.d file names contained
 * in the Dir_Name directory.
 */
static void
read_master_database1(dir_name)
	char			*dir_name;	/* Directory name */
{
	register DIR		*dir;		/* Default directory */
	register struct direct	*dir_entry;	/* Directory entry */
	Symbol			name[MAX_NAME];	/* Name to create */
	register Node		node;		/* Current node */
	register char		*pointer;	/* Pointer into file name */
	int			size;		/* Size of string */
	char			symbol[MAX_STRING];  /* Temporary symbol */

	dir = opendir(dir_name);
	if (dir == NULL){
		warn("Could not find defaults database directory %s\n", dir_name);
		warn("Supressing subsequent error messages.\n");
		defaults->errors = 1000000;	/* A big number */
		return;
	}
	
	/* Scan the database for all the top level nodes. */
	name[1] = NULL;
	while (True){
		dir_entry = readdir(dir);
		if (dir_entry == NULL)
			break;
		pointer = dir_entry->d_name;
		size = strlen(pointer);
		if ((size < 3) || (strcmp(&pointer[size - 2], ".d") != 0))
			continue;
		(void)strcpy(symbol, pointer);
		symbol[size - 2] = '\0';
		name[0] = symbol_lookup(symbol);
		node = node_lookup_name(name, False, (int *)NULL);
		if (node == NULL)
			node = node_find_name(name, False, (int *)NULL);
		if (node == NULL)
			warn("Could not create node '%s'\n");
		else	node->file = True;
	}
	closedir(dir);
}

/*
 * read_private_database() will read in env var DEFAULTS_FILE
 */
static void
read_private_database()
{
	char	file_name[MAX_STRING];	/* Local defaults file name */
	Symbol	prefix[1];		/* Empty prefix */
	char    *temp;

	file_name[0] = '0';
	if ((temp = getenv("DEFAULTS_FILE")) != NULL)  {
		(void)strcpy(file_name,temp);
	}   else   {
		(void)strcpy(file_name, get_login_directory());
		(void)strcat(file_name, "/.defaults");
	}
	prefix[0] = NULL;
	read(file_name, prefix, True);
}

/*
 * set(path_name, new_value) will set Full_Name to type
 * Node_Type with value New_Value.
 */
static void
set(path_name, new_value, status)
	char		*path_name;	/* Full node name */
	char		*new_value;	/* New value */
	int		*status;	/* Status flag */
{
	Symbol		name[MAX_NAME];	/* Name */
	register Node	node;		/* Node to set */

	if (defaults == NULL)
		create_defaults();	

	/* Get the node assoicated with Full_Name. */
	name_quick_parse(path_name, name);
	node = node_find_name(name, True, status);
	if (node == NULL){
		warn("Can not bind node to '%s'", path_name);
		return;
	}

	/* It is not worth the effort to try to deallocate any storage. */
	node->value = new_value;
	node->private = True;
	node->deleted = False;
}

/*
 * slash_append(text) will return make sure that Text has a slash ('/')
 * on the end.
 */
static char *
slash_append(text)
	register char	*text;		/* String to append to */
{
	char temp[MAX_STRING];		/* Maximum string length */

	if (text[strlen(text) - 1] == '/')
		return text;
	(void)strcpy(temp, text);
	(void)strcat(temp, "/");
	return symbol_copy(temp);
}

/*
 * str_write(out_file, string) will write String to Out_File as a C-style
 * string.
 */
static void
str_write(out_file, string)
	register FILE	*out_file;	/* Output file */
	register char	*string;	/* String to output */
{
	register int	chr;		/* Temporary character */

	putc('"', out_file);
	while (True){
		chr = *string++;
		if (chr == '\0')
			break;
		if ((' ' <= chr) && (chr <= '~') &&
		    (chr != '\\') && (chr != '"')){
			putc(chr, out_file);
			continue;
		}
		putc('\\', out_file);
		switch (chr){
		    case '\n':
			chr = 'n';
			break;
		    case '\r':
			chr = 'r';
			break;
		    case '\t':
			chr = 't';
			break;
		    case '\b':
			chr = 'b';
			break;
		    case '\\':
			chr = '\\';
			break;
		    case '\f':
			chr = 'f';
			break;
		    case '"':
			chr = '"';
			break;
		}
		if ((' ' <= chr) && (chr <= '~'))
			putc(chr, out_file);
		else	(void)fprintf(out_file, "%03o", chr & 0377);
	}
	putc('"', out_file);
}

/*
 * symbol_copy(symbol) will make and return a copy of Symbol.
 */
static char *
symbol_copy(symbol)
	char	 *symbol;		/* Symbol to copy */
{
	register char *new;		/* New symbol */

	new = get_memory(strlen(symbol) + 1);
	(void)strcpy(new, symbol);
	return new;
}

/*
 * symbol_equal(symbol1, symbol2) will return True if Symbol1 equals Symbol2.
 */
static Bool
symbol_equal(symbol1, symbol2)
	register char	*symbol1;	/* First symbol */
	register char	*symbol2;	/* Second symbol */
{
	register char	chr1;		/* Character from first symbol */
	register char	chr2;		/* Character from second symbol */

	while (True){
		chr1 = *symbol1++;
		if (('A' <= chr1) && (chr1 <= 'Z'))
			chr1 += 'a' - 'A';
		chr2 = *symbol2++;
		if (('A' <= chr2) && (chr2 <= 'Z'))
			chr2 += 'a' - 'A';
		if (chr1 != chr2)
			return False;
		if (chr1 == '\0')
			return True;
	}
}

/*
 * symbol_hash(symbol) will return a hash of Symbol.
 */
static int
symbol_hash(symbol)
	Symbol	symbol;		/* Symbol to hash */
{
	register char	chr;		/* Character from symbol */
	register int	hash;		/* Hash value */

	hash = 0;
	do {	chr = *symbol++;
		if (('A' <= chr) && (chr <= 'Z'))
			chr += 'a' - 'A';
		hash += chr;
	} while (chr != '\0');
	return hash;
}

/*
 * symbol_lookup(symbol) will return the conical value for Symbol from
 * the symbols hash table.
 */
static Symbol
symbol_lookup(symbol)
	Symbol		symbol;		/* Symbol to lookup */
{
	register Symbol	new;		/* New symbol */

	if (defaults == NULL)
		create_defaults();
	new = (char *)hash_lookup(defaults->symbols, (caddr_t)symbol);
	if (new == NULL){
		new = symbol_copy(symbol);
		(void)hash_insert(defaults->symbols, (caddr_t)new, (caddr_t)new);
	}
	return new;
}

/*
 * warn(format, arg1, arg2, arg3) will print out a warning message
 * to the console using Format, Arg1, Arg2 and Arg3.  This is all provided
 * the number of errors has not gotten too excessive.
 */
/*VARARGS1*/
static void
warn(format, arg1, arg2, arg3, arg4)
	char	*format;/* Format string */
	long	arg1;	/* First argument */
	long	arg2;	/* Second argument */
	long	arg3;	/* Third argument */
	long	arg4;	/* Fourth argument */
{
	static Defaults_pairs error_action[] = {
		"Continue", 1,
		"Abort", 2,
		"Suppress", 3,
		NULL, 1,
	};
	register int	action;			/* Action number */
	int		errors;			/* Number of errors */
	int		max_errors;		/* Maximum errors */
	register Node	node;			/* Error action node */
	char 		temp[MAX_STRING];	/* Temporary string */

	action = 1;
	if (defaults == NULL)
		errors = 0;
	else {	errors = ++defaults->errors;
		/* See how many errors to accept. */
		(void)strcpy(temp, SEP_STR);
		(void)strcat(temp, "Defaults");
		(void)strcat(temp, SEP_STR);
		(void)strcat(temp, "Maximum_Errors");
		node = node_lookup_path(temp, True, (int *)NULL);
		if (node == NULL)
			max_errors = MAX_ERRORS;
		else	max_errors = atoi(node->value);
		if (defaults->errors == max_errors)
			format = "Supressing all subsequent errors";

		/* See what to do on error action. */
		(void)strcpy(temp, SEP_STR);
		(void)strcat(temp, "Defaults");
		(void)strcat(temp, SEP_STR);
		(void)strcat(temp, "Error_Action");
		node = node_lookup_path(temp, True, (int *)NULL);
		if (node != NULL)
			action = defaults_lookup(node->value, error_action);
	}
	if ((errors <= max_errors) && (action < 3)){
		(void)fprintf(stderr, "SunDefaults Error: ");
		(void)fprintf(stderr, format, arg1, arg2, arg3, arg4);
		(void)fprintf(stderr, "\n");
		(void)fflush(stderr);
	}
	if (action == 2)
		bomb();
}

