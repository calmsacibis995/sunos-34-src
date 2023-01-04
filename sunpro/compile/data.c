/* @(#)data.c 1.1 87/01/08 SMI */

#include "driver.h"

#define SUFFIX(name, in_driver, out_driver, compile, collect, help) \
	{ "name", (in_driver), (out_driver), (compile), (collect), (help)},
#define SUFFIX_(string, name, in_driver, out_driver, compile, collect, help) \
	{ (string), (in_driver), (out_driver), (compile), (collect), (help)},
suffixvt	suffix= {
	SUFFIXES
	{NULL, 0, 0, NULL, NULL, NULL}
};

#define const_int_3(name, value, help) const_intt name= {(value), (help), (char *)NULL};
#define const_int_4(name, value, help, extra) const_intt name= {(value), (help), (extra)};
	define_const_int()

#define O(name, driver, type, help) \
	{ (name), (driver), (type), NULL, NULL, NULL, (help)},
#define O_S0(name, driver, flag, help) \
	{ (name), (driver), set_int_arg_option, (super_intpt)(flag), NULL, 0, (help)},
#define O_S1(name, driver, flag, help) \
	{ (name), (driver), set_int_arg_option, (super_intpt)(flag), NULL, (const_intpt)1, (help)},
#define O_SA(name, driver, flag, arg, help) \
	{ (name), (driver), set_super_int_arg_option, &(flag), NULL, &(arg), (help)},
#define	O_PASS_ON_1(name, driver, prog, help) \
	{ (name), (driver), pass_on_1_option, NULL, &(program.prog), NULL, (help)},
#define	O_PASS_ON_1T(name, driver, prog, help) \
	{ (name), (driver), pass_on_1t_option, NULL, &(program.prog), NULL, (help)},
#define	O_PASS_ON_12(name, driver, prog, help) \
	{ (name), (driver), pass_on_12_option, NULL, &(program.prog), NULL, (help)},
#define	O_PASS_ON_1TO(name, driver, prog, help) \
	{ (name), (driver), pass_on_1to_option, NULL, &(program.prog), NULL, (help)},
#define	O_PASS_ON_2(name, driver, prog, help) \
	{ (name), (driver), pass_on_2_option, NULL, &(program.prog), NULL, (help)},
