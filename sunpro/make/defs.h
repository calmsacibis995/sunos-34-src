/*	@(#)defs.h 1.3 87/04/17 SMI	*/

/*
 *	Copyright (c) 1986 Sun Microsystems, Inc.	[Remotely from S5R2]
 */

#include <stdio.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/time.h>
#include <values.h>
#include <vroot.h>
#ifdef sparc
#  include <alloca.h>
#endif

#define AR_MEMBER_NAME_LEN	15
#define HASHSIZE		2048

#define AMPERSAND		'&'
#define ASTERISK		'*'
#define AT			'@'
#define BACKSLASH		'\\'
#define BAR			'|'
#define BRACELEFT		'{'
#define BRACERIGHT		'}'
#define BRACKETLEFT		'['
#define BRACKETRIGHT		']'
#define COLON			':'
#define COMMA			','
#define DOLLAR			'$'
#define EQUAL			'='
#define EXCLAM			'!'
#define GREATER			'>'
#define HYPHEN			'-'
#define NEWLINE			'\n'
#define NUL			'\0'
#define NUMBERSIGN		'#'
#define PARENLEFT		'('
#define PARENRIGHT		')'
#define PERCENT			'%'
#define PERIOD			'.'
#define PLUS			'+'
#define QUESTION		'?'
#define SEMICOLON		';'
#define SLASH			'/'
#define SPACE			' '
#define TAB			'\t'
#define TILDE			'~'

/* This macro supplies the arguments printf() needs to indent trace output */
#define BLANKS recursion_level, ""

#define max(a,b)		((a)>(b)?(a):(b))
#define out_of_date(a,b)	(((a) < (b)) || (((a) == 0) && ((b) == 0)))
#define setvar(name, value, append) \
				setvar_daemon(name, value, append, no_daemon)
#define getname(a,b)		getname_fn((a), (b), 0)
#define FIND_LENGTH		-1
#define is_equal(a,b)		(!strcmp((a), (b)))
#define is_equaln(a,b,n)	(!strncmp((a), (b), (n)))

typedef time_t			Timetype;
#define FILE_NO_TIME		-1
#define FILE_DOESNT_EXIST	0
#define FILE_IS_DIR		1
#define FILE_MAX_TIME		MAXINT

typedef enum {
	false= 0,
	true= 1,
	failed= 0,
	succeeded= 1
}				Boolean;
#define boolean(expr)		((expr) ? true:false)
#define is_true(expr)		((expr) != false)
#define is_false(expr)		((expr) == false)

typedef struct {
	Boolean			continue_after_error:8;		/* `-k' */
	Boolean			do_not_exec_rule:8;		/* `-n' */
	Boolean			env_wins:8;			/* `-e' */
	Boolean			ignore_default_mk:8;		/* `-r' */
	Boolean			ignore_errors:8;		/* `-i' */
	Boolean			interactive:8;			/* `-l' */
	Boolean			keep_state:8;			/* KEEP_STATE */
	Boolean			report_dependencies:8;		/* -P */
	Boolean			report_pwd:8;
	Boolean			trace_reader:8;			/* `-D' */
	Boolean			trace_status:8;			/* `-p' */
	Boolean			quest:8;			/* `-q' */
	Boolean			silent:8;			/* `-s' */
	Boolean			touch:8;			/* `-t' */
} Flag;

/* Bits stored in funny vector to classify chars */
#define DOLLAR_FUNNY		0001
#define META_FUNNY		0002
#define PERCENT_FUNNY		0004
#define WILDCARD_FUNNY		0010
#define COMMAND_PREFIX_FUNNY	0020
#define SPECIAL_MACRO_FUNNY	0040

/* Flags passed to the getname function */
#define DONT_ENTER		1
#define FILE_TYPE		2

/* Type returned from doname class functions */
typedef enum {
	build_dont_know= 0,
	build_failed,
	build_ok,
	build_in_progress
}				Doname;

/* The String struct defines a string with the following layout
	"xxxxxxxxxxxxxxxCxxxxxxxxxxxxxxx________________"
	^		^		^		^
	|		|		|		|
	buffer.start	text.p		text.end	buffer.end
	text.p points to the next char to read/write.
 */
typedef struct {
	struct Text {
		char		*p;	/* Read/Write pointer */
		char		*end;	/* Read limit pointer */
	}		text;
	struct Physical_buffer {
		char		*start;	/* Points to start of buffer */
		char		*end;	/* End of physical buffer */
	}		buffer;
	Boolean		free_after_use:1;
} String;

