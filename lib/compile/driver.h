/* @(#)driver.h 1.7 86/12/15 SMI */

#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "vroot.h"

#define add_to_list_tail(cons, value) add_to_list_tail3(cons, value, (suffixpt)NULL)
#define get_memory(size) malloc((unsigned)(size))

#define FLOAT_OPTION "FLOAT_OPTION"

typedef struct {
	int		value;
	char		*name;
	char		*extra;
} const_intt, *const_intpt;
typedef struct {
	const_intpt	value;
	char		*help;
	int		touched:1;
	int		redefine_ok:1;
} super_intt, *super_intpt;

typedef struct {
	char		*suffix;
	short		in_drivers, out_drivers;
	void		(*compile)();
	void		(*collect)();
	char		*help;
} suffixt, *suffixpt;

typedef struct const {
	char		*value;
	suffixpt	suffix;
	struct const	*next;
} const, *conspt;

typedef struct programt {
	char		*name;
	short		drivers;
	conspt		flags;
	conspt		permanent_flags;
	conspt		infile;
	char		*outfile;
	char		*path;
	char		*(*setup)();
	char		template[5];
} programt, *programpt;

typedef enum { super_var_op, var_op, flag_var_op, and_op, or_op, not_op, driver_op, target_op, end_op} opt, *oppt;
typedef struct exprt {
	opt		op:8;
	int		*value;
} exprt, *exprpt;

typedef struct stept {
	programpt	program;		/* Run this */
	suffixpt	suffix;
	exprpt		expr;
	char		*(*setup)();
	struct timeval	start;
	short		process;
	int		killed:1;
} stept, *steppt;

typedef enum { end_of_list= 1,
		help_option, infile_option,
		make_lint_option,
		module_list_option, module_option,
		optimize_option, outfile_option,
		pass_on_lint_option, pass_on_select_option,
		pass_on_1_option,	/* -x		=> -x */
		pass_on_1t_option,	/* -xREST	=> -xREST */
		pass_on_12_option,	/* -x REST	=> -x REST */
		pass_on_1to_option,	/* -xREST	=> REST */
		pass_on_2_option,	/* -x REST	=> REST */
		produce_option,
		replace_option,
		load_m2l_option, run_m2l_option,
		set_int_arg_option, set_super_int_arg_option,
		target_option,
		temp_dir_option
} option_typet;
typedef struct {
	char		*name;
	short		drivers;
	option_typet	type:8;
	super_intt	*pointer;
	programpt	program;
	const_intpt	arg;
	char		*help;
} optiont, *optionpt;

#define fatal0(string) { (void)fflush(stdout); (void)fprintf(stderr, "%s: ", program_name); (void)fprintf(stderr, (string)); cleanup(0); exit(1);}
#define fatal1(string, arg1) { (void)fflush(stdout); (void)fprintf(stderr, "%s: ", program_name); (void)fprintf(stderr, (string), (arg1)); cleanup(0); exit(1);}
#define fatal2(string, arg1, arg2) { (void)fflush(stdout); (void)fprintf(stderr, "%s: ", program_name); (void)fprintf(stderr, (string), (arg1), (arg2)); cleanup(0); exit(1);}

#define set_int(i, v) { (i).value= (v); (i).touched= 1;}

#define HIDE_OPTION	0x0100
#define NO_MINUS_OPTION 0x0200
#define OBSOLETE_OPTION	0x0400
#define PRODUCE_OPTION	0x0800
#define SOURCE_SUFFIX	0x1000
#define ZH(x) ((x) | HIDE_OPTION)
#define ZM(x) ((x) | NO_MINUS_OPTION)
#define ZO(x) (ZH(x) | OBSOLETE_OPTION)
#define ZP(x) ((x) | PRODUCE_OPTION)
#define ZS(x) ((x) | SOURCE_SUFFIX)