#define END_OF_OPTIONS() { (char *)NULL, 0, end_of_list, NULL, NULL, NULL, NULL}
optiont options[]= {
/* Order is important in this table. Long strings should come before short ones. */
	O_PASS_ON_1("-66", Z_F, f77pass1, "Report non FORTRAN-66 constructs as errors")
	O_PASS_ON_12("-align", ZM(Z_CFMP), ld, "Page align and pad symbol X (for ld)")
	O_S1("-a", Z_CF, statement_count, "Prepare to count number of times each basic block is executed")
	O("-a", Z_L, pass_on_lint_option, "Report assignments of long values to int variables")
	O_PASS_ON_1TO("-A", Z_C, as, "Obsolete. Use -Qoption as opt.")
	O_PASS_ON_12("-A", ZH(Z_FMP), ld, "Incremental linking")
	O("-b", Z_L, pass_on_lint_option, "Report break statements that can not be reached")
	O_PASS_ON_1("-b", Z_P, pc0, "Buffer 'output' in blocks, not lines")
	O_S1("-B", ZO(Z_C), junk, NULL)
	O_SA("-c", Z_CFP, produce, object, "Produce '.o' file. Do not run ld.")
	O("-c", Z_L, pass_on_lint_option, "Complain about casts with questionable portability")
	O_PASS_ON_1("-C", Z_C, cpp, "Make ccp preserve C style comments")
	O_PASS_ON_1("-C", Z_F, f77pass1, "Generate code to check subscripts")
	O("-C", Z_L, make_lint_option, NULL)
	O_S1("-C", Z_P, checkC, "Generate code to check subscripts and subranges")
	O_S1("-dryrun", Z_all, dryrun, "Show but do not execute the commands constructed by the driver")
	O_PASS_ON_1("-d", Z_F, f77pass1, "Debug/trace option")
	O_PASS_ON_1("-d", ZH(Z_CMP), ld, "Force definition of common")
	O_PASS_ON_1T("-D", Z_CFLP, cpp, "Define cpp symbol X (for cpp)")
	O("-e", ZM(Z_M), run_m2l_option, NULL)
	O("-E", ZM(Z_M), load_m2l_option, NULL)
	O_SA("-E", Z_C, produce, preprocessed, "Run source thru cpp, deliver on stdout")
	O_SA("-f68881", Z_CFMP, float_mode, f68881, NULL)
	O_SA("-ffpa", Z_CFMP, float_mode, ffpa, NULL)
	O_SA("-fsky", Z_CFMP, float_mode, fsky, NULL)
	O_SA("-fsoft", Z_CFMP, float_mode, fsoft, NULL)
	O_S1("-fstore", Z_F, fstore, "Force assignments to write to memory")
	O_SA("-fswitch", Z_CFMP, float_mode, fswitch, NULL)
	O_PASS_ON_1("-fsingle2", Z_C, ccom, "Pass float values as float not double")
	O_PASS_ON_1("-fsingle", Z_C, ccom, "Use single precision arithmetic when 'float' only")
	O_SA("-F", Z_F, produce, preprocessed, "Run cpp/ratfor only")
	O_SA("-go", Z_C, debugger, adb, "Generate extra information for adb")
	O_SA("-g", Z_CFMP, debugger, dbx, "Generate extra information for dbx")
/* -G */
	O("-help", Z_all, help_option, NULL)
	O("-h", Z_L, pass_on_lint_option, "Be heuristic")
	O_S1("-H", Z_P, checkH, "Generate code to check heap pointers")
	O_PASS_ON_1("-i2", Z_F, f77pass1, "Make integers be two bytes by default")
	O_PASS_ON_1("-i4", Z_F, f77pass1, "Make integers be four bytes by default")
	O_SA("-i", Z_L, produce, lint1_file, "Run lint pass1 only. Leave '.ln' files")
	O_PASS_ON_12("-i", Z_P, pc0, "Produce list of module")
	O_PASS_ON_1T("-I", Z_CFLP, cpp, "Add directory X to cpp include path (for cpp)")
/* -j */
	O_S1("-J", Z_CP, long_offset, "Generate long offsets for switch/case statements")
	O_PASS_ON_1("-keys", Z_M, m2l, "Report detailed result of consistency checks by m2l")
/* -K */
	O_PASS_ON_1("-list", Z_M, m2cfe, "Generate listing")
	O("-l", Z_CFM, infile_option, "Read object library (for ld)")
	O("-l", Z_L, infile_option, "Read definition of object library")
	O("-l", Z_P, infile_option, "Read object library (for ld) or generate listing")
	O_PASS_ON_1T("-L", Z_CFM, ld, "Add directory X to ld library path (for ld)")
	O_PASS_ON_1("-L", Z_P, pc0, "Map upper case letters to lower case in identifiers and keywords")
	O_S1("-m4", Z_F, do_m4, "Run source thru m4")
	O_SA("-mc68010", Z_all, target, mc68010_arch, "Generate code for the 68010")
	O_SA("-mc68020", Z_all, target, mc68020_arch, "Generate code for the 68020")
	O_SA("-m68010", ZH(Z_all), target, mc68010_arch, "Generate code for the 68010")
	O_SA("-m68020", ZH(Z_all), target, mc68020_arch, "Generate code for the 68020")
	O_PASS_ON_12("-map", ZM(Z_M), m2l, "Generate modula-2 link map file")
	O("-m", ZM(Z_M), module_option, NULL)
	O("-M", Z_M, module_list_option, NULL)
	O_S1("-M", Z_C, do_dependency, "Collect dependencies")
	O_PASS_ON_1("-nobounds", Z_M, m2cfe, "Do not compile array bound checking code")
	O_PASS_ON_1("-norange", Z_M, m2cfe, "Do not compile range checking code")
	O_S1("-n", Z_L, ignore_lc, "Do not check against C library")
	O_PASS_ON_1("-n", ZH(Z_CFMP), ld, "Make shared")
	O_PASS_ON_1T("-Nc", Z_F, f77pass1, "Set size of do loop nesting compiler table to X")
	O_PASS_ON_1T("-Nn", Z_F, f77pass1, "Set size of identifier compiler table to X")
	O_PASS_ON_1T("-Nq", Z_F, f77pass1, "Set size of equivalenced variables compiler table to X")
	O_PASS_ON_1T("-Ns", Z_F, f77pass1, "Set size of statement numbers compiler table to X")
	O_PASS_ON_1T("-Nx", Z_F, f77pass1, "Set size of external identifier compiler table to X")
	O_PASS_ON_1("-N", ZH(Z_CFMP), ld, "Do not make shared")
	O_S1("-onetrip", Z_F, onetrip, "Perform DO loops at least once")
	O("-o", ZM(Z_CFLMP), outfile_option, NULL)
	O("-O", Z_CFMP, optimize_option, NULL)
	O_S1("-pipe", Z_all, pipe_ok, "Use pipes, not temp files")
	O_SA("-pg", Z_CFMP, profile, gprof, "Prepare to collect data for the gprof program")
	O_SA("-p", Z_CFMP, profile, prof, "Prepare to collect data for the prof program")
	O_SA("-P", Z_C, produce, preprocessed2, "Run source thru cpp, deliver on file")
	O("-P", Z_F, optimize_option, NULL)
	O_PASS_ON_1("-P", Z_P, pc0, "Use partial evaluation semantics for and/or")
/* -q */
	O("-Qoption", ZM(Z_all), pass_on_select_option, NULL)
	O("-Qpath", ZM(Z_all), replace_option, NULL)
	O("-Qproduce", ZM(Z_all), produce_option, NULL)
	O("-Qtarget", ZH(ZM(Z_all)), target_option, NULL)
	O_PASS_ON_1("-r", ZH(Z_CFMP), ld, "Make relocateable")
	O_S1("-R", Z_CMP, as_R, "Merge data segment with text segment (for as)")
	O_PASS_ON_1TO("-R", Z_F, ratfor, "Obsolete. Use -Qoption ratfor opt.")
	O_PASS_ON_1("-s", Z_P, pc0, "Accept Standard Pascal only")
	O_PASS_ON_1("-s", ZH(Z_CFM), ld, "Strip")
	O_SA("-S", Z_CFMP, produce, assembler, "Produce '.s' file. Do not run as.")
	O("-temp=", Z_all, temp_dir_option, NULL)
	O_S1("-time", Z_all, time_run, "Report the execution time for the compiler passes")
	O_S1("-trace", Z_M, trace, "Show compiler actions")
	O("-t", Z_P, temp_dir_option, NULL)
	O_PASS_ON_1("-t", ZH(Z_CFM), ld, "Trace ld")
	O_PASS_ON_12("-Tdata", ZH(Z_CFMP), ld, "Set address")
	O_PASS_ON_12("-Ttext", ZH(Z_CFMP), ld, "Set address")
	O_PASS_ON_1("-u", Z_F, f77pass1, "Make the default type be 'undefined', not 'integer'")
	O_PASS_ON_12("-u", ZH(Z_CMP), ld, "Undefine")
	O("-u", Z_L, pass_on_lint_option, "Library mode")
	O_PASS_ON_1T("-U", Z_CLP, cpp, "Delete initial defintion of cpp symbol X (for cpp)")
	O_PASS_ON_1("-U", Z_F, f77pass1, "Do not map upper case letters to lower case")
	O_S1("-verbose", Z_L, verbose, "Report which programs the driver invokes")
	O_S1("-v", Z_CFMP, verbose, "Report which programs the driver invokes")
	O("-v", Z_L, pass_on_lint_option, "Do not complain about unused formals")
	O_S1("-V", Z_P, checkV, "Report hard errors for non standard Pascal constructs")
	O_PASS_ON_1("-w66", Z_F, f77pass1, "Do not print FORTRAN 66 compatibility warnings")
	O_PASS_ON_1("-w", Z_C, ccom, "Do not print warnings")
	O_PASS_ON_1("-w", Z_F, f77pass1, "Do not print warnings")
	O_S1("-w", Z_P, warning, "Do not print warnings")
/* -W */
	O("-x", Z_L, pass_on_lint_option, "Report variables referred to by extern but not used")
	O_PASS_ON_1("-x", ZH(Z_CFMP), ld, "Remove locals")
	O_PASS_ON_1("-X", ZH(Z_CFMP), ld, "Save most locals")
	O_PASS_ON_1T("-y", ZH(Z_CFMP), ld, "Trace symbol")
/* -Y */
	O("-z", Z_L, pass_on_lint_option, "Do not complain about referenced but not defined structs")
	O_PASS_ON_1("-z", Z_P, pc0, "Prepare to collect data for the pxp program")
	O_PASS_ON_1("-z", ZH(Z_CFM), ld, "Make demand load")
	O_PASS_ON_1("-Z", Z_P, pc0, "Initialize local storage to zero")
	END_OF_OPTIONS()
};

