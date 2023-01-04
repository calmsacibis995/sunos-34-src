
/*	@(#)getgroup.h 1.1 86/09/25 (C) 1985 Sun Microsystems, Inc.	*/

struct grouplist {		
	char *gl_machine;
	char *gl_name;
	char *gl_domain;
	struct grouplist *gl_nxt;
};

struct grouplist *my_getgroup();

			
