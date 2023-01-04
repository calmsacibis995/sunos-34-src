#ifndef lint
static	char sccsid[] = "@(#)mkioconf.c 1.1 86/09/25 SMI"; /* from UCB 2.8 83/06/11 */
#endif

#include <stdio.h>
#include <sun/autoconf.h>
#include "y.tab.h"
#include "config.h"

/*
 * build the ioconf.c file
 */
char	*qu();
char	*intv();

#ifdef MACHINE_VAX
vax_ioconf()
{
	register struct device *dp, *mp, *np;
	register int uba_n, slave;
	register struct vlst *vp;
	FILE *fp;

	fp = fopen(path("ioconf.c"), "w");
	if (fp == 0) {
		perror(path("ioconf.c"));
		exit(1);
	}
	fprintf(fp, "#include \"../machine/pte.h\"\n");
	fprintf(fp, "#include \"../h/param.h\"\n");
	fprintf(fp, "#include \"../h/buf.h\"\n");
	fprintf(fp, "#include \"../h/map.h\"\n");
	fprintf(fp, "#include \"../h/vm.h\"\n");
	fprintf(fp, "\n");
	fprintf(fp, "#include \"../vaxmba/mbavar.h\"\n");
	fprintf(fp, "#include \"../vaxuba/ubavar.h\"\n\n");
	fprintf(fp, "\n");
	fprintf(fp, "#define C (caddr_t)\n\n");
	fprintf(fp, "\n");
	fprintf(fp, "#ifndef lint\n");
	fprintf(fp, "#define INTR_HAND(func)\t\textern func();\n");
	fprintf(fp, "#else lint\n");
	fprintf(fp, "#define INTR_HAND(func)\t\tfunc() { ; }\n");
	fprintf(fp, "#endif lint\n");
	fprintf(fp, "\n\n");

	/*
	 * First print the mba initialization structures
	 */
	if (seen_mba) {
		for (dp = dtab; dp != 0; dp = dp->d_next) {
			mp = dp->d_conn;
			if (mp == 0 || mp == TO_NEXUS ||
			    !eq(mp->d_name, "mba"))
				continue;
			fprintf(fp, "extern struct mba_driver %sdriver;\n",
			    dp->d_name);
		}
		fprintf(fp, "\nstruct mba_device mbdinit[] = {\n");
		fprintf(fp, "\t/* Device,  Unit, Mba, Drive, Dk */\n");
		for (dp = dtab; dp != 0; dp = dp->d_next) {
			mp = dp->d_conn;
			if (dp->d_unit == QUES || mp == 0 ||
			    mp == TO_NEXUS || !eq(mp->d_name, "mba"))
				continue;
			if (dp->d_addr != UNKNOWN) {
				printf("can't specify csr address on mba for %s%d\n",
				    dp->d_name, dp->d_unit);
				continue;
			}
			if (dp->d_vec != 0) {
				printf("can't specify vector for %s%d on mba\n",
				    dp->d_name, dp->d_unit);
				continue;
			}
			if (dp->d_drive == UNKNOWN) {
				printf("drive not specified for %s%d\n",
				    dp->d_name, dp->d_unit);
				continue;
			}
			if (dp->d_slave != UNKNOWN) {
				printf("can't specify slave number for %s%d\n", 
				    dp->d_name, dp->d_unit);
				continue;
			}
			fprintf(fp, "\t{ &%sdriver, %d,   %s,",
				dp->d_name, dp->d_unit, qu(mp->d_unit));
			fprintf(fp, "  %s,  %d },\n",
				qu(dp->d_drive), dp->d_dk);
		}
		fprintf(fp, "\t0\n};\n\n");
		/*
		 * Print the mbsinit structure
		 * Driver Controller Unit Slave
		 */
		fprintf(fp, "struct mba_slave mbsinit [] = {\n");
		fprintf(fp, "\t/* Driver,  Ctlr, Unit, Slave */\n");
		for (dp = dtab; dp != 0; dp = dp->d_next) {
			/*
			 * All slaves are connected to something which
			 * is connected to the massbus.
			 */
			if ((mp = dp->d_conn) == 0 || mp == TO_NEXUS)
				continue;
			np = mp->d_conn;
			if (np == 0 || np == TO_NEXUS ||
			    !eq(np->d_name, "mba"))
				continue;
			fprintf(fp, "\t{ &%sdriver, %s",
			    mp->d_name, qu(mp->d_unit));
			fprintf(fp, ",  %2d,    %s },\n",
			    dp->d_unit, qu(dp->d_slave));
		}
		fprintf(fp, "\t0\n};\n\n");
	}
	/*
	 * Now generate interrupt vectors for the unibus
	 */
	for (dp = dtab; dp != 0; dp = dp->d_next) {
		if (dp->d_vec != 0) {
			mp = dp->d_conn;
			if (mp == 0 || mp == TO_NEXUS ||
			    !eq(mp->d_name, "uba"))
				continue;
			fprintf(fp,
			    "extern struct uba_driver %sdriver;\n",
			    dp->d_name);
			vp = dp->d_vec;
			for (vp = dp->d_vec; vp; vp= vp->v_next)
				fprintf(fp, "INTR_HAND(X%s%d)\n",
				    vp->v_id, dp->d_unit);
			fprintf(fp, "int\t (*%sint%d[])() = { ", dp->d_name,
			    dp->d_unit);
			for (vp = dp->d_vec; ;) {
				fprintf(fp, "X%s%d", vp->v_id, dp->d_unit);
				if (vp->v_vec != 0)
					fprintf(stderr,
					    "vector number for %s ignored\n",
					    vp->v_id);
				vp = vp->v_next;
				if (vp == 0)
					break;
				fprintf(fp, ", ");
			}
			fprintf(fp, ", 0 } ;\n");
		}
	}
	fprintf(fp, "\nstruct uba_ctlr ubminit[] = {\n");
	fprintf(fp, "/*\t driver,\tctlr,\tubanum,\talive,\tintr,\taddr */\n");
	for (dp = dtab; dp != 0; dp = dp->d_next) {
		mp = dp->d_conn;
		if (dp->d_type != CONTROLLER || mp == TO_NEXUS || mp == 0 ||
		    !eq(mp->d_name, "uba"))
			continue;
		if (dp->d_vec == 0) {
			printf("must specify vector for %s%d\n",
			    dp->d_name, dp->d_unit);
			continue;
		}
		if (dp->d_addr == UNKNOWN) {
			printf("must specify csr address for %s%d\n",
			    dp->d_name, dp->d_unit);
			continue;
		}
		if (dp->d_drive != UNKNOWN || dp->d_slave != UNKNOWN) {
			printf("drives need their own entries; dont ");
			printf("specify drive or slave for %s%d\n",
			    dp->d_name, dp->d_unit);
			continue;
		}
		if (dp->d_flags) {
			printf("controllers (e.g. %s%d) ",
			    dp->d_name, dp->d_unit);
			printf("don't have flags, only devices do\n");
			continue;
		}
		fprintf(fp,
		    "\t{ &%sdriver,\t%d,\t%s,\t0,\t%sint%d, C 0%o },\n",
		    dp->d_name, dp->d_unit, qu(mp->d_unit),
		    dp->d_name, dp->d_unit, dp->d_addr);
	}
	fprintf(fp, "\t0\n};\n");
/* unibus devices */
	fprintf(fp, "\nstruct uba_device ubdinit[] = {\n");
	fprintf(fp,
"\t/* driver,  unit, ctlr,  ubanum, slave,   intr,    addr,    dk, flags*/\n");
	for (dp = dtab; dp != 0; dp = dp->d_next) {
		mp = dp->d_conn;
		if (dp->d_unit == QUES || dp->d_type != DEVICE || mp == 0 ||
		    mp == TO_NEXUS || mp->d_type == MASTER ||
		    eq(mp->d_name, "mba"))
			continue;
		np = mp->d_conn;
		if (np != 0 && np != TO_NEXUS && eq(np->d_name, "mba"))
			continue;
		np = 0;
		if (eq(mp->d_name, "uba")) {
			if (dp->d_vec == 0) {
				printf("must specify vector for device %s%d\n",
				    dp->d_name, dp->d_unit);
				continue;
			}
			if (dp->d_addr == UNKNOWN) {
				printf("must specify csr for device %s%d\n",
				    dp->d_name, dp->d_unit);
				continue;
			}
			if (dp->d_drive != UNKNOWN || dp->d_slave != UNKNOWN) {
				printf("drives/slaves can be specified ");
				printf("only for controllers, ");
				printf("not for device %s%d\n",
				    dp->d_name, dp->d_unit);
				continue;
			}
			uba_n = mp->d_unit;
			slave = QUES;
		} else {
			if ((np = mp->d_conn) == 0) {
				printf("%s%d isn't connected to anything ",
				    mp->d_name, mp->d_unit);
				printf(", so %s%d is unattached\n",
				    dp->d_name, dp->d_unit);
				continue;
			}
			uba_n = np->d_unit;
			if (dp->d_drive == UNKNOWN) {
				printf("must specify ``drive number'' ");
				printf("for %s%d\n", dp->d_name, dp->d_unit);
				continue;
			}
			/* NOTE THAT ON THE UNIBUS ``drive'' IS STORED IN */
			/* ``SLAVE'' AND WE DON'T WANT A SLAVE SPECIFIED */
			if (dp->d_slave != UNKNOWN) {
				printf("slave numbers should be given only ");
				printf("for massbus tapes, not for %s%d\n",
				    dp->d_name, dp->d_unit);
				continue;
			}
			if (dp->d_vec != 0) {
				printf("interrupt vectors should not be ");
				printf("given for drive %s%d\n",
				    dp->d_name, dp->d_unit);
				continue;
			}
			if (dp->d_addr != UNKNOWN) {
				printf("csr addresses should be given only ");
				printf("on controllers, not on %s%d\n",
				    dp->d_name, dp->d_unit);
				continue;
			}
			slave = dp->d_drive;
		}
		fprintf(fp, "\t{ &%sdriver,  %2d,   %s,",
		    eq(mp->d_name, "uba") ? dp->d_name : mp->d_name, dp->d_unit,
		    eq(mp->d_name, "uba") ? " -1" : qu(mp->d_unit));
		fprintf(fp, "  %s,    %2d,   %s, C 0%-6o,  %d,  0x%x },\n",
		    qu(uba_n), slave, intv(dp), dp->d_addr, dp->d_dk,
		    dp->d_flags);
	}
	fprintf(fp, "\t0\n};\n");
	pseudo_inits(fp);
	(void) fclose(fp);
}
#endif MACHINE_VAX

