/*LINTLIBRARY*/
/*	@(#)vroot.c	1.1 9/24/86 SMI 86/01/17 */
#include "vroot.h"
#include <sys/file.h>

vroot_datat	vroot_data= {
	{ 0, NULL, "VIRTUAL_ROOT"},
	{ 0, NULL, "PATH"},
	"", NULL, NULL, NULL,
	"SUNPRO_DEPENDENCIES", NULL};

void
add_path_dir(path, pointer, position)
	char *path;
	pathpt *pointer;
	int position;

{	int size= 0, length;
	char *name;
	pathcellpt p;
	pathpt new;

	if (*pointer != NULL) {
		for (p= &((**pointer)[0]); p->path != NULL; p++, size++);
		if (position < 0)
			position= size;}
	else
		if (position < 0)
			position= 0;
	if (position >= size) {
		new= (pathpt)calloc((unsigned)(position+2), sizeof(pathcellt));
		if (*pointer != NULL) {
			bcopy((char *)(*pointer), (char *)new, size*sizeof(pathcellt));
			free((char *)(*pointer));};
		*pointer= new;};
	length= strlen(path);
	name= malloc((unsigned)(length+1));
	(void)strcpy(name, path);
	if ((**pointer)[position].path != NULL)
		free((**pointer)[position].path);
	(**pointer)[position].path= name;
	(**pointer)[position].length= length;
}

pathpt translate_path_string(string) char *string;
{	char *p;
	pathpt result= NULL;

	if (string != NULL)
		for (; 1; string= p+1) {
			if (p= index(string, ':')) *p= 0;
			add_path_dir(string, &result, -1);
			if (p) *p= ':';
			else return(result);};
	return((pathpt)NULL);
}

static void
close_report_file(status, file) int status; FILE *file;
{
	putc('\n', file);
	(void)fclose(file);
}

void
report_dependency(name) char *name;
{	char *filename, buffer[MAXPATHLEN+1], *p;

	if (vroot_data.file == NULL) {
		if ((filename= getenv(vroot_data.sunpro_dependencies)) == NULL) {
			vroot_data.file= (FILE *)-1; return;};
		(void)strcpy(buffer, name); name= buffer;
		p= index(filename, ' '); *p= 0;
		if ((vroot_data.file= fopen(filename, "a")) == NULL) {
			if ((vroot_data.file= fopen(filename, "w")) == NULL) {
				vroot_data.file= (FILE *)-1; return;};};
		on_exit(close_report_file, (char *)vroot_data.file);
		fputs(p+1, vroot_data.file); putc(':', vroot_data.file);};
	if (vroot_data.file == (FILE *)-1)
		return;
	fputs(name, vroot_data.file);
	putc(' ', vroot_data.file);
}

char *
vroot_path(vroot, path, filename) char **vroot, **path, **filename;
{
	if (vroot != NULL) *vroot= vroot_data.vroot_start;
	if (path != NULL) *path= vroot_data.path_start;
	if (filename != NULL) *filename= vroot_data.filename_start;
	return(vroot_data.full_path);
}

void
translate_with_thunk(filename, thunk, path_vector, vroot_vector, rw)
	char *filename;
	int (*thunk)();
	pathpt path_vector, vroot_vector;
	rwt rw;
{	pathcellt *vp, *pp, *pp1;
	char *p;

/* Setup path to use */
	if (rw == rw_write)
		pp1= NULL;		/* Do not use path when writing */
	else {
		if (path_vector == DEFAULT) {
			if (!vroot_data.path.init) {
				vroot_data.path.init= 1;
				vroot_data.path.vector= translate_path_string(getenv(vroot_data.path.env_var));};
			path_vector= vroot_data.path.vector;};
		pp1= path_vector == NULL ? NULL : &(*path_vector)[0];};

/* Setup vroot to use */
	if (vroot_vector == DEFAULT) {
		if (!vroot_data.vroot.init) {
			vroot_data.vroot.init= 1;
			vroot_data.vroot.vector= translate_path_string(getenv(vroot_data.vroot.env_var));};
		vroot_vector= vroot_data.vroot.vector;};
	vp= vroot_vector == NULL ? NULL : &(*vroot_vector)[0];

/* Setup to remember pieces */
	vroot_data.vroot_start= NULL;
	vroot_data.path_start= NULL;
	vroot_data.filename_start= NULL;

	switch ((vp ?1:0) + (pp1 ? 2:0)) {
	    case 0:	/* No path. No vroot. */
	    use_name:
		(void)strcpy(vroot_data.full_path, filename);
		vroot_data.filename_start= vroot_data.full_path;
		(void)(*thunk)(vroot_data.full_path);
		return;
	    case 1:	/* No path. Vroot */
		if (filename[0] != '/') goto use_name;
		for (; vp->path != NULL; vp++) {
			p= vroot_data.full_path;
			(void)strcpy(vroot_data.vroot_start= p, vp->path);
			p+= vp->length;
			(void)strcpy(vroot_data.filename_start= p, filename);
			if ((*thunk)(vroot_data.full_path)) return;};
		(void)strcpy(vroot_data.full_path, filename);
		return;
	    case 2:	/* Path. No vroot. */
		if (filename[0] == '/') goto use_name;
		for (; pp1->path != NULL; pp1++) {
			p= vroot_data.full_path;
			(void)strcpy(vroot_data.path_start= p, pp1->path);
			p+= pp1->length;
			*p++= '/';
			(void)strcpy(vroot_data.filename_start= p, filename);
			if ((*thunk)(vroot_data.full_path)) return;};
		(void)strcpy(vroot_data.full_path, filename);
		return;
	    case 3: {	/* Path. Vroot. */
			int *rel_path, path_len= 1;
		for (pp= pp1; pp->path != NULL; pp++) path_len++;
		rel_path= (int *)alloca(path_len*sizeof(int));
		for (path_len-= 2; path_len >= 0; path_len--) rel_path[path_len]= 0;
		for (; vp->path != NULL; vp++)
			for (pp= pp1, path_len= 0; pp->path != NULL; pp++, path_len++) {
				if (rel_path[path_len] == 1) continue;
				if (pp->path[0] != '/') rel_path[path_len]= 1;
				p= vroot_data.full_path;
				if ((filename[0] == '/') || (pp->path[0] == '/')) {
					(void)strcpy(vroot_data.vroot_start= p, vp->path); p+= vp->length;};
				if (filename[0] != '/') {
					(void)strcpy(vroot_data.path_start= p, pp->path); p+= pp->length;
					*p++= '/';};
				(void)strcpy(vroot_data.filename_start= p, filename);
				if ((*thunk)(vroot_data.full_path)) return;};
		(void)strcpy(vroot_data.full_path, filename);
		return;};};
}
