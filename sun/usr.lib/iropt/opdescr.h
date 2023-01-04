/*	@(#)opdescr.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */


/* this table gives the names, properties and register "weights" of ir
	operators - see also the op_map_tab in /lib/cgrdr/pcc.c
*/
struct opdescr_st op_descr[] =	{
	{"err", HOUSE_OP,
		{0, 0}, {0, 0}, 0},
	{"entry_def", UN_OP | HOUSE_OP | MOD_OP | BOOL_OP | FLOAT_OP,
		{-12, -12}, {0, 0}, -60},
	{"exit_use", UN_OP | USE1_OP | HOUSE_OP | BOOL_OP | FLOAT_OP,
		{-12, -12}, {0, 0}, -60}, 
	{"implicit_def", UN_OP | HOUSE_OP | MOD_OP | BOOL_OP | FLOAT_OP,
		{-12, -12}, {0, 0}, -60},
	{"implicit_use", UN_OP | HOUSE_OP | USE1_OP | BOOL_OP | FLOAT_OP,
		{-12, -12}, {0, 0}, -60},
	{"plus", BIN_OP | VALUE_OP | USE1_OP | USE2_OP | FLOAT_OP | COMMUTE_OP,
		{12, 12}, {12, 12}, 30},
	{"minus", BIN_OP | VALUE_OP | USE1_OP | USE2_OP | FLOAT_OP,
		{12, 12}, {12, 8}, 30},
	{"mult", BIN_OP | VALUE_OP | USE1_OP | USE2_OP | FLOAT_OP | COMMUTE_OP,
		{12, 12}, {12, 12}, 30},
 	{"div", BIN_OP | VALUE_OP | USE1_OP | USE2_OP | FLOAT_OP,
		{12, 12}, {12, 12}, 30},
	{"remainder", BIN_OP | VALUE_OP | USE1_OP | USE2_OP | FLOAT_OP,
		{12, 12}, {12, 12}, 30},
	{"and", BIN_OP | VALUE_OP | USE1_OP | USE2_OP | BOOL_OP | COMMUTE_OP,
		{12, 8}, {12, 8}, -60},
	{"or", BIN_OP | VALUE_OP | USE1_OP | USE2_OP | BOOL_OP | COMMUTE_OP,
		{12, 8}, {12, 8}, -60},
	{"xor", BIN_OP | VALUE_OP | USE1_OP | USE2_OP | BOOL_OP | COMMUTE_OP,
		{12, 8}, {12, 8}, -60},
	{"not", UN_OP | VALUE_OP | USE1_OP | BOOL_OP,
		{12, 8}, {12, 8}, -60},
	{"lshift", BIN_OP | VALUE_OP | USE1_OP | USE2_OP,
		{14, 10}, {14, 10}, -60},
	{"rshift", BIN_OP | VALUE_OP | USE1_OP | USE2_OP,
		{14, 10}, {14, 10}, -60},
	{"scall", BIN_OP | ROOT_OP | USE1_OP | NTUPLE_OP,
		{0, 0}, {0, 0}, 0},
	{"fcall", BIN_OP | VALUE_OP |  USE1_OP | FLOAT_OP | NTUPLE_OP,
		{0, 0}, {0, 0}, 0},
	{"eq", BIN_OP | VALUE_OP | USE1_OP | USE2_OP | BOOL_OP | FLOAT_OP | COMMUTE_OP,
		{12, 8}, {12, 8}, 30},
	{"ne", BIN_OP | VALUE_OP | USE1_OP | USE2_OP | BOOL_OP | FLOAT_OP | COMMUTE_OP,
		{12, 8}, {12, 8}, 30},
	{"le", BIN_OP | VALUE_OP | USE1_OP | USE2_OP | BOOL_OP | FLOAT_OP ,
		{12, 8}, {12, 8}, 30},
	{"lt", BIN_OP | VALUE_OP | USE1_OP | USE2_OP | BOOL_OP | FLOAT_OP ,
		{12, 8}, {12, 8}, 30},
	{"ge", BIN_OP | VALUE_OP | USE1_OP | USE2_OP | BOOL_OP | FLOAT_OP ,
		{12, 8}, {12, 8}, 30},
	{"gt", BIN_OP | VALUE_OP | USE1_OP | USE2_OP | BOOL_OP | FLOAT_OP ,
		{12, 8}, {12, 8}, 30},
	{"conv", UN_OP | VALUE_OP | USE1_OP | FLOAT_OP | BOOL_OP ,
		{12, 8}, {0, 0}, 30},
	{"compl", UN_OP | VALUE_OP | USE1_OP | BOOL_OP ,
		{12, 8}, {12, 8}, 30},
	{"neg", UN_OP | VALUE_OP | USE1_OP | FLOAT_OP ,
		{18, 14}, {0, 0}, 30},
	{"addrof", UN_OP | VALUE_OP  ,
	-1000000, -1000000, -1000000},
	{"ifetch", UN_OP | VALUE_OP | USE1_OP | FLOAT_OP | BOOL_OP ,
		{8, 12}, {0, 0}, -60},
	{"istore", BIN_OP | MOD_OP | USE1_OP | USE2_OP | FLOAT_OP | BOOL_OP | ROOT_OP,
		{8, 12}, {12, 12}, 30},
	{"goto", UN_OP | BRANCH_OP | ROOT_OP  | NTUPLE_OP,
		{8, 12}, {0, 0}, -60},
	{"cbranch", BIN_OP | BRANCH_OP | USE1_OP | BOOL_OP | ROOT_OP  | NTUPLE_OP,
		{12, 8}, {8, 12}, 30},
	{"switch", BIN_OP | BRANCH_OP | USE1_OP | ROOT_OP  | NTUPLE_OP,
		{12, 12}, {8, 12}, 30},
	{"repeat", BIN_OP | BRANCH_OP | USE1_OP | MOD_OP | ROOT_OP  | NTUPLE_OP,
		{22, 8}, {0, 0}, -60},
	{"assign", BIN_OP | MOD_OP | USE2_OP | BOOL_OP | FLOAT_OP | ROOT_OP  ,
		{12, 12}, {12, 12}, 30},
	{"pass", UN_OP | HOUSE_OP  ,
		{0, 0}, {0, 0}, 0},
	{"stmt", UN_OP | HOUSE_OP  ,
		{0, 0}, {0, 0}, 0},
	{"labeldef", UN_OP | ROOT_OP,
		{0, 0}, {0, 0}, 0},
	{"indirgoto", UN_OP | BRANCH_OP | USE1_OP | ROOT_OP  | NTUPLE_OP ,
		{8, 12}, {0,0}, 0},
	{"fval", UN_OP | ROOT_OP | USE1_OP  ,
		{0, 0}, {0, 0}, 0},
	{"labelref", BIN_OP | HOUSE_OP  ,
		{0, 0}, {0, 0}, 0},
	{"param", UN_OP | USE1_OP  ,
		{12, 12}, {0, 0}, 30}
};