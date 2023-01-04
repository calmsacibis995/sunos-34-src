#ifndef lint 
static char sccsid[] = "@(#)rpc_svcout.c 1.1 86/09/25 (C) 1986 SMI";
#endif

/*
 * rpc_svcout.c, Server-skeleton outputter for the RPC protocol compiler
 * Copyright (C) 1986, Sun Microsytsems, Inc.
 */
#include <stdio.h>
#include <strings.h>
#include "rpc_parse.h"
#include "rpc_util.h"

static char RQSTP[] = "rqstp";
static char TRANSP[] = "transp";
static char ARG[] = "argument";
static char RESULT[] = "result";
static char ROUTINE[] = "local";


/*
 * write most of the service, that is,
 * everything but the registrations.
 */
void
write_most()
{
	list *l;
	definition *def;
	version_list *vp;


	for (l = defined; l != NULL; l = l->next) {
		def = (definition *) l->val;
		if (def->def_kind == DEF_PROGRAM) {
			write_program(def);
		}
	}
	fprintf(fout,"\n\n");
	fprintf(fout,"main()\n");
	fprintf(fout,"{\n");
	fprintf(fout,"\tSVCXPRT *%s;\n",TRANSP);
	fprintf(fout,"\n");
	for (l = defined; l != NULL; l = l->next) {
		def = (definition *) l->val;
		if (def->def_kind != DEF_PROGRAM) {	
			continue;
		}
		for (vp = def->def.pr.versions; vp != NULL; vp = vp->next) {
			 fprintf(fout,"\tpmap_unset(%s, %s);\n",def->def_name,vp->vers_name);
		}
	}
}


/*
 * write a registration for the given transport
 */
void
write_register(transp)
	char *transp;
{
	list *l;
	definition *def;
	version_list *vp;

	fprintf(fout,"\n");
	fprintf(fout,"\t%s = svc%s_create(RPC_ANYSOCK",TRANSP,transp);
	if (streq(transp,"tcp")) {
		fprintf(fout,", 0, 0");
	}
	fprintf(fout,");\n");
	fprintf(fout,"\tif (%s == NULL) {\n",TRANSP);
	fprintf(fout,"\t\tfprintf(stderr,\"cannot create %s service.\\n\");\n",transp);
	fprintf(fout,"\t\texit(1);\n");
	fprintf(fout,"\t}\n");

	for (l = defined; l != NULL; l = l->next) {
		def = (definition *) l->val;
		if (def->def_kind != DEF_PROGRAM) {	
			continue;
		}
		for (vp = def->def.pr.versions; vp != NULL; vp = vp->next) {
			fprintf(fout,
				"\tif (! svc_register(%s, %s, %s, ",
				TRANSP,def->def_name,vp->vers_name);
			pvname(def->def_name,vp->vers_num);
			fprintf(fout,", IPPROTO_%s)) {\n",
				streq(transp,"udp") ? "UDP" : "TCP");
	 		fprintf(fout,
				"\t\tfprintf(stderr,\"unable to register (%s, %s, %s).\\n\");\n",
				def->def_name,vp->vers_name, transp);
			fprintf(fout,"\t\texit(1);\n");
			fprintf(fout,"\t}\n");
		}
	}
}


/*
 * write the rest of the service
 */
void
write_rest()
{
	fprintf(fout,"\tsvc_run();\n");
	fprintf(fout,"\tfprintf(stderr,\"svc_run returned\\n\");\n");
	fprintf(fout,"\texit(1);\n");
	fprintf(fout,"}\n");
}