#define STRING_BUFFER_LENGTH	1024
#define init_string_from_stack(str, buf) { \
			str.buffer.start= str.text.p= buf; \
			str.text.end= NULL; \
			str.buffer.end= buf+sizeof(buf); \
			str.free_after_use= false;}

#define alloc(x)		((struct x *)Malloc(sizeof(struct x)))

typedef struct Chain		*Chain;
typedef struct Cmd_line		*Cmd_line;
typedef struct Dependency	*Dependency;
typedef struct Name		*Name;
typedef struct Percent		*Percent;
typedef struct Property		*Property;
typedef struct Source		*Source;

/* Used for storing the $? list and also for the "target + target:" construct */
struct Chain {
	Chain			next;
	Name			name;
};

/* Stores one command line for a rule */
struct Cmd_line {
	Cmd_line		next;
	Name			command_line;
	Boolean			make_refd:1;	/* $(MAKE) referenced? */
	/* Remeber command line prefixes */
	Boolean			ignore_command_dependency:1;	/* `?' */
	Boolean			assign:1;			/* `=' */
	Boolean			ignore_error:1;			/* `-' */
	Boolean			silent:1;			/* `@' */
};

/* Linked list of targets/files */
struct Dependency {
	Dependency		next;
	Name			name;
	Boolean			automatic:1;
	Boolean			stale:1;
};

/* The specials are markers for targets that the reader should special case */
typedef enum {
	no_special= 0,
	ar_replace_special,
	built_last_make_run_special,
	default_special,
	ignore_special,
	keep_state_special,
	make_version_special,
	precious_special,
	sccs_get_special,
	silent_special,
	suffixes_special,
	sym_link_to_special
}				Special;
enum Separator {
	no_colon= 0,
	one_colon,
	two_colon,
	equal_seen,
	conditional_seen,
	none_seen
};
typedef enum Member_class {
	not_member= 0,
	old_member,
	new_member
}				Member_class;
struct Name {
	Name			next;		/* pointer to next Name */
	Property		prop;		/* List of properties */
	char			*string;	/* ASCII name string */
	struct {
		unsigned int		length;
		unsigned int		sum;
	}                       hash;
	struct {
		Timetype		time;		/* Modification */
		int			errno;		/* error from "stat" */
		unsigned int		size;		/* Of file */
		unsigned short		mode;		/* Of file */
		Boolean			is_file:1;
		Boolean			is_dir:1;
		Boolean			is_sym_link:1;
		Boolean			should_be_sym_link:1;
		Boolean			is_precious:1;
		enum {
			dont_know_sccs= 0,
			no_sccs,
			has_sccs
		}			has_sccs:2;
	}                       stat;
	/* Count instances of :: definitions for this target */
	short			colon_splits;
	/* We only clear the automatic depes once per target per report */
	short			temp_file_number;
	/* Count how many conditional macros this target has defined */
	short			conditional_cnt;
	/* A conditional macro was used when building this target */
	Boolean			depends_on_conditional:1;
	Boolean			has_member_depe:1;
	Member_class		member_class:2;
	/* This target is a directory that has been read */
	Boolean			has_read_dir:1;
	/* This name is a macro that is now being expanded */
	Boolean			being_expanded:1;
	/* This name is a magic name that the reader must know about */
	Special			special_reader:4;
	Doname			state:2;
	enum Separator		colons:2;
	Boolean			has_depe_list_expanded:1;
	Boolean			suffix_scan_done:1;
	Boolean			has_complained:1;	/* For sccs */
	/* This target has been built during this make run */
	Boolean			has_built:1;
	Boolean			with_squiggle:1;	/* for .SUFFIXES */
	Boolean			without_squiggle:1;	/* for .SUFFIXES */
	Boolean			has_read_suffixes:1;	/* Suffix list cached */
	Boolean			has_suffixes:1;
	Boolean			has_target_prop:1;
	Boolean			has_vpath_alias_prop:1;
	Boolean			dependency_printed:1;	/* For printdesc() */
	Boolean			dollar:1;		/* In namestring */
	Boolean			meta:1;			/* In namestring */
	Boolean			percent:1;		/* In namestring */
	Boolean			wildcard:1;		/* In namestring */
	Boolean			has_recursive_dependency:1;
};