#if defined(MACHINE_SUN2) || defined(MACHINE_SUN3)

#include <machine/scb.h>

check_vector(vec)
	register struct vlst *vec;
{

	if (vec->v_vec == 0)
		fprintf(stderr, "vector number for %s not given\n", vec->v_id);
	else if (vec->v_vec < VEC_MIN || vec->v_vec > VEC_MAX)
		fprintf(stderr,
			"vector number %d for %s is not between %d and %d\n",
			vec->v_vec, vec->v_id, VEC_MIN, VEC_MAX);
}

sun_ioconf()
{
	register struct device *dp, *mp;
	register int slave;
	register struct vlst *vp;
	FILE *fp;

	fp = fopen(path("ioconf.c"), "w");
	if (fp == 0) {
		perror(path("ioconf.c"));
		exit(1);
	}
	fprintf(fp, "#include \"../h/param.h\"\n");
	fprintf(fp, "#include \"../h/buf.h\"\n");
	fprintf(fp, "#include \"../h/map.h\"\n");
	fprintf(fp, "#include \"../h/vm.h\"\n");
	fprintf(fp, "\n");
	fprintf(fp, "#include \"../sundev/mbvar.h\"\n");
	fprintf(fp, "\n");
	fprintf(fp, "#define C (caddr_t)\n");
	fprintf(fp, "\n");
	fprintf(fp, "#ifndef lint\n");
	fprintf(fp, "#define INTR_HAND(func, var, val)\t");
	fprintf(fp, 	"int var = val;\textern func();\n");
	fprintf(fp, "#else lint\n");
	fprintf(fp, "#define INTR_HAND(func, var, val)\t");
	fprintf(fp,	"int var = val;\tfunc() { ; }\n");
	fprintf(fp, "#endif lint\n");
	fprintf(fp, "\n\n");

	/*
	 * Now generate interrupt vectors for the Mainbus
	 */
	for (dp = dtab; dp != 0; dp = dp->d_next) {
		mp = dp->d_conn;
		if (mp == TO_NEXUS || mp == 0 || mp->d_conn != TO_NEXUS)
			continue;
		fprintf(fp, "extern struct mb_driver %sdriver;\n",
			    dp->d_name);
		if (dp->d_vec != 0) {
			if (dp->d_pri == 0)
				fprintf(stderr,
				    "no priority specified for %s%d\n",
				    dp->d_name, dp->d_unit);
			for (vp = dp->d_vec; vp; vp = vp->v_next)
				fprintf(fp, "INTR_HAND(X%s%d, V%s%d, %d)\n", 
				    vp->v_id, dp->d_unit,
				    vp->v_id, dp->d_unit, dp->d_unit);
			fprintf(fp, "struct vec %s[] = { ", intv(dp));
			for (vp = dp->d_vec; vp != 0; vp = vp->v_next) {
				fprintf(fp, "{ X%s%d, %d, &V%s%d }, ",
					vp->v_id, dp->d_unit, vp->v_vec,
					vp->v_id, dp->d_unit);
				check_vector(vp);
			}
			fprintf(fp, "0 };\n");
		}
	}

	/*
	 * Now spew forth the mb_ctlr structures
	 */
	fprintf(fp, "\nstruct mb_ctlr mbcinit[] = {\n");
	fprintf(fp,
"/* driver,\tctlr,\talive,\taddress,\tintpri,\t intr,\tspace */\n");
	for (dp = dtab; dp != 0; dp = dp->d_next) {
		mp = dp->d_conn;
		if (dp->d_type != CONTROLLER || mp == TO_NEXUS || mp == 0 ||
		    mp->d_conn != TO_NEXUS)
			continue;
		if (dp->d_addr == UNKNOWN) {
			printf("must specify csr address for %s%d\n",
			    dp->d_name, dp->d_unit);
			continue;
		}
		if (dp->d_drive != UNKNOWN || dp->d_slave != UNKNOWN) {
			printf("drives need their own entries; ");
			printf("don't specify drive or slave for %s%d\n",
			    dp->d_name, dp->d_unit);
			continue;
		}
		if (dp->d_flags) {
			printf("controllers (e.g. %s%d) don't have flags, ",
			    dp->d_name, dp->d_unit);
			printf("only devices do\n");
			continue;
		}
		if (dp->d_pri > 0 && dp->d_vec == 0 && (
		    dp->d_bus == SP_VME16D16 ||
		    dp->d_bus == SP_VME24D16 ||
		    dp->d_bus == SP_VME32D16 ||
		    dp->d_bus == SP_VME16D32 ||
		    dp->d_bus == SP_VME24D32 ||
		    dp->d_bus == SP_VME32D32)) {
			printf("Warning: should use vectored interrupts for ");
			printf("vme device %s%d\n", dp->d_name, dp->d_unit);
		}
		fprintf(fp,
		"{ &%sdriver,\t%d,\t0,\tC 0x%08x,\t%d,\t%s, 0x%x },\n",
		    dp->d_name, dp->d_unit, dp->d_addr,
		    dp->d_pri, intv(dp), (MAKE_MACH(dp->d_mach) | dp->d_bus));
	}
	fprintf(fp, "\t0\n};\n");

	/*
	 * Now we go for the mb_device stuff
	 */
	fprintf(fp, "\nstruct mb_device mbdinit[] = {\n");
	fprintf(fp,
"/* driver,\tunit, ctlr, slave, address,      pri, dk, flags, intr, space */\n");
	for (dp = dtab; dp != 0; dp = dp->d_next) {
		mp = dp->d_conn;
		if (dp->d_unit == QUES || dp->d_type != DEVICE || mp == 0 ||
		    mp == TO_NEXUS || mp->d_type == MASTER)
			continue;
		if (mp->d_conn == TO_NEXUS) {
			if (dp->d_addr == UNKNOWN) {
				printf("must specify csr for device %s%d\n",
				    dp->d_name, dp->d_unit);
				continue;
			}
			if (dp->d_drive != UNKNOWN || dp->d_slave != UNKNOWN) {
				printf("drives/slaves can be specified only ");
				printf("for controllers, not for device %s%d\n",
				    dp->d_name, dp->d_unit);
				continue;
			}
			if (dp->d_pri > 0 && dp->d_vec == 0 && (
			    dp->d_bus == SP_VME16D16 ||
			    dp->d_bus == SP_VME24D16 ||
			    dp->d_bus == SP_VME32D16 ||
			    dp->d_bus == SP_VME16D32 ||
			    dp->d_bus == SP_VME24D32 ||
			    dp->d_bus == SP_VME32D32)) {
				printf("Warning: should use vectored ");
				printf("interrupts for vme device %s%d\n",
				    dp->d_name, dp->d_unit);
			}
			slave = QUES;
		} else {
			if (mp->d_conn == 0) {
				printf("%s%d isn't connected to anything, ",
				    mp->d_name, mp->d_unit);
				printf("so %s%d is unattached\n",
				    dp->d_name, dp->d_unit);
				continue;
			}
			if (dp->d_drive == UNKNOWN) {
				printf("must specify ``drive number'' for %s%d\n",
				   dp->d_name, dp->d_unit);
				continue;
			}
			/* NOTE THAT ON THE UNIBUS ``drive'' IS STORED IN */
			/* ``SLAVE'' AND WE DON'T WANT A SLAVE SPECIFIED */
			if (dp->d_slave != UNKNOWN) {
				printf("slave numbers should be given only ");
				printf("for massbus tapes, not for %s%d\n",
				    dp->d_name, dp->d_unit);
				continue;
			}
			if (dp->d_pri != 0) {
				printf("interrupt priority should not be ");
				printf("given for drive %s%d\n",
				    dp->d_name, dp->d_unit);
				continue;
			}
			if (dp->d_addr != UNKNOWN) {
				printf("csr addresses should be given only");
				printf(" on controllers, not on %s%d\n",
				    dp->d_name, dp->d_unit);
				continue;
			}
			slave = dp->d_drive;
		}
		fprintf(fp,
"{ &%sdriver,\t%d,  %s,   %2d,     C 0x%08x, %d,   %d, 0x%x, %s, 0x%x },\n",
		    mp->d_conn == TO_NEXUS? dp->d_name : mp->d_name, dp->d_unit,
		    mp->d_conn == TO_NEXUS? " -1" : qu(mp->d_unit),
		    slave,
		    dp->d_addr == UNKNOWN? 0 : dp->d_addr,
		    dp->d_pri, dp->d_dk, dp->d_flags, intv(dp),
		    (MAKE_MACH(dp->d_mach) | dp->d_bus));
	}
	fprintf(fp, "\t0\n};\n");
	pseudo_inits(fp);
	(void) fclose(fp);
}
#endif defined(MACHINE_SUN2) || defined(MACHINE_SUN3)

