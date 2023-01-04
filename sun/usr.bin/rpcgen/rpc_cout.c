#ifndef lint
static char	sccsid[] = "@(#)rpc_cout.c 1.1 86/09/25 (C) 1986 SMI";
#endif

/*
 * rpc_cout.c, XDR routine outputter for the RPC protocol compiler
 * Copyright (C) 1986, Sun Microsystems, Inc.
 */
#include <stdio.h>
#include <strings.h>
#include "rpc_util.h"
#include "rpc_parse.h"

#define SPECIALDEF 1	/* special if definition created by compiler */
#define NORMALDEF 0		/* normal if definition created by human */


/*
 * Emit the C-routine for the given definition
 */
void
emit(def)
	definition *def;
{
	do_emit(def,NORMALDEF);
}



static
do_emit(def,special)
	definition *def;
	int special;
{

	if (def->def_kind == DEF_PROGRAM) {
		return;
	}
	if (isprinted(def->def_name)) {
		return;
	}
	print_undefineds(def);
	print_header(def,special);
	switch (def->def_kind) {
	case DEF_UNION:
		emit_union(def);
		break;
	case DEF_ARRAY:
		emit_array(def);
		break;
	case DEF_ENUM:
		emit_enum(def);
		break;
	case DEF_STRUCT:	
		emit_struct(def);
		break;	
	case DEF_TYPEDEF:
		emit_typedef(def);
		break;
	}
	print_trailer();
}

static
findtype(def,type)
	definition *def;
	char *type;
{
	if (def->def_kind == DEF_PROGRAM) {
		return(0);
	} else {
		return(streq(def->def_name,type));
	}
}

static
undefined(type)
	char *type;
{
	definition *def;

	def = (definition *) FINDVAL(defined,type,findtype);
	return(def == NULL);
}

static	
isprinted(name)
	char *name;
{
	char *find;

	find = (char *) FINDVAL(printed,name,streq);
	if (find) {
		return(1);
	}
	STOREVAL(&printed, name);
	return(0);
}



static char *
format_funcname(decp)
	declaration *decp;
{
	char buf[256];
	char *p;

	switch (decp->rel) {
	case REL_POINTER:
		sprintf(buf,"%s_ptr",decp->type);
		break;
	case REL_VECTOR:
		if (streq(decp->type,"string") && decp->array_max == NULL) {
			sprintf(buf,"wrapstring");
		} else {
			sprintf(buf,"%s_%s",decp->type,decp->array_max);
		}
		break;
	case REL_ALIAS:
		sprintf(buf,"%s",decp->type);
		break;
	}
	p = alloc(strlen(buf) + 1);
	strcpy(p,buf);
	return(p);
}




static 
print_undefineds(def)
	definition *def;
{
	case_list *cl;
	declaration *dflt;

	if (def->def_kind != DEF_UNION) {
		return;
	}
	for (cl = def->def.un.cases; cl != NULL; cl = cl->next) {
		if (cl->case_decl.rel != REL_ALIAS) {
			emit_new(&cl->case_decl);
		}
	}
	dflt = def->def.un.default_decl;
	if (dflt) {
		if (dflt->rel != REL_ALIAS) {
			emit_new(dflt);
		}
	}
}


static	
emit_new(dec)
	declaration *dec;
{
	definition *def;

	if (dec->rel == REL_VECTOR && streq(dec->type,"string") && 
			dec->array_max == NULL) {
		return;
	}
	def = ALLOC(definition);
	def->def_kind = DEF_TYPEDEF;
	def->def_name = format_funcname(dec);
	def->def.ty.old_prefix = dec->prefix;
	def->def.ty.old_type = dec->type;
	def->def.ty.rel = dec->rel;
	def->def.ty.array_max = dec->array_max;
	do_emit(def, SPECIALDEF);
}

static
print_header(def,special)
	definition *def;	
	int special;
{
	space();	
	if (special) {
		fprintf(fout,"static ");
	}
	fprintf(fout,"bool_t\n");
	fprintf(fout,"xdr_%s(xdrs,objp)\n",def->def_name);
	fprintf(fout,"\tXDR *xdrs;\n");
	if (special) {
		if (streq(def->def.ty.old_type,"string")) {
			fprintf(fout,"\tchar *");	
		} else if (streq(def->def.ty.old_type,"opaque")) {
			fprintf(fout,"\tchar *");
		} else if (streq(def->def.ty.old_type,"bool")) {
			fprintf(fout,"\tbool_t *");
		} else {
			fprintf(fout,"\t%s *",def->def.ty.old_type);
		}
	} else {
		fprintf(fout,"\t%s ",def->def_name);
	}
	if (def->def_kind != DEF_TYPEDEF  ||
			! isvectordef(def->def.ty.old_type,def->def.ty.rel)) {
		fprintf(fout,"*");
	}
	fprintf(fout,"objp;\n");
	fprintf(fout,"{\n");
}

static
typedefed(def,type)
	definition *def;
	char *type;
{
	if (def->def_kind != DEF_TYPEDEF || def->def.ty.old_prefix != NULL) {
		return(0);
	} else {
		return (streq(def->def_name,type));
	}
}

static
isvectordef(type,rel)
	char *type;
	relation rel;	
{
	definition *def;

	for (;;) {
		switch (rel) {
		case REL_VECTOR:
			return(! streq(type,"string"));
		case REL_POINTER:
			return(0);
		case REL_ALIAS:	
			def = (definition *) FINDVAL(defined,type,typedefed);
			if (def == NULL) {
				return(0);
			}
			type = def->def.ty.old_type;
			rel = def->def.ty.rel;
		}
	}
}
	