#define Z_C 0x01	/* cc */
#define Z_F 0x02	/* f77 */
#define Z_L 0x04	/* lint */
#define Z_M 0x08	/* m2c */
#define Z_P 0x10	/* pc */
#define Z_all	(Z_C|Z_F|Z_L|Z_M|Z_P)
#define Z_CF	(Z_C|Z_F)
#define Z_CFLM	(Z_C|Z_F|Z_L|Z_M)
#define Z_CFLMP	(Z_C|Z_F|Z_L|Z_M|Z_P)
#define Z_CFLP	(Z_C|Z_F|Z_L|Z_P)
#define Z_CFM	(Z_C|Z_F|Z_M)
#define Z_CFMP	(Z_C|Z_F|Z_M|Z_P)
#define Z_CFP	(Z_C|Z_F|Z_P)
#define Z_CL	(Z_C|Z_L)
#define Z_CLP	(Z_C|Z_L|Z_P)
#define Z_CMP	(Z_C|Z_M|Z_P)
#define Z_CP	(Z_C|Z_P)
#define Z_FL	(Z_F|Z_L)
#define Z_FMP	(Z_F|Z_M|Z_P)
#define Z_LP	(Z_L|Z_P)
#define Z_MP	(Z_M|Z_P)

extern	char			*getwd();
extern	char			*malloc();
extern	char			*sprintf();
extern	char			*index();
extern	char			*rindex();
extern	char			*getenv();
extern	char			*strcat();
extern	char			*strcpy();
extern	char			*strncpy();
extern 	char			*sys_siglist[];
extern	int			errno;

/* from data.c */
extern	optiont			options[];
extern	optiont			drivers[];
extern	stept			c_steps[];
extern	stept			def_steps[];
extern	stept			f_iropt_steps[];
extern	stept			f_no_iropt_steps[];
extern	stept			ld_steps[];
extern	stept			lint_steps[];
extern	stept			m2c_steps[];
extern	stept			mod_steps[];
extern	stept			p_steps[];
extern	stept			pc_steps[];
extern	stept			s_steps[];

/* from compile.c */
extern	char			*add_to_list_tail3();
extern	char			*get_file_suffix();
extern	void			cleanup();
extern	char			*lint_lib();

/* from run_pass.c */
extern	char			*outfile_name();
extern	char			*temp_file_name();
extern	void			run_steps();

/* from setup.c */
extern	void			compile_c();
extern	void			compile_def();
extern	void			compile_f();
extern	void			compile_F();
extern	void			compile_i();
extern	void			compile_mod();
extern	void			compile_r();
extern	void			compile_p();
extern	void			compile_pi();
extern	void			compile_S();
extern	void			compile_s();
extern	void			init_programs();
extern	void			scan_path();
extern	char			*setup_cpp();
extern	char			*setup_cpp_for_cc();
extern	char			*setup_bb_count();
extern	char			*setup_cat_for_f77();
extern	char			*setup_cat_for_lint();
extern	char			*setup_m2cfe_for_def();
extern	char			*setup_m2cfe_for_mod();
extern	char			*setup_f77pass1();
extern	char			*setup_cg_for_tcov();
extern	char			*setup_inline_for_pc();
extern	char			*setup_as_for_f77();
extern	char			*setup_pc3();
extern	char			*setup_ld();
extern	char			*setup_m2l();
extern	void			collect_ln();
extern	void			collect_o();
extern	void			cc_doit();
extern	void			f77_doit();
extern	void			lint_doit();
extern	void			m2c_doit();
extern	void			pc_doit();

#define SUFFIXES \
	SUFFIX(a, Z_CFMP, 0, NULL, collect_o,		"Object library")\
	SUFFIX(c, ZS(Z_CFLMP), Z_C, compile_c, NULL,	"C source")\
	SUFFIX(def, ZS(Z_M), 0, compile_def, NULL,	"Module definitions")\
	SUFFIX(f, ZS(Z_CFMP), Z_F, compile_f, NULL,	"F77 source")\
	SUFFIX(F, ZS(Z_CFMP), 0, compile_F, NULL,	"F77 source for cpp")\
	SUFFIX(il, Z_CFMP, 0, NULL, NULL,		"Inline expansion file")\
	SUFFIX(i, ZS(Z_CL), Z_C, compile_i, NULL,	"C source after cpp")\
	SUFFIX(ln, Z_L, Z_L, NULL, collect_ln,		"Lint library")\
	SUFFIX(mod, ZS(Z_M), 0, compile_mod, NULL,	"Modula-2 source")\
	SUFFIX(o, Z_CFMP, Z_CFMP, NULL, collect_o,	"Object file")\
	SUFFIX(pi, ZS(Z_P), Z_P, compile_pi, NULL,	"Pascal source after cpp")\
	SUFFIX(p, ZS(Z_CFMP), 0, compile_p, NULL,	"Pascal source")\
	SUFFIX(r, ZS(Z_CFMP), 0, compile_r, NULL,	"Ratfor source")\
	SUFFIX(sym, 0, Z_M, NULL, NULL,			"Modula2 Symbol file")\
	SUFFIX(s, ZS(Z_CFMP), Z_CFMP, compile_s, NULL,	"Assembler source")\
	SUFFIX(S, ZS(Z_CFMP), 0, compile_S, NULL,	"Assembler source for cpp")\
	SUFFIX(none, 0, 0, NULL, NULL,			"Intermediate file")