#define DRIVER(name, drivers, value) { (name), (drivers), set_super_int_arg_option, &driver, NULL, (value), NULL},
optiont drivers[]= {
	DRIVER("cc", Z_C, &cc_driver)
	DRIVER("compile", ZH(0), &dummy_driver)
	DRIVER("f77", Z_F, &f77_driver)
	DRIVER("lint", Z_L, &lint_driver)
	DRIVER("m2c", Z_M, &m2c_driver)
	DRIVER("pc", Z_P, &pc_driver)
	END_OF_OPTIONS()
};

#define AND { and_op, NULL}
#define OR { or_op, NULL}
#define NOT { not_op, NULL}
#define SI(v) { super_var_op, (int *)&(v)}
#define FI(v) {flag_var_op, (int *)(v)}
#define I(v) { var_op, &(v)}
#define D(d) { driver_op, (int *)&(d)}
#define T(t) { target_op, (int *)&(t)}
#define END_EXPRS { end_op, NULL}
#define expr1(name, x1) static exprt name[]= { x1, END_EXPRS}
#define expr2(name, x1, x2) static exprt name[]= { x1, x2, END_EXPRS}
#define expr3(name, x1, x2, x3) static exprt name[]= { x1, x2, x3, END_EXPRS}
#define expr4(name, x1, x2, x3, x4) static exprt name[]= { x1, x2, x3, x4, END_EXPRS}