static
print_trailer()
{
	fprintf(fout,"\treturn(TRUE);\n");
	fprintf(fout,"}\n");
	space();
}

static
print_ifopen(name)
	char *name;
{
	fprintf(fout,"\tif (! xdr_%s(xdrs",name);
}


static
print_ifarg(arg)
	char *arg;
{
	fprintf(fout,", %s",arg);
}


static
print_ifsizeof(prefix,type)
	char *prefix;
	char *type;
{
	if (streq(type,"bool")) {
		fprintf(fout,", sizeof(bool_t), xdr_bool");
	} else {
		fprintf(fout,", sizeof(");
		if (undefined(type) && prefix) {
			fprintf(fout,"%s ",prefix);
		}
		fprintf(fout,"%s), xdr_%s",type,type);
	}
}

static
print_ifclose()
{
	fprintf(fout,")) {\n");
	fprintf(fout,"\t\treturn(FALSE);\n");
	fprintf(fout,"\t}\n");
}

static
space()
{
	fprintf(fout,"\n\n");
}

static
print_ifstat(prefix,type,rel,amax,name)
	char *prefix;
	char *type;
	relation rel;
	char *amax;
	char *name;
{
	char *alt = NULL;

	switch (rel) {
	case REL_POINTER:
		print_ifopen("pointer");
		print_ifarg("(char *) ");
		fprintf(fout,name);
		print_ifsizeof(prefix,type);
		break;
	case REL_VECTOR:
		if (streq(type,"string")) {
			if (amax) {
				alt = "string";
			} else {
				alt = "wrapstring";	
			}
		} else if (streq(type,"opaque")) {
			alt = "opaque";
		}
		if (alt) {
			print_ifopen(alt);
			print_ifarg(name);
		} else {
			print_ifopen("vector");
			print_ifarg("(char *) ");
			fprintf(fout,name);
		}
		if (amax) {
			print_ifarg(amax);
		}
		if (! alt) {
			print_ifsizeof(prefix,type);
		}
		break;
	case REL_ALIAS:
		print_ifopen(type);
		print_ifarg(name);
		break;
	}
	print_ifclose();
}


/*ARGSUSED*/
static
emit_enum(def)
	definition *def; 
{
	print_ifopen("enum");
	print_ifarg("(enum_t *) objp");
	print_ifclose();
}


static
emit_array(def)
	definition *def;
{
	array_def *ad = &def->def.ar;
	char *prefix = ad->array_prefix;
	char *type = ad->array_type;
	int bytes;

	bytes = streq(type,"opaque");
	print_ifopen(bytes ? "bytes" : "array");
	print_ifarg("&objp->");
	fprintf(fout,ad->array_name);
	print_ifarg("(char *) &objp->");
	fprintf(fout,ad->len_name);
	print_ifarg(ad->array_max);
	if (! bytes) {
		print_ifsizeof(prefix,type);
	}
	print_ifclose();
}



static
print_funcname(decp)
	declaration *decp;
{
	char *buf;

	buf = format_funcname(decp);
	fprintf(fout,buf);
	free(buf);
}


	
static
emit_union(def)
	definition *def;
{
	declaration *dflt;
	

	print_tags(def);
	print_ifopen("union");
	print_ifarg("(enum_t *) &objp->");
	fprintf(fout,def->def.un.enum_decl.name);
	print_ifarg("(char *) &objp->");
	fprintf(fout,"%s",def->def_name);
	print_ifarg("choices");
	dflt = def->def.un.default_decl;
	if (dflt) {
		print_ifarg("xdr_");
		print_funcname(dflt);
	} else {
		print_ifarg("NULL");
	}
	print_ifclose();
}



static
print_tags(def)
	definition *def;
{
	case_list *cl;

	fprintf(fout,"\tstatic struct xdr_discrim choices[] = {\n",def->def_name);
	for (cl = def->def.un.cases; cl != NULL; cl = cl->next) {
		fprintf(fout,"\t\t{ (int) %s, xdr_",cl->case_name);
		print_funcname(&cl->case_decl);
		fprintf(fout," },\n");
	}
	fprintf(fout,"\t\t{ __dontcare__, NULL }\n");
	fprintf(fout,"\t};\n");
	fprintf(fout,"\n");	
}


static
emit_struct(def)
	definition *def;
{
	decl_list *dl;

	for (dl = def->def.st.decls; dl != NULL; dl = dl->next) {
		print_stat(&dl->decl);
	}
}



		
static
emit_typedef(def)
	definition *def;
{
	char *prefix = def->def.ty.old_prefix;
	char *type = def->def.ty.old_type;
	char *amax = def->def.ty.array_max;
	relation rel = def->def.ty.rel;

	print_ifstat(prefix,type,rel,amax,"objp");
}





static
print_stat(dec) 
	declaration *dec;
{
	char *prefix = dec->prefix;
	char *type = dec->type;
	char *amax = dec->array_max;
	relation rel = dec->rel;
	char name[256];

	if (isvectordef(type,rel)) {
		sprintf(name,"objp->%s",dec->name);
	} else {
		sprintf(name,"&objp->%s",dec->name);
	}
	print_ifstat(prefix,type,rel,amax,name);
}