/* Stores the % matched default rules */
struct Percent {
	Percent			next;
	Name			target_prefix;
	Name			target_suffix;
	Name			source_prefix;
	Name			source_suffix;
	Cmd_line		command_template;
	Boolean			being_expanded:1;
	Boolean			source_percent:1;
};

/* Each Name has a list of properties */
/* The properties are used to store information that only */
/* a subset of the Names need */
typedef enum {
	no_prop,
	conditional_prop,
	line_prop,
	macro_prop,
	makefile_prop,
	member_prop,
	recursive_prop,
	sccs_prop,
	suffix_prop,
	sym_link_to_prop,
	target_prop,
	vpath_alias_prop
}				Property_id;

#define PROPERTY_HEAD_SIZE (sizeof(struct Property)-sizeof(union Body))
struct Property {
	Property		next;
	Property_id		type:4;
	union Body {
		struct Conditional {
			/* For "foo := ABC [+]= xyz" constructs */
			/* Name "foo" gets one conditional prop */
			Name			name;
			Name			value;
			Boolean			append:1;
		}			conditional;
		struct Line {
			/* For "target : dependencies" constructs */
			/* Name "target" gets one line prop */
			Cmd_line		command_template;
			Cmd_line		command_used;
			Dependency		dependencies;
			Timetype		dependency_time;
			Chain			target_group;
			Boolean			is_out_of_date:1;
			Boolean			sccs_command:1;
			Boolean			command_template_redefined:1;
			/* Values for the dynamic macros */
			Name			target;
			Name			star;
			Name			less;
			Name			percent;
			Chain			query;
		}			line;
		struct Macro {
			/* For "ABC = xyz" constructs */
			/* Name "ABC" get one macro prop */
			Name			value;
			Boolean			exported:1;
			Boolean			read_only:1;
			/* This macro is defined conditionally */
			Boolean			is_conditional:1;
			/*
			 * The list for $? is stored as a structured list that
			 * is translated into a string iff it is referenced.
			 * This is why  some macro values need a daemon. 
			 */
			enum daemon {
				no_daemon= 0,
				chain_daemon
			}			daemon:2;
		}			macro;
		struct Makefile {
			/* Names that reference makefiles gets one prop */
			char			*contents;
			int			size;
		}			makefile;
		struct Member {
			/* For "lib(member)" and "lib((entry))" constructs */
			/* Name "lib(member)" gets one member prop */
			/* Name "lib((entry))" gets one member prop */
			/* The member field is filled in when the prop is refd*/
			Name			library;
			Name			entry;
			Name			member;
		}			member;
		struct Recursive {
			/* For "target: .RECURSIVE dir makefiles" constructs */
			/* Used to keep track of recursive calls to make */
			/* Name "target" gets one recursive prop */
			Name			directory;
			Name			target;
			Dependency		makefiles;
		}			recursive;
		struct Sccs {
			/* Each file that has a SCCS s. file gets one prop */
			Name			file;
		}			sccs;
		struct Suffix {
			/* Cached list of suffixes that can build this target */
			/* suffix is built from .SUFFIXES */
			Name			suffix;
			Cmd_line		command_template;
		}			suffix;
		struct Sym_link_to {
			/* For "foo: .SYM_LINK_TO fie" constructs */
			/* Name "foo" gets one sym_link_to prop */
			/* This forces make to build the link *and* it is a */
			/* regular dependency */
			Name			link_to;
		}			sym_link_to;
		struct Target {
			/* For "target:: dependencies" constructs */
			/* The "::" construct is handled by converting it to */
			/* "foo: 1@foo" + "1@foo: dependecies" */
			/* "1@foo" gets one target prop */
			/* This target prop cause $@ to be bound to "foo" */
			/* not "1@foo" when the rule is evaluated */
			Name			target;
		}			target;
		struct Vpath_alias {
			/* If a file was found using the VPATH it gets a vpath_alias prop */
			Name			alias;
		}			vpath_alias;
	}			body;
};

/* Macros for the reader */
#define uncache_source()	if (source != NULL) \
					source->string.text.p= source_p
#define cache_source(comp)	if (source != NULL) { \
					source_p= source->string.text.p-(comp);\
					source_end= source->string.text.end;}
#define get_next_block(source)	{ uncache_source(); \
				 source= get_next_block_fn(source); \
				 cache_source(0)}
#define get_char()		((source == NULL) || \
				(source_p >= source_end)?0:*source_p)