static
write_program(def)
	definition *def;
{
	version_list *vp;
	proc_list *proc;
	int filled;

	for (vp = def->def.pr.versions; vp != NULL; vp = vp->next) {
		fprintf(fout,"\n");
		fprintf(fout,"static void\n");
		pvname(def->def_name, vp->vers_num);
		fprintf(fout,"(%s, %s)\n",RQSTP,TRANSP);
		fprintf(fout,"	struct svc_req *%s;\n",RQSTP);
		fprintf(fout,"	SVCXPRT *%s;\n",TRANSP);
		fprintf(fout,"{\n");

		filled = 0;	
		fprintf(fout,"\tunion {\n");
		for (proc = vp->procs; proc != NULL; proc = proc->next) {
			if (streq(proc->arg_type,"void")) {
				continue;
			}
			filled = 1;
			fprintf(fout,"\t\t");	
			if (proc->arg_prefix) {
				if (streq(proc->arg_prefix,"enum")) {
					fprintf(fout,"enum ");
				} else {
					fprintf(fout,"struct ");
				}
			}
			if (streq(proc->arg_type, "bool")) {
				fprintf(fout,"bool_t ");
			} else {
				fprintf(fout,"%s ",proc->arg_type);
			}
			pvname(proc->proc_name,vp->vers_num);
			fprintf(fout,"_arg;\n");
		}
		if (! filled) {
			fprintf(fout,"\t\tint fill;\n");
		}
		fprintf(fout,"\t} %s;\n",ARG);
		fprintf(fout,"\tchar *%s;\n",RESULT);
		fprintf(fout,"\tbool_t (*xdr_%s)(), (*xdr_%s)();\n",ARG,RESULT);
		fprintf(fout,"\tchar *(*%s)();\n",ROUTINE);
		for (proc = vp->procs; proc != NULL; proc = proc->next) {
			fprintf(fout,"\textern ");
			if (proc->res_prefix) {
				if (streq(proc->res_prefix, "enum")) {
					fprintf(fout,"enum ");
				} else {
					fprintf(fout,"struct ");
				}
			}
			if (streq(proc->res_type, "bool")) {
				fprintf(fout,"bool_t ");
			} else {
				fprintf(fout,"%s *",proc->res_type);
			}
			pvname(proc->proc_name,vp->vers_num);
			fprintf(fout,"();\n");
		}
		fprintf(fout,"\n");
		fprintf(fout,"\tswitch (%s->rq_proc) {\n",RQSTP);

		fprintf(fout,"\tcase NULLPROC:\n");
		fprintf(fout,"\t\tsvc_sendreply(%s, xdr_void, NULL);\n",TRANSP);
		fprintf(fout,"\t\treturn;\n\n");

		for (proc = vp->procs; proc != NULL; proc = proc->next) {
			fprintf(fout,"\tcase %s:\n",proc->proc_name);
			fprintf(fout,"\t\txdr_%s = xdr_%s;\n",ARG,proc->arg_type);
			fprintf(fout,"\t\txdr_%s = xdr_%s;\n",RESULT,proc->res_type);
			fprintf(fout,"\t\t%s = (char *(*)()) ",ROUTINE);
			pvname(proc->proc_name,vp->vers_num);
			fprintf(fout,";\n");
			fprintf(fout,"\t\tbreak;\n\n");
		}
		fprintf(fout,"\tdefault:\n");
		printerr("noproc",TRANSP);
		fprintf(fout,"\t\treturn;\n");
		fprintf(fout,"\t}\n");

		fprintf(fout,"\tbzero(&%s, sizeof(%s));\n",ARG,ARG);
		printif("getargs",TRANSP,"&",ARG);
		printerr("decode",TRANSP);
		fprintf(fout,"\t\treturn;\n");
		fprintf(fout,"\t}\n");

		fprintf(fout,"\t%s = (*%s)(&%s);\n",RESULT,ROUTINE,ARG);
		printif("sendreply",TRANSP,"",RESULT);
		printerr("systemerr",TRANSP);
		fprintf(fout,"\t}\n");

		printif("freeargs",TRANSP,"&",ARG);
		fprintf(fout,"\t\tfprintf(stderr,\"unable to free arguments\\n\");\n");
		fprintf(fout,"\t\texit(1);\n");
		fprintf(fout,"\t}\n");

		fprintf(fout,"}\n\n");
	}
}
	
static
printerr(err,transp)
	char *err;
	char *transp;
{
	fprintf(fout,"\t\tsvcerr_%s(%s);\n",err,transp);
}

static
printif(proc,transp,prefix,arg)
	char *proc;
	char *transp;
	char *prefix;
	char *arg;
{
	fprintf(fout,"\tif (! svc_%s(%s, xdr_%s, %s%s)) {\n",
		proc,transp,arg,prefix,arg);
}

static char *
locase(str)
	char *str;
{
	char c;
	static char buf[100];
	char *p = buf;

	while (c = *str++) {
		*p++ = (c >= 'A' && c <= 'Z') ? (c - 'A' + 'a') : c;
	}
	*p = 0;
	return(buf);
}


static
pvname(pname,vnum)
	char *pname;
	char *vnum;
{
	fprintf(fout,"%s_%s",locase(pname),vnum);
}

