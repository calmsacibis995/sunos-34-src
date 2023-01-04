#ifndef lint
static	char sccsid[] = "@(#)toolmerge.c 1.3 87/01/07 SMI";
#endif

/*
 * Sun Microsystems, Inc.
 */

/*
 * 	Overview:	Tool Merger: The size of the libraries used by tools
 *			is large.  Substantial memory size saving can be
 *			realized by merging together a group of tools.
 *			The text for every instance of each of the merged tools
 *			is shared.  Each merged tool should be a symbolic
 *			link to the shared object file.
 */

extern selection_svc_main();
extern cmdtool_main();
extern shelltool_main();
extern textedit_main();
extern gfxtool_main();
extern view_surface_main();
extern clocktool_main();
extern suntools_main();
extern mailtool_main();
extern perfmeter_main();
extern cursor_demo_main();
extern framedemo_main();
extern spheresdemo_main();
extern canvas_demo_main();
extern jumpdemo_main();
extern bouncedemo_main();
extern lockscreen_main();
extern lockscreen_default_main();
extern tektool_main();
extern overview_main();
extern adjacentscreens_main();
extern toolplaces_main();
extern perfmon_main();
extern swin_main();
extern align_equals_main();
extern capitalize_main();
extern clear_functions_main();
extern get_selection_main();
extern insert_brackets_main();
extern setkeys_main();
extern shift_lines_main();
extern fontedit_main();
extern traffic_main();
extern iconedit_main();
extern defaultsedit_main();
extern scrolldefaults_main();
extern m2d_main();
extern d2m_main();
extern defaults_to_indentpro_main();
extern indentpro_to_defaults_main();
extern stty_from_defaults_main();
extern defaults_from_input_main();
extern input_from_defaults_main();
extern switcher_main();

struct map {
	char cmd[32];
	int  (*routine)();
};
struct map  cmd_routine_map[] = {
#include TOOLSLIST
};

main(argc, argv)
	int argc;
	char **argv;
{
	extern	char *rindex();
	char	*filename = rindex(argv[0], '/');
	int i;
	int num_of_cmds;

	if (!filename)
		filename = argv[0];
	else
		filename++;
	num_of_cmds = sizeof(cmd_routine_map)/sizeof(struct map);
	for (i = 0; i < sizeof(cmd_routine_map)/sizeof(struct map); i++)  { 
		if (strcmp(filename,cmd_routine_map[i].cmd) == 0)  {
			cmd_routine_map[i].routine(argc,argv);
			exit(0);
		}
	}
	printf("Couldn't run %s in toolmerge.main\n", argv[0]);
	
	exit(0);
}