struct Source {
	String			string;
	Source			previous;
	int			bytes_left_in_file;
	short			fd;
	Boolean			already_expanded:1;
};

typedef struct {
	Name				ar_replace;
	Name				built_last_make_run;
	Name				c_at;
	Name				current_make_version;
	Name				default_makefile;
	Name				default_rule;
	Name				dollar;
	Name				done;
	Name				dot;
	Name				dotdot;
	Name				dot_a;
	Name				dot_keep_state;
	Name				empty_suffix_name;
	Name				force;
	Name				host_arch;
	Name				ignore;
	Name				init;
	Name				keep_state;
	Name				less;
	Name				make;
	Name				make_state;
	Name				makefile;
	Name				Makefile;
	Name				makeflags;
	Name				make_version;
	Name				mflags;
	Name				path;
	Name				percent;
	Name				plus;
	Name				precious;
	Name				query;
	Name				quit;
	Name				recursive;
	Name				sccs_get;
	Name				sh;
	Name				shell;
	Name				silent;
	Name				standard_in;
	Name				star;
	Name				suffixes;
	Name				sunpro_dependencies;
	Name				sym_link_to;
	Name				target_arch;
	Name				usr_include;
	Name				usr_include_sys;
	Name				virtual_root;
	Name				vpath;
}				Cached_names;

typedef enum {
	reading_nothing,
	reading_makefile,
	reading_statefile,
	rereading_statefile,
	reading_cpp_file
}				Makefile_type;

/*
 *	extern declarations for all global variables.
 *	The actual declarations are in main.c
 */
extern Cmd_line		ar_replace_rule;
extern Boolean		assign_done;
extern Boolean		built_last_make_run_seen;
extern Cached_names	cached_names;
extern Property		current_line;
extern Boolean		command_changed;
extern Boolean		commands_done;
extern Name		current_path;
extern Name		current_target;
extern short		debug_level;
extern Cmd_line		default_rule;
extern Name		default_target_to_build;
extern int		depth;
extern int		file_number;
extern FILE		*fin;
extern Flag		flag;
extern char		funny[];
extern Name		hashtab[];
extern Boolean		is_conditional;
extern Boolean		is_out_of_date;
extern int		line_length;
extern Makefile_type	makefile_type;
extern pathpt		makefile_path;
extern Dependency	makefiles_used;
extern String		makeflags_string;
extern Boolean		make_word_mentioned;
extern Percent		percent_list;
extern int		process_running;
extern Boolean		query_mentioned;
extern short		read_trace_level;
extern int		recursion_level;
extern Boolean		rewrite_statefile;
extern Cmd_line		sccs_get_rule;
extern int		(*sigivalue)();
extern int		(*sigqvalue)();
extern Dependency	suffixes;
extern Cmd_line		sym_link_to_rule;
extern char		*target_name;
extern Name		temp_file_name;
extern short		temp_file_number;
extern Boolean		vpath_defined;
extern Boolean		working_on_targets;

extern char		**environ;
extern int		errno;
extern char		*sys_siglist[];

extern char		*alloca();
extern char		*getenv();
extern char		*index();
extern char		*rindex();
extern char		*setenv();
extern int		sleep();
extern char		*sprintf();
extern char		*strcat();
extern char		*strcpy();
extern char		*strncat();
extern char		*strncpy();

extern void		append_char();
extern Property		append_prop();
extern void		append_string();
extern void		build_suffix_list();
extern void		check_current_path();
extern void		check_read_state_file();

extern Boolean		await();
extern Doname		doname();
extern Doname		doname_check();
extern Boolean		dosys();
extern void		enable_interrupt();
extern void		enter_dependency();
extern void		enter_equal();
extern char		*errmsg();
extern Timetype		exists();
extern void		expand_macro();
extern void		expand_string();
extern void		expand_value();
extern void		fatal();
extern void		fatal_reader();
extern void		warning();
extern void		Free();
extern Source		get_next_block_fn();
extern Property		get_prop();
extern Name		getname_fn();
extern char		*getwd();
extern Name		getvar();
extern int		handle_interrupt();
extern void		print_dependencies();
extern Timetype		read_archive();
extern char		*Malloc();
extern Property		maybe_append_prop();
extern void		read_dependency_file();
extern Boolean		read_simple_file();
extern Property		setvar_daemon();
extern void		read_dir();
extern void		write_state_file();
extern char		*time_to_string();
extern pathpt		vroot_path;
extern void		lock_file();
extern char		*lock_file_name;
