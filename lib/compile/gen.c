/*	@(#)gen.c	1.1 9/24/86 SMI 86/01/17 */
#include <ctype.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <stdio.h>

extern	char		*malloc();
extern	char		*index();
extern	char		*rindex();
extern	char		*strncpy();
extern	char		*sprintf();

typedef struct {
	char		*name;
	char		*expr;
	int		translated;
} formalt, *formalpt;

typedef struct namet {
	char		*type;
	char		*name;
	int		formals_count;
	int		translated_count;
	formalt		formals[32];
	char		*formal_type;
	char		*returns;
	char		*ifdef;
	char		*include;
	char		*comment;
	int		special;
	char		*special_expr;
} namet, *namept;

struct {
	short		translated;
	short		untranslated;
} options= {0};
namet		name;
char		*buffer;

void fatal(message) char *message;
{
	(void)fprintf(stderr, "ERROR %s\n", message);
	exit(17);
}

char *copy_string(string, length) char *string; int length;
{	char *result;

	if (length < 0) length= strlen(string);
	if ((result= malloc((unsigned)(length+1))) == NULL) fatal("Malloc failed");
	(void)strncpy(result, string, length);
	return(result);
}

void print_fn_decl(file, name_suffix, formals, formal_type, extra_returns, returns)
	FILE *file; char *name_suffix, *formals, *formal_type, *returns;
{	int n;

	(void)fprintf(file, "%s\t%s%s(", name.type, name.name, name_suffix);
	for (n= 0; n < name.formals_count; n++) {
		(void)fprintf(file, "%s", name.formals[n].name);
		if (n+1 != name.formals_count) (void)fprintf(file, ", ");};
	if ((name.formals_count > 0) && (formals[0] != 0)) (void)fprintf(file, ", ");
	if (returns) {
		(void)fprintf(file, "%s) %s %s {%s", formals, name.formal_type, formal_type, extra_returns);
		for (n= 0; n < name.formals_count; n++)
			(void)fprintf(file, "Z(%s); ", name.formals[n].name);
		(void)fprintf(file, "%s}\n", returns);}
	else
		(void)fprintf(file, "%s) %s %s\n", formals, name.formal_type, formal_type);
}

void generic_gen(name_suffix, formals, formal_type, path, vroot)
	char *name_suffix, *formals, *formal_type, *path, *vroot;
{	FILE *file;
	char filename[MAXPATHLEN];
	int n, m;

	(void)sprintf(filename, "%s%s.c", name.name, name_suffix);
	file= fopen(filename, "w");
	if (name.ifdef)
		(void)fprintf(file, "#ifdef %s\n", name.ifdef);
	(void)fprintf(file, "#include \"vroot.h\"\n");
	(void)fprintf(file, "#include <sys/param.h>\n");
	(void)fprintf(file, "#include <stdio.h>\n");
	(void)fprintf(file, "#include <sys/time.h>\n");
	if (name.include)
		(void)fprintf(file, "#include <%s>\n", name.include);
	if (name.special) {
		(void)fprintf(file, "#include \"%s.h\"\n\n", name.name);
		fprintf(file, "extern\tint\t%s_thunk();\n\n", name.name);};
	print_fn_decl(file, name_suffix, formals, formal_type, (char *)NULL, (char *)NULL);
	(void)fprintf(file, "{\n");
	if (name.special) { char *rw;
		for (n= 0; n < name.formals_count; n++)
			if (!name.formals[n].translated)
				fprintf(file, "\tvroot_args.arg.%s= %s;\n",
					name.formals[n].name, name.formals[n].name);
		fprintf(file, "\ttranslate_with_thunk(");
		for (n= 0; n < name.formals_count; n++)
			if (name.formals[n].translated) {
				fprintf(file, "%s", name.formals[n].name);
				rw= name.formals[n].expr;};
		fprintf(file, ", %s_thunk, %s, %s, %s);\n", name.name, path, vroot, rw);
		fprintf(file, "\treturn(vroot_args.returns.type);\n}\n");
		goto end_fn;};
	if (name.translated_count > 1)
		(void)fprintf(file, "\tregister char *P, *Q;\n");
	(void)fprintf(file, "\textern\tint\taccess_thunk();\n");
	for (n= 0, m= 1; n < name.formals_count-1; n++)
		if ((name.formals[n].translated) && (m++ != name.translated_count))
			(void)fprintf(file, "\tchar BUFFER%d[MAXPATHLEN];\n", n);
	if (name.translated_count > 1)
		(void)fprintf(file, "\n");
	for (n= 0, m= 1; n < name.formals_count; n++)
	    if (name.formals[n].translated) {
		(void)fprintf(file, "\ttranslate_with_thunk(%s, access_thunk, %s, %s, %s);\n",
				name.formals[n].name, path, vroot, name.formals[n].expr);
		if (m++ != name.translated_count)
			(void)fprintf(file, "\tfor (P= BUFFER%d, Q= vroot_data.full_path; *P++= *Q++;);\n", n);};
	(void)fprintf(file, "\treturn(%s_plain(", name.name);
	for (n= 0, m= 1; n < name.formals_count; n++) {
		if (name.formals[n].translated) {
			if (m++ != name.translated_count)
				(void)fprintf(file, "BUFFER%d", n);
			else
				(void)fprintf(file, "vroot_data.full_path");}
		else
			(void)fprintf(file, "%s", name.formals[n].name);
		if (n+1 != name.formals_count) (void)fprintf(file, ", ");};
	(void)fprintf(file, "));\n}\n");
    end_fn:
	if (name.ifdef)
		(void)fprintf(file, "#endif %s\n", name.ifdef);
	(void)fclose(file);
}