pseudo_inits(fp)
	FILE *fp;
{
	register struct device *dp;
	int count;

	for (dp = dtab; dp != 0; dp = dp->d_next) {
		if (dp->d_type != PSEUDO_DEVICE || dp->d_init == 0)
			continue;
		fprintf(fp, "extern %s();\n", dp->d_init);
	}
	fprintf(fp, "\nstruct pseudo_init {\n");
	fprintf(fp, "\tint\tps_count;\n\tint\t(*ps_func)();\n");
	fprintf(fp, "} pseudo_inits[] = {\n");
	for (dp = dtab; dp != 0; dp = dp->d_next) {
		if (dp->d_type != PSEUDO_DEVICE || dp->d_init == 0)
			continue;
		count = dp->d_slave;
		if (count <= 0)
			count = 1;
		fprintf(fp, "\t%d,\t%s,\n", count, dp->d_init);
	}
	fprintf(fp, "\t0,\t0,\n};\n");
}

char *intv(dev)
	register struct device *dev;
{
	static char buf[20];

	if (dev->d_vec == 0)
		return ("     0");
	(void)sprintf(buf, "%sint%d", dev->d_name, dev->d_unit);
	return (buf);
}

char *
qu(num)
{

	if (num == QUES)
		return ("'?'");
	if (num == UNKNOWN)
		return (" -1");
	(void)sprintf(errbuf, "%3d", num);
	return (errbuf);
}
