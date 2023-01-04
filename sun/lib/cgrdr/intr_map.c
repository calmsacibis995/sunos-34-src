#ifndef lint
static	char sccsid[] = "@(#)intr_map.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/* this module knows which libF77 library functions are implemented as
** pcc operators.
*/

#define NULL 0
/* 			from mip.h */
# define FABS_PCCOP    115	/*  y = abs(x) */
# define FCOS_PCCOP    116	/*  y = cos(x) */
# define FSIN_PCCOP    117	/*  y = sin(x) */
# define FTAN_PCCOP    118	/*  y = tan(x) */
# define FACOS_PCCOP   119	/*  y = arccos(x) */
# define FASIN_PCCOP   120	/*  y = arcsin(x) */
# define FATAN_PCCOP   121	/*  y = arctan(x) */
# define FCOSH_PCCOP   122	/*  y = cosh(x) */
# define FSINH_PCCOP   123	/*  y = sinh(x) */
# define FTANH_PCCOP   124	/*  y = tanh(x) */
# define FEXP_PCCOP    125	/*  y = e**x */
# define F10TOX_PCCOP  126	/*  y = 10**x */
# define F2TOX_PCCOP   127	/*  y = 2**x */
# define FLOGN_PCCOP   128	/*  y = log_base_e(x) */
# define FLOG10_PCCOP  129	/*  y = log_base_10(x) */
# define FLOG2_PCCOP   130	/*  y = log_base_2(x) */
# define FSQR_PCCOP    131	/*  y = sqr(x) */
# define FSQRT_PCCOP   132	/*  y = sqrt(x) */
# define FAINT_PCCOP   133	/*  y = aint(x) */
# define FANINT_PCCOP  134	/*  y = anint(x) */
# define FNINT_PCCOP   135	/*  n = nint(x) */

static struct intr_descr {
	char *name;
	int pcc_opno;
} intr_descr_tab[] = {
	{ "d_abs", FABS_PCCOP }, { "r_abs", FABS_PCCOP },
	{ "d_cos", FCOS_PCCOP }, { "r_cos", FCOS_PCCOP },
	{ "d_sin", FSIN_PCCOP }, { "r_sin", FSIN_PCCOP },
	{ "d_tan", FTAN_PCCOP }, { "r_tan", FTAN_PCCOP },
	{ "d_acos", FACOS_PCCOP }, { "r_acos", FACOS_PCCOP },
	{ "d_asin", FASIN_PCCOP }, { "r_asin", FASIN_PCCOP },
	{ "d_atan", FATAN_PCCOP }, { "r_atan", FATAN_PCCOP },
	{ "d_cosh", FCOSH_PCCOP }, { "r_cosh", FCOSH_PCCOP },
	{ "d_sinh", FSINH_PCCOP }, { "r_sinh", FSINH_PCCOP },
	{ "d_tanh", FTANH_PCCOP }, { "r_tanh", FTANH_PCCOP },
	{ "d_exp", FEXP_PCCOP }, { "r_exp", FEXP_PCCOP },
	{ "pow_10d", F10TOX_PCCOP }, { "pow_10r", F10TOX_PCCOP },
	{ "pow_2d", F2TOX_PCCOP }, { "pow_2r", F2TOX_PCCOP },
	{ "d_sqr", FSQR_PCCOP }, { "r_sqr", FSQR_PCCOP },
	{ "d_sqrt", FSQRT_PCCOP }, { "r_sqrt", FSQRT_PCCOP },
	{ "d_log", FLOGN_PCCOP }, { "r_log", FLOGN_PCCOP },
	{ "d_lg10", FLOG10_PCCOP }, { "r_lg10", FLOG10_PCCOP },
	{ "d_int", FAINT_PCCOP }, { "r_int", FAINT_PCCOP },
	{ "d_nint", FANINT_PCCOP }, { "r_nint", FANINT_PCCOP },
};
#define N_INTR_DESCR (sizeof(intr_descr_tab) / sizeof(struct intr_descr))

/* must be greater  than N_INTR_DESCR */
#define HASH_TAB_SIZE  256
static struct intr_descr *hash_tab[HASH_TAB_SIZE];


intr_map_init()
{
register int i, index;
register struct intr_descr *descr;
register char *cp;

	for(i=0; i< N_INTR_DESCR; i++ ) {
		descr = &intr_descr_tab[i];
		cp = descr->name;
		index = 0;
		while(*cp) index += *cp++;
		index = index%HASH_TAB_SIZE;
		while(hash_tab[index]) {
			index++;
			index %= HASH_TAB_SIZE;
		}
		hash_tab[index] = descr;
	}
}

is_intrinsic(name)
register char *name;
{
register int index, len;
register struct intr_descr **hashp;
char name_copy[21];
register char *cp2;

	if(!name) return -1;
	len = strlen(name);
	if(len > 20) return -1;
	cp2 = name_copy;
	index = 0;
	while(*name && *name != ' ') {
		index += *name;
		*cp2++ = *name++;
	}
	*cp2 = '\0';
	index %=  HASH_TAB_SIZE;
	hashp = &hash_tab[index];
	while(*hashp) {
		if(strcmp(name_copy, (*hashp)->name) == 0) {
			return (*hashp)->pcc_opno;
		}
		hashp ++;
	}
	return -1;
}