#define SUFFIX(name, in_driver, out_driver, compile, collect, help) suffixt name;
#define SUFFIX_(string, name, in_driver, out_driver, compile, collect, help) suffixt name;
typedef struct suffixvt {
	SUFFIXES
	suffixt		sentinel_suffix_field;
} suffixvt;
extern	suffixvt		suffix;
#undef SUFFIX
#undef SUFFIX_

/* Template values. (Others are NULL or proper string pointer) */
#define IN_Z		1
#define OUT_Z		2
#define FLAG_Z		3
#define STDOUT_Z	4
#define STDIN_Z 	5	/* This must be the first template if it is used */
#define MINUS_O_Z	6

#define PROGRAMS \
	PROGRAM3(cpp, "/lib/cpp", Z_CFLP, NULL, STDOUT_Z, FLAG_Z, IN_Z)\
	PROGRAM3(m4, "/usr/bin/m4", Z_F, NULL, STDIN_Z, STDOUT_Z, FLAG_Z)\
	PROGRAM3(ratfor, "/usr/bin/ratfor", Z_F, NULL, STDIN_Z, STDOUT_Z, FLAG_Z)\
	PROGRAM3(bb_count, "/usr/lib/bb_count", Z_C, setup_bb_count, IN_Z, FLAG_Z, OUT_Z)\
	PROGRAM3(lint1, "/usr/lib/lint/lint1", Z_L, NULL, STDIN_Z, STDOUT_Z, FLAG_Z)\
	PROGRAM3(cat, "/bin/cat", ZH(Z_FL), NULL, STDOUT_Z, FLAG_Z, IN_Z)\
	PROGRAM3(lint2, "/usr/lib/lint/lint2", Z_L, NULL, IN_Z, FLAG_Z, NULL)\
	PROGRAM3(m2cfe, "/usr/lib/modula2/m2cfe", Z_M, NULL, FLAG_Z, IN_Z, STDOUT_Z)\
	PROGRAM3(ccom, "/lib/ccom", Z_C, NULL, STDIN_Z, STDOUT_Z, FLAG_Z)\
	PROGRAM4(pc0, "/usr/lib/pc0", Z_P, NULL, MINUS_O_Z, OUT_Z, FLAG_Z, IN_Z)\
	PROGRAM3(f1, "/usr/lib/f1", Z_P, NULL, STDOUT_Z, FLAG_Z, IN_Z)\
	PROGRAM3(mf1, "/usr/lib/modula2/f1", Z_M, NULL, STDOUT_Z, FLAG_Z, IN_Z)\
	PROGRAM3(f77pass1, "/usr/lib/f77pass1", Z_F, setup_f77pass1, FLAG_Z, IN_Z, OUT_Z)\
	PROGRAM4(iropt, "/usr/lib/iropt", Z_F, NULL, FLAG_Z, MINUS_O_Z, OUT_Z, IN_Z)\
	PROGRAM3(cg, "/usr/lib/cg", Z_F, NULL, STDOUT_Z, FLAG_Z, IN_Z)\
	PROGRAM3(inline, "/usr/lib/inline", Z_CFMP, NULL, STDIN_Z, STDOUT_Z, FLAG_Z)\
	PROGRAM3(c2, "/lib/c2", Z_CFMP, NULL, STDIN_Z, STDOUT_Z, FLAG_Z)\
	PROGRAM4(as, "/bin/as", Z_CFMP, NULL, MINUS_O_Z, OUT_Z, FLAG_Z, IN_Z)\
	PROGRAM3(pc3, "/usr/lib/pc3", Z_P, NULL, FLAG_Z, IN_Z, NULL)\
	PROGRAM4(ld, "/bin/ld", Z_CFP, NULL, FLAG_Z, MINUS_O_Z, OUT_Z, IN_Z)\
	PROGRAM4(m2l, "/usr/bin/m2l", Z_M, NULL, FLAG_Z, IN_Z, MINUS_O_Z, OUT_Z)