void gen_thunk()
{	FILE *file;
	char filename[MAXPATHLEN];
	int n;

	(void)sprintf(filename, "%s_thunk.c", name.name);
	file= fopen(filename, "w");
	(void)fprintf(file, "#include <errno.h>\n");
	(void)fprintf(file, "#include <sys/time.h>\n");
	(void)fprintf(file, "#include \"%s.h\"\n", name.name);
	(void)fprintf(file, "extern\tint\terrno;\n");
	(void)fprintf(file, "extern\t%s\t%s_plain();\n\n", name.type, name.name);
	(void)fprintf(file, "int\t%s_thunk(path) char *path;\n{\n", name.name);
	(void)fprintf(file, "\tvroot_args.returns.type= %s_plain(", name.name);
	for (n= 0; n < name.formals_count; n++) {
		if (n != 0) (void)fprintf(file, ", ");
		if (name.formals[n].translated)
			(void)fprintf(file, "path");
		else
			(void)fprintf(file, "vroot_args.arg.%s", name.formals[n].name);};
	(void)fprintf(file, ");\n");
	(void)fprintf(file, "\treturn((%s) != 0);\n", name.special_expr);
	(void)fprintf(file, "}\n");
	(void)fclose(file);
}

void gen_plain()
{	FILE *file;
	char filename[MAXPATHLEN];

	(void)sprintf(filename, "%s_plain.S", name.name);
	file= fopen(filename, "w");
	if (name.ifdef)
		(void)fprintf(file, "#ifdef %s\n", name.ifdef);
	if (name.translated_count > 0)
		(void)fprintf(file, "#define PLAIN_CALL\n");
	if (buffer[0] == 0) {
		(void)fprintf(file, "#include \"SYS.h\"\n");
		(void)fprintf(file, "SYSCALL(%s)\n", name.name);
		(void)fprintf(file, "\tRET\n");}
	else
		(void)fputs(buffer, file);
	if (name.ifdef)
		(void)fprintf(file, "#endif %s\n", name.ifdef);
	(void)fclose(file);
}

/*
 *	argv[0]		program name
 *	argv[1]		data file name
 *	argv[2..]	options
 */

/*
 * File format. One line with the following fields
 *	<type>\t<call-name>(<formals>) <formal-types> { return(<value>);} <gen-args>
 * followed by the assembly code for the plain call if it shouldn't be generated.
 * The <gen-args> can be one or nore of:
 *	/* <comment> * /		For the lint file.
 *	#label				Surround ouput with #ifdef label #endif label
 *	#include <filename>		Add #include <filename> to c output files
 */

