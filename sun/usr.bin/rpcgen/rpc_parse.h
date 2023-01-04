/* @(#)rpc_parse.h 1.1 86/09/25 (C) 1986 SMI */

/*
 * rpc_parse.h, Definitions for the RPCL parser 
 * Copyright (C) 1986, Sun Microsystems, Inc.
 */

enum defkind {
	DEF_ARRAY,
	DEF_STRUCT,
	DEF_UNION,
	DEF_ENUM,
	DEF_TYPEDEF,
	DEF_PROGRAM
};
typedef enum defkind defkind;

struct array_def {
	char *len_name;
	char *array_name;
	char *array_prefix;
	char *array_type;
	char *array_max;	
};
typedef struct array_def array_def;

enum relation {
	REL_VECTOR,
	REL_POINTER,
	REL_ALIAS,
};
typedef enum relation relation;

struct typedef_def {
	char *old_prefix;
	char *old_type;
	relation rel;
	char *array_max;
};
typedef struct typedef_def typedef_def;


struct enumval_list {
	char *name;
	char *assignment;
	struct enumval_list *next;
};
typedef struct enumval_list enumval_list;

struct enum_def {
	enumval_list *vals;
};
typedef struct enum_def enum_def;


struct declaration {
	char *prefix;
	char *type;
	char *name;
	relation rel;
	char *array_max;
};
typedef struct declaration declaration;


struct decl_list {
	declaration decl;
	struct decl_list *next;
};
typedef struct decl_list decl_list;

struct struct_def {
	decl_list *decls;
};
typedef struct struct_def struct_def;


struct case_list {
	char *case_name;
	declaration case_decl;
	struct case_list *next;
};
typedef struct case_list case_list;

struct union_def {
	declaration enum_decl;
	case_list *cases;
	declaration *default_decl;
};
typedef struct union_def union_def;



struct proc_list {
	char *proc_name;
	char *proc_num;
	char *arg_type;
	char *arg_prefix;
	char *res_type;
	char *res_prefix;
	struct proc_list *next;
};
typedef struct proc_list proc_list;


struct version_list {
	char *vers_name;
	char *vers_num;	
	proc_list *procs;
	struct version_list *next;
};
typedef struct version_list version_list;

struct program_def {	
	char *prog_num;
	version_list *versions;
};
typedef struct program_def program_def;

struct definition {
	char *def_name;
	defkind def_kind;
	union {
		array_def ar;
		struct_def st;
		union_def un;
		enum_def en;
		typedef_def ty;
		program_def pr;
	} def;
};
typedef struct definition definition;

definition *get_definition();
