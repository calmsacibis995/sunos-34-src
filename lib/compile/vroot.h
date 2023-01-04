/*LINTLIBRARY*/
/*	@(#)vroot.h	1.1 9/24/86 SMI 86/01/17 */
#include <stdio.h>
#include <sys/param.h>

#define DEFAULT ((pathpt)-1)

typedef struct {
	char		*path;
	short		length;
} pathcellt, *pathcellpt, patht[];
typedef patht		*pathpt;

typedef struct {
	short		init;
	pathpt		vector;
	char		*env_var;
} vroot_patht;
typedef struct {
	vroot_patht	vroot;
	vroot_patht	path;
	char		full_path[MAXPATHLEN+1];
	char		*vroot_start;
	char		*path_start;
	char		*filename_start;
	char		*sunpro_dependencies;
	FILE		*file;
} vroot_datat, *vroot_datapt;

typedef enum { rw_read, rw_write} rwt, *rwpt;

extern	vroot_datat	vroot_data;
extern	pathpt		translate_path_string();
extern	char		*calloc();
extern	char		*malloc();
extern	void		add_path_dir();
extern	char		*getenv();
extern	void		translate_with_path_vroot();
extern	char		*vroot_path();
extern	void		report_dependency();
extern	char		*strcpy();
extern	char		*index();
extern	char		*rindex();
extern	char		*sprintf();