main(argc, argv) int argc; char *argv[];
{	int fd, n;
	struct stat stat_buffer;
	char *p, *q;
	FILE *file;

	switch (argc) {
		case 0: case 1:
			fatal("Too few args: gen data-file [translated] [untranslated]");
		case 2: break;
		default:
			for (n= 2; n<argc; n++) {
				options.untranslated|= strcmp(argv[n], "untranslated") == 0 ? 1:0;
				options.translated|= strcmp(argv[n], "translated") == 0 ? 1:0;};};
/* read file */
	if ((fd= open(argv[1], O_RDONLY, 0)) < 0) fatal("Could not open infile");
	if (fstat(fd, &stat_buffer) < 0) fatal("Stat failed");
	if ((buffer= malloc((unsigned)(stat_buffer.st_size+2))) == NULL) fatal("Malloc failed");
	if (read(fd, buffer, stat_buffer.st_size) != stat_buffer.st_size) fatal("Read failed");
	(void)close(fd);
	buffer[stat_buffer.st_size+2]= 0;
/* parse file */
	name.special= 0;
	while (buffer[0] == '#') {
		if ((buffer= index(buffer, '\n')) == NULL)
			fatal("Illegal comment");};
	if ((p= index(buffer, '\t')) == NULL) fatal("Missing tab");
	name.type= copy_string(buffer+1, p-buffer-1); buffer= p+1;
	if ((p= index(buffer, '(')) == NULL) fatal("Missing (");
	name.name= copy_string(buffer, p-buffer); buffer= p;
	if ((p= index(buffer, ')')) == NULL) fatal("Missing )");
	name.formals_count= name.translated_count= 0; buffer++;
	if (p > buffer)
	    while (1) {
		q= index(buffer, ',');
		if (!q || (q > p)) {
			name.formals[name.formals_count].name= copy_string(buffer, p-buffer);
			name.formals[name.formals_count].expr= NULL;
			name.formals[name.formals_count].translated= 0;
			if (name.formals[name.formals_count].name[0] == '!') {
				name.translated_count++;
				name.formals[name.formals_count].translated= 1;
				name.formals[name.formals_count].name++;};
			name.formals_count++;
			buffer= p;
			break;};
		name.formals[name.formals_count].name= copy_string(buffer, q-buffer);
		name.formals[name.formals_count].expr= NULL;
		name.formals[name.formals_count].translated= 0;
		if (name.formals[name.formals_count].name[0] == '!') {
			name.translated_count++;
			name.formals[name.formals_count].translated= 1;
			name.formals[name.formals_count].name++;};
		name.formals_count++;
		buffer= q+1;
		while (isspace(*buffer)) buffer++;};
	if ((p= rindex(buffer, '{')) == NULL) fatal("Missing {");
	if (p-buffer-3 > 0)
		name.formal_type= copy_string(buffer+2, p-buffer-3);
	else
		name.formal_type= "";
	buffer= p;
	if ((p= index(buffer, '}')) == NULL) fatal("Missing }");
	name.returns= copy_string(buffer+1, p-buffer-1); buffer= p+1;
	name.include= NULL;
	while (*buffer != '\n') {
		while ((*buffer != '\n' && isspace(*buffer))) buffer++;
		if (*buffer == '\n') break;
		if (!strncmp(buffer, "#include <", 10)) {
			buffer+= 10;
			if ((p= index(buffer, '>')) == NULL) fatal("Missing >");
			name.include= copy_string(buffer, p-buffer);
			buffer= p+1;
			continue;};
		if (*buffer == '$') {
			buffer++;
			name.special= 1;
			continue;};
		if (*buffer == '#') {
			p= ++buffer;
			while ((*buffer != '\n') && !isspace(*buffer)) buffer++;
			name.ifdef= copy_string(p, buffer-p);
			continue;};
		if (*buffer == '/') {
			p= buffer++;
			while ((*buffer != '\n') && (*buffer != '/')) buffer++;
			buffer++;
			name.comment= copy_string(p, buffer-p);
			continue;};};
	if (name.special) {
		buffer++;
		if ((p= index(buffer, '\n')) == NULL) fatal("special expression missing");
		*p= 0;
		name.special_expr= buffer;
		buffer= p;};
	for (buffer++, n= 0; n < name.formals_count; n++)
		if (name.formals[n].translated) {
			if ((p= index(buffer, '\n')) == NULL) fatal("rw expression missing");
			*p= 0;
			name.formals[n].expr= buffer;
			buffer= p+1;};
	
	if (name.translated_count > 0)
		options.translated= 1;
	if (options.untranslated)
		options.translated= 0;
	if (name.special) { char filename[MAXPATHLEN];
		(void)sprintf(filename, "%s.h", name.name);
		file= fopen(filename, "w");
		fprintf(file, "\tstruct { union { %s type; int test;} returns; struct { %s} arg;} vroot_args;\n",
			name.type, name.formal_type);
		(void)fclose(file);
		gen_thunk();};
	if (options.translated) generic_gen("", "", "", "NULL", "DEFAULT");
	if (options.translated) generic_gen("p", "", "", "DEFAULT", "DEFAULT");
	if (options.translated) generic_gen("_pv", "PATH, VROOT", "pathpt PATH, VROOT;", "PATH", "VROOT");
	gen_plain();
}