expr4(bb_count_expr, FI(statement_count), D(lint_driver), NOT, AND);
expr1(cpp_expr, FI(do_cpp));
expr1(cat_expr, FI(do_cat));
expr4(inline_expr, FI(do_inline), D(lint_driver), NOT, AND);
expr1(lint_expr, D(lint_driver));
expr3(m4_expr, FI(do_ratfor), FI(do_m4), AND);
expr2(nlint_expr, D(lint_driver), NOT);
expr4(optimize_expr, FI(optimize), D(lint_driver), NOT, AND);
expr1(ratfor_expr, FI(do_ratfor));
expr2(statement_expr, FI(statement_count), NOT);

#define STEP(prog, suff, expr, setup) \
	{&program.prog, &suffix.suff, (expr), (setup)},
#define END_OF_STEPS() { &program.sentinel_program_field, NULL, NULL, NULL}

stept c_steps[]= {
	STEP(cpp, i, cpp_expr, setup_cpp_for_cc)
	STEP(bb_count, c, bb_count_expr, NULL)
	STEP(ccom, s, nlint_expr, NULL)
	STEP(lint1, ln, lint_expr, NULL)
	STEP(inline, s, inline_expr, NULL)
	STEP(c2, s, optimize_expr, NULL)
	STEP(as, o, nlint_expr, NULL)
	END_OF_STEPS()
};

stept def_steps[]= {
	STEP(m2cfe, sym, NULL, setup_m2cfe_for_def)
	END_OF_STEPS()
};

stept f_iropt_steps[]= {
	STEP(cpp, f, cpp_expr, setup_cpp)
	STEP(m4, f, m4_expr, NULL)
	STEP(ratfor, f, ratfor_expr, NULL)
	STEP(f77pass1, none, NULL, NULL)
	STEP(iropt, none, NULL, NULL)
	STEP(cg, s, NULL, setup_cg_for_tcov)
	STEP(inline, s, inline_expr, NULL)
	STEP(c2, s, statement_expr, NULL)
	STEP(cat, s, cat_expr, setup_cat_for_f77)
	STEP(as, o, NULL, setup_as_for_f77)
	END_OF_STEPS()
};

stept f_no_iropt_steps[]= {
	STEP(cpp, f, cpp_expr, setup_cpp)
	STEP(m4, f, m4_expr, NULL)
	STEP(ratfor, f, ratfor_expr, NULL)
	STEP(f77pass1, s, NULL, NULL)
	STEP(inline, s, inline_expr, NULL)
	STEP(c2, s, optimize_expr, NULL)
	STEP(cat, s, cat_expr, setup_cat_for_f77)
	STEP(as, o, NULL, setup_as_for_f77)
	END_OF_STEPS()
};

stept ld_steps[]= {
	STEP(ld, none, NULL, setup_ld)
	END_OF_STEPS()
};

stept lint_steps[]= {
	STEP(cat, none, NULL, setup_cat_for_lint)
	STEP(lint2, none, NULL, NULL)
	END_OF_STEPS()
};

stept m2c_steps[]= {
	STEP(m2l, none, NULL, setup_m2l)
	END_OF_STEPS()
};

stept mod_steps[]= {
	STEP(m2cfe, none, NULL, setup_m2cfe_for_mod)
	STEP(mf1, s, NULL, NULL)
	STEP(inline, s, inline_expr, NULL)
	STEP(c2, s, optimize_expr, NULL)
	STEP(as, o, NULL, NULL)
	END_OF_STEPS()
};

stept p_steps[]= {
	STEP(cpp, pi, cpp_expr, setup_cpp)
	STEP(pc0, none, NULL, NULL)
	STEP(f1, s, NULL, NULL)
	STEP(inline, s, NULL, setup_inline_for_pc)
	STEP(c2, s, optimize_expr, NULL)
	STEP(as, o, NULL, NULL)
	END_OF_STEPS()
};

stept pc_steps[]= {
	STEP(pc3, none, NULL, setup_pc3)
	STEP(ld, none, NULL, setup_ld)
	END_OF_STEPS()
};

stept s_steps[]= {
	STEP(cpp, s, cpp_expr, setup_cpp)
	STEP(as, o, NULL, NULL)
	END_OF_STEPS()
};
