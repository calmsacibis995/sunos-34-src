#ifndef lint
static	char sccsid[] = "@(#)mkglue.c 1.1 86/09/25 SMI"; /* from UCB 1.10 83/07/09 */
#endif

/*
 * Make the uba interrupt file ubglue.s or mb interrupt file mbglue.s
 */
#include <stdio.h>
#include "config.h"
#include "y.tab.h"

/*
 * print an interrupt handler for unibus
 */
dump_ub_handler(fp, vec, number)
	register FILE *fp;
	register struct vlst *vec;
	int number;
{
	char nbuf[80];
	register char *v = nbuf;

	(void) sprintf(v, "%s%d", vec->v_id, number);
	fprintf(fp, "\t.globl\t_X%s\n\t.align\t2\n_X%s:\n\tpushr\t$0x3f\n",
	    v, v);
	if (strncmp(vec->v_id, "dzx", 3) == 0)
		fprintf(fp, "\tmovl\t$%d,r0\n\tjmp\tdzdma\n\n", number);
	else {
		if (strncmp(vec->v_id, "uur", 3) == 0) {
			fprintf(fp, "#ifdef UUDMA\n");
			fprintf(fp, "\tmovl\t$%d,r0\n\tjsb\tuudma\n", number);
			fprintf(fp, "#endif\n");
		}
		fprintf(fp, "\tpushl\t$%d\n", number);
		fprintf(fp, "\tcalls\t$1,_%s\n\tpopr\t$0x3f\n", vec->v_id);
		fprintf(fp, "#if defined(VAX750) || defined(VAX730)\n");
		fprintf(fp, "\tincl\t_cnt+V_INTR\n#endif\n\trei\n\n");
	}
}

/*
 * print an interrupt handler for mainbus
 */
dump_mb_handler(fp, vec, number)
	register FILE *fp;
	register struct vlst *vec;
	int number;
{
	fprintf(fp, "\tVECINTR(_X%s%d, _%s, _V%s%d)\n",
		vec->v_id, number, vec->v_id, vec->v_id, number);
}

mbglue()
{
	register FILE *fp;
	char *name = "mbglue.s";

	fp = fopen(path(name), "w");
	if (fp == 0) {
		perror(path(name));
		exit(1);
	}
	fprintf(fp, "#include \"../machine/asm_linkage.h\"\n\n");
	glue(fp, dump_mb_handler);
	(void) fclose(fp);
}

ubglue()
{
	register FILE *fp;
	char *name = "ubglue.s";

	fp = fopen(path(name), "w");
	if (fp == 0) {
		perror(path(name));
		exit(1);
	}
	glue(fp, dump_ub_handler);
	(void) fclose(fp);
}

glue(fp, dump_handler)
	register FILE *fp;
	register int (*dump_handler)();
{
	register struct device *dp, *mp;

	for (dp = dtab; dp != 0; dp = dp->d_next) {
		mp = dp->d_conn;
		if (mp != 0 && mp != (struct device *)-1 &&
		    !eq(mp->d_name, "mba")) {
			struct vlst *vd, *vd2;

			for (vd = dp->d_vec; vd; vd = vd->v_next) {
				for (vd2 = dp->d_vec; vd2; vd2 = vd2->v_next) {
					if (vd2 == vd) {
						(void)(*dump_handler)
							(fp, vd, dp->d_unit);
						break;
					}
					if (!strcmp(vd->v_id, vd2->v_id))
						break;
				}
			}
		}
	}
}