#define PROGRAM3(name, path, driver, setup, t1, t2, t3) programt name;
#define PROGRAM4(name, path, driver, setup, t1, t2, t3, t4) programt name;
typedef struct {
	PROGRAMS
	programt	sentinel_program_field;
} programvt;
extern	programvt		program;
#undef PROGRAM3
#undef PROGRAM4

#define set_flag(flag) (global_flags[(flag)>>5]|= (1<<((flag)&0x1f)))
#define reset_flag(flag) (global_flags[(flag)>>5]&= ~(1<<((flag)&0x1f)))
#define is_on(flag) (((global_flags[(flag)>>5]&(1<<((flag)&0x1f))) == 0) ? 0:1)
#define is_off(flag) (((global_flags[(flag)>>5]&(1<<((flag)&0x1f))) == 0) ? 1:0)
#define	as_R			1
#define	checkC			2
#define	checkH			3
#define	checkV			4
#define do_cpp			6
#define do_dependency		7
#define do_cat			8
#define do_inline		9
#define do_m2l			10
#define do_m4			11
#define do_ratfor		12
#define doing_mod_file		13
#define	dryrun			14
#define	fstore			15
#define	ignore_lc		16
#define	junk			17
#define long_offset		18
#define no_default_module_list	19
#define	onetrip			20
#define optimize		21
#define	pipe_ok			22
#define root_module_seen	23
#define	statement_count		24
#define	time_run		26
#define	trace			27
#define used_outfile		28
#define	verbose			29
#define	warning			30

#define Y_EXE	1
#define Y_I	2
#define Y_L	3
#define Y_O	4
#define Y_S	5
#define Z_MC68010 1
#define Z_MC68020 2

#define Z_68881		10
#define Z_FPA		11
#define Z_SKY		12
#define Z_SOFT		13
#define Z_SWITCH	14

#ifdef S5EMUL
#define define_const_int() \
	const_int_3(adb, 1,		"adb")\
	const_int_3(assembler, Y_S,	"assembly-source")\
	const_int_3(cc_driver, Z_C,	(char *)cc_doit)\
	const_int_3(dummy_driver, 0,	NULL)\
	const_int_3(preprocessed, Y_I,	"source")\
	const_int_3(preprocessed2, Y_I,	"source")\
	const_int_3(dbx, 2,		"dbx")\
	const_int_3(executable, Y_EXE,	"executable")\
	const_int_4(f68881, Z_68881,	"-f68881", "/lib/Mcrt1.o")\
	const_int_3(f77_driver, Z_F,	(char *)f77_doit)\
	const_int_4(ffpa, Z_FPA,	"-ffpa", "/lib/Wcrt1.o")\
	const_int_4(fsky, Z_SKY,	"-fsky", "/lib/Scrt1.o")\
	const_int_4(fsoft, Z_SOFT,	"-fsoft", "/lib/Fcrt1.o")\
	const_int_4(fswitch, Z_SWITCH,	"-fswitch", NULL)\
	const_int_4(gprof, 2,		"gprof", "/usr/5lib/gcrt0.o")\
	const_int_3(lint1_file, Y_L,	"lint")\
	const_int_3(lint_driver, Z_L,	(char *)lint_doit)\
	const_int_3(mc68010_arch, Z_MC68010,	"-mc68010")\
	const_int_3(mc68020_arch, Z_MC68020,	"-mc68020")\
	const_int_3(m2c_driver, Z_M,	(char *)m2c_doit)\
	const_int_4(no_prof, 3,		NULL, "/usr/5lib/crt0.o")\
	const_int_3(object, Y_O,	"object")\
	const_int_3(one, 1,		NULL)\
	const_int_3(pc_driver, Z_P,	(char *)pc_doit)\
	const_int_4(prof, 1,		"prof", "/usr/5lib/mcrt0.o")
