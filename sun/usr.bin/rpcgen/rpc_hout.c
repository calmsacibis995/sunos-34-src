#ifndef lint 
static char sccsid[] = "@(#)rpc_hout.c 1.1 86/09/25 (C) 1986 SMI";
#endif
 
/*
 * rpc_hout.c, Header file outputter for the RPC protocol compiler
 * Copyright (C) 1986, Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <ctype.h>
#include "rpc_util.h"
#include "rpc_parse.h"


/*
 * Print the C-version of an xdr definition
 */
void
print_datadef(def)
	definition *def;
{
	
	fprintf(fout,"\n");
	switch (def->def_kind) {
	case DEF_STRUCT:
		pstructdef(def);
		break;
	case DEF_UNION:
		puniondef(def);
		break;
	case DEF_ARRAY:
		parraydef(def);
		break;
	case DEF_ENUM:
		penumdef(def);
		break;
	case DEF_TYPEDEF:
		ptypedef(def);
		break;
	case DEF_PROGRAM:
		pprogramdef(def);
		break;
	}
	fprintf(fout,"\n");
}

/*
 * Declare all of the functions that were written
 */
void
print_funcdefs()
{
	list *l;
	definition *def;

	for (l = defined; l != NULL; l = l->next) {
		def = (definition *) l->val;
		if (def->def_kind != DEF_PROGRAM) {
			fprintf(fout,"bool_t xdr_%s();\n",def->def_name);
		}
	}
}

static
pstructdef(def)
	definition *def;
{
	decl_list *l;
	char *name = def->def_name;

	fprintf(fout,"struct %s {\n",name);
	for (l = def->def.st.decls; l != NULL; l = l->next) {
		pdeclaration(name,&l->decl,1);
	}
	fprintf(fout,"};\n");
	fprintf(fout,"typedef struct %s %s;\n",name,name);
}

static
puniondef(def)
	definition *def;
{
	case_list *l;
	char *name = def->def_name;
	declaration *decl;

	fprintf(fout,"struct %s {\n",name);
	decl = &def->def.un.enum_decl;
	fprintf(fout,"\t%s %s;\n",decl->type,decl->name);
	fprintf(fout,"\tunion {\n");
	for (l = def->def.un.cases; l != NULL; l = l->next) {
		pdeclaration(name,&l->case_decl,2);
	}
	decl = def->def.un.default_decl;
	if (decl && ! streq(decl->type,"void")) {
		pdeclaration(name,decl,2);
	}
	fprintf(fout,"\t} %s;\n",name);
	fprintf(fout,"};\n");
	fprintf(fout,"typedef struct %s %s;\n",name,name);
}



static
pdefine(name,num)
	char *name;
	char *num;	
{
	
	fprintf(fout,"#define %s %s\n",name,num);
}

static
dprinted(stop,start)
	proc_list *stop;
	version_list *start;
{
	version_list *vers;
	proc_list *proc;

	for (vers = start; vers != NULL; vers = vers->next) {
		for (proc = vers->procs; proc != NULL; proc = proc->next) {
			if (proc == stop) {
				return(0);
			} else if (streq(proc->proc_name,stop->proc_name)) {
				return(1);
			}
		}
	}
	abort();	
	/* NOTREACHED */
}
	
	
static
pprogramdef(def)
	definition *def;
{
	version_list *vers;
	proc_list *proc;

	pdefine(def->def_name,def->def.pr.prog_num);
	for (vers = def->def.pr.versions; vers != NULL; vers = vers->next) {
		pdefine(vers->vers_name,vers->vers_num);
		for (proc = vers->procs; proc != NULL; proc = proc->next) {
			if (! dprinted(proc,def->def.pr.versions)) {
				pdefine(proc->proc_name,proc->proc_num);
			}
		}
	}
}


static
penumdef(def)
	definition *def;
{
	char *name = def->def_name;
	enumval_list *l;
	char *last = NULL;
	int count = 0;

	fprintf(fout,"enum %s {\n",name);
	for (l = def->def.en.vals; l != NULL; l = l->next) {
		fprintf(fout,"\t%s",l->name);
		if (l->assignment) {
			fprintf(fout," = %s",l->assignment);
			last = l->assignment;
			count = 1;
		} else {
			if (last == NULL) {
				fprintf(fout," = %d",count++);
			} else {
				fprintf(fout," = %s + %d",last,count++);
			}
		}	
		fprintf(fout,",\n");
	}
	fprintf(fout,"};\n");
	fprintf(fout,"typedef enum %s %s;\n",name,name);
}

static
parraydef(def)
	definition *def;
{
	char *name = def->def_name;
	char *atype = def->def.ar.array_type;
	char *aname = def->def.ar.array_name;

	fprintf(fout,"struct %s {\n",name);
	fprintf(fout,"\tu_int %s;\n",def->def.ar.len_name);
	if (streq(atype,"opaque")) {
		atype = "char";
	}
	fprintf(fout,"\t%s *%s;\n",atype,aname);
	fprintf(fout,"};\n");
	fprintf(fout,"typedef struct %s %s;\n",name,name);
}
	

static
ptypedef(def)
	definition *def;
{
	char *name = def->def_name;
	char *old = def->def.ty.old_type;
	char *prefix = def->def.ty.old_prefix;
	relation rel = def->def.ty.rel;

	if (! streq(name,old)) {
		if (streq(old,"string")) {
			old = "char";
			rel = REL_POINTER;
		} else if (streq(old,"opaque")) {
			old = "char";
		} else if (streq(old,"bool")) {
			old = "bool_t";
		}	
		fprintf(fout,"typedef ");
		if (undefined2(old,name) && prefix) {
			fprintf(fout,"%s ",prefix);
		}
		fprintf(fout,"%s ",old);
		if (rel == REL_POINTER) {
			fprintf(fout,"*");
		}
		fprintf(fout,name);
		if (rel == REL_VECTOR) {
			fprintf(fout,"[%s]",def->def.ty.array_max);
		}
		fprintf(fout,";\n");
	}
}

static
pdeclaration(name,dec,tab)
	char *name;
	declaration *dec;
	int tab;
{	
	if (streq(dec->type,"void")) {
		return;
	}
	while (tab--) {	
		fprintf(fout,"\t");
	}
	if (streq(dec->type,name) && ! dec->prefix) {
		fprintf(fout,"struct ");
	}
	if (streq(dec->type,"string")) {
		fprintf(fout,"char *%s",dec->name);
	} else {
		if (streq(dec->type,"bool")) {
			fprintf(fout,"bool_t ");
		} else if (streq(dec->type,"opaque")) {
			fprintf(fout,"char ");
		} else {
			if (dec->prefix) {
				if (streq(dec->prefix,"enum")) {
					fprintf(fout,"enum ");
				} else {
					fprintf(fout,"struct ");
				}
			}
			fprintf(fout,"%s ",dec->type);
		}
		if (dec->rel == REL_POINTER) {
			fprintf(fout,"*");
		}
		fprintf(fout,dec->name);
		if (dec->rel == REL_VECTOR) {
			fprintf(fout,"[%s]",dec->array_max);
		}
	}
	fprintf(fout,";\n");
}



static
undefined2(type,stop)
	char *type;
	char *stop;
{
	list *l;
	definition *def;

	for (l = defined; l != NULL; l = l->next) {
		def = (definition *) l->val;
		if (def->def_kind != DEF_PROGRAM) {
			if (streq(def->def_name,stop)) {
				return(1);
			} else if (streq(def->def_name,type)) {
				return(0);
			}
		}
	}
	return(1);
}