#else
#define define_const_int() \
	const_int_3(adb, 1,		"adb")\
	const_int_3(assembler, Y_S,	"assembly-source")\
	const_int_3(cc_driver, Z_C,	(char *)cc_doit)\
	const_int_3(dummy_driver, 0,	NULL)\
	const_int_3(preprocessed, Y_I,	"source")\
	const_int_3(preprocessed2, Y_I,	"source")\
	const_int_3(dbx, 2,		"dbx")\
	const_int_3(executable, Y_EXE,	"executable")\
	const_int_4(f68881, Z_68881,	"-f68881", "/lib/Mcrt1.o")\
	const_int_3(f77_driver, Z_F,	(char *)f77_doit)\
	const_int_4(ffpa, Z_FPA,	"-ffpa", "/lib/Wcrt1.o")\
	const_int_4(fsky, Z_SKY,	"-fsky", "/lib/Scrt1.o")\
	const_int_4(fsoft, Z_SOFT,	"-fsoft", "/lib/Fcrt1.o")\
	const_int_4(fswitch, Z_SWITCH,	"-fswitch", NULL)\
	const_int_4(gprof, 2,		"gprof", "/lib/gcrt0.o")\
	const_int_3(lint1_file, Y_L,	"lint")\
	const_int_3(lint_driver, Z_L,	(char *)lint_doit)\
	const_int_3(mc68010_arch, Z_MC68010,	"-mc68010")\
	const_int_3(mc68020_arch, Z_MC68020,	"-mc68020")\
	const_int_3(m2c_driver, Z_M,	(char *)m2c_doit)\
	const_int_4(no_prof, 3,		NULL, "/lib/crt0.o")\
	const_int_3(object, Y_O,	"object")\
	const_int_3(one, 1,		NULL)\
	const_int_3(pc_driver, Z_P,	(char *)pc_doit)\
	const_int_4(prof, 1,		"prof", "/lib/mcrt0.o")
#endif
#define define_super_int() \
	super_int(debugger, NULL,		"debug")\
	super_int(driver, NULL,			"")\
	super_int(float_mode, NULL,		"floating point option")\
	super_int(host, NULL,			"")\
	super_int(produce, &executable,		"produce")\
	super_int(profile, &no_prof,		"profile")\
	super_int(target, NULL,			"target")
#define define_regular_int() \
	regular_int(arg_list_size)\
	regular_int(build_lint_lib)\
	regular_int(exit_status)\
	regular_int(infile_count)\
	regular_int(process_number)\
	regular_int(processes_running)\
	regular_int(source_infile_count)\
	regular_int(status)\
	regular_int(temp_file_number)
#define define_regular_variable()\
	variable_2(stept,	compile_steps[sizeof(program)/sizeof(programt)])\
	variable_3(steppt,	compile_stepsp, compile_steps)\
	variable_2(conspt,	files_to_unlink)\
	variable_2(char,	float_mode_name[MAXPATHLEN])\
	variable_2(char *,	iropt_files[3])\
	variable_2(int,		global_flags[1])\
	variable_2(conspt,	infile)\
	variable_2(conspt,	infile_ln)\
	variable_2(conspt,	infile_o)\
	variable_2(int,		last_program_path)\
	variable_3(char *,	lint_library_dir, "/usr/lib/lint")\
	variable_2(conspt,	module_list)\
	variable_2(char *,	outfile)\
	variable_2(char,	profile_name[MAXPATHLEN])\
	variable_3(char *,	program_name, "compile")\
	variable_2(pathpt,	program_path)\
	variable_2(char *,	pwd)\
	variable_2(suffixpt,	requested_suffix)\
	variable_2(char *,	target_architecture)\
	variable_2(char *,	tcov_file)\
	variable_3(char *,	temp_dir, "/tmp")

#define const_int_3(name, value, help) extern const_intt name;
#define const_int_4(name, value, help, extra) extern const_intt name;
#define super_int(name, value, help) extern super_intt name;
#define regular_int(name) extern int name;
#define variable_2(type, name) extern type name;
#define variable_3(type, name, value) extern type name;

	define_const_int()
	define_super_int()
	define_regular_int()
	define_regular_variable()

#undef const_int_3
#undef const_int_4
#undef super_int
#undef regular_int
#undef variable_2
#undef variable_3
