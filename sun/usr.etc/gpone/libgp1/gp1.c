#ifndef lint
static	char sccsid[] = "@(#)gp1.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/* 
 * low level gp routines for loading microcode, restarting etc.
 */

#include <sys/types.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sun/gpio.h>
#include <stdio.h>
#include <pixrect/gp1reg.h>

gp1_open(gpdev, gp1_base, gp1_fd, allocp)
	char	*gpdev;
	short   **gp1_base;
	int      *gp1_fd;
	caddr_t  *allocp;
	{
	int p;
	int align, mmap_size;

	if ((*gp1_fd = open(gpdev, O_RDWR | O_NDELAY, 0)) < 0)
		{
		perror("gp1_open - can't open device");
		return(-1);
		}
	align = getpagesize();
	if ((*allocp = (caddr_t) malloc(VME_GP1SIZE + align)) == 0)
		{
		perror("gp1_open - can't malloc address space");
		close(*gp1_fd);
		return(-1);
		}
	p = ((int) *allocp + align - 1) & ~(align-1);
	if (mmap(p, VME_GP1SIZE, PROT_READ|PROT_WRITE, 
					MAP_SHARED, *gp1_fd, 0) == -1) {
		return(-1);
		}
	*gp1_base = (short *)p;
	return(0);
	}


gp1_close(gp1_fd, allocp)
	int gp1_fd;
	caddr_t allocp;

	{
	close(gp1_fd);
	free(allocp);
	}


gp1_reset(gp1_base)
	short *gp1_base;

	{
	caddr_t gp1_shmem;

	gp1_hwreset(gp1_base);
	gp1_primepipe(gp1_base);
	gp1_shmem = (caddr_t)gp1_base + GP1_SHMEM_OFFSET;
	gp1_swreset(gp1_shmem);
	return(0);
	}

gp1_primepipe(gp1_base)
	short *gp1_base;

	{
	register short *gp1_ucode;
	int i;
	short savedgpucode[4];
				  /* Get a harmless instruction into the
				     instruction registers for both vp and pp */
	gp1_ucode = &gp1_base[GP1_UCODE_DATA_REG];

	gp1_base[GP1_UCODE_ADDR_REG] = 0;    /* At microcode address 0 */
	for (i=0; i<4; i++)		     /* save start of ucode */
		savedgpucode[i] = *gp1_ucode;

	gp1_base[GP1_UCODE_ADDR_REG] = 0;    /* At microcode address 0 */
	*gp1_ucode = 0x0068;		     /* deposit a jump 0 instruction */
	*gp1_ucode = 0x0000;		     /* (for both vp and pp) */
	*gp1_ucode = 0x7140;
	*gp1_ucode = 0x0000;
	gp1_2p_start(gp1_base, 0);		     /* start both processors */
	gp1_hwreset(gp1_base);			     /* Now halt them */
				  /* At this point both vp and pp have jump 0
				     instructions in the IR, so that when they
				     are restarted with a new microprogram,
				     nothing funny will happen to the fifo,
				     shared memory, etc. on the first cycle */

	gp1_base[GP1_UCODE_ADDR_REG] = 0;    /* At microcode address 0 */
	for (i=0; i<4; i++)		     /* restore start of ucode */
		*gp1_ucode = savedgpucode[i];

	}


gp1_2p_start(gp1_base, cont_flag)
	short *gp1_base;
	int cont_flag;

	{
	register short *gp1_cntrl = &gp1_base[GP1_CONTROL_REG];

	*gp1_cntrl = 0;
	if (cont_flag)
		*gp1_cntrl = GP1_CR_VP_CONT | GP1_CR_PP_CONT;
	else
		*gp1_cntrl = GP1_CR_VP_STRT0 | GP1_CR_VP_CONT |
			     GP1_CR_PP_STRT0 | GP1_CR_PP_CONT;
	}


gp1_2p_halt(gp1_base) 
	short *gp1_base;
	{
	register short *gp1_cntrl = &gp1_base[GP1_CONTROL_REG];

	*gp1_cntrl = 0;
	*gp1_cntrl = GP1_CR_VP_HLT | GP1_CR_PP_HLT;
	}


gp1_hwreset(gp1_base)
	short *gp1_base;

	{
	gp1_base[GP1_CONTROL_REG] = GP1_CR_CLRIF | GP1_CR_INT_DISABLE |
					GP1_CR_RESET;
	gp1_base[GP1_CONTROL_REG] = 0;
	}


gp1_swreset(gp1_shmem)
	caddr_t gp1_shmem;

	{
	register int *shmem = (int *) gp1_shmem;
	register short i;

	i = 133;
	while (--i)
		*shmem++ = 0;
	*((int *) &gp1_shmem[10]) = 0x800000FF;
	}


gp1_load(gp1_base, filename)
	short *gp1_base;
	char *filename;

	{
	FILE *fp;
	u_short tadd, nlines;
	u_short ucode[4096 * 4];
	int nwords;
	register u_short *ptr;
	register short *gp1_ucode;

	if((fp=fopen(filename,"r"))==NULL)
		{
		fprintf(stderr, "can't open %s\n", filename);
		return(1);
		}
	gp1_ucode = &gp1_base[GP1_UCODE_DATA_REG];
	while(fread(&tadd, sizeof(tadd), 1, fp) == 1)
						/* starting microcode address */
		{
		gp1_base[GP1_UCODE_ADDR_REG] = tadd;
		fread(&nlines, sizeof(nlines), 1, fp);
						/* number of microcode lines  */
		while(nlines > 0)
			{
			nwords = (nlines > 4096) ? 4096 : nlines;
			nlines -= nwords;
			fread(ucode, sizeof(u_short),  4 * nwords, fp);	
			for (ptr = ucode; nwords > 0; nwords--)
				{
				*gp1_ucode = *ptr++;
				*gp1_ucode = *ptr++;
				*gp1_ucode = *ptr++;
				*gp1_ucode = *ptr++;
				}
			}
		}

	fclose(fp);
	return(0);
	}

gp1_vp_start(gp1_base, cont_flag)
	short *gp1_base;
	int cont_flag;

	{
	register short *gp1_cntrl = &gp1_base[GP1_CONTROL_REG];

	*gp1_cntrl = 0;
	if (cont_flag)
		*gp1_cntrl = GP1_CR_VP_CONT;
	else
		*gp1_cntrl = GP1_CR_VP_STRT0 | GP1_CR_VP_CONT;
	return(0);
	}


gp1_vp_halt(gp1_base)
	short *gp1_base;

	{
	register short *gp1_cntrl = &gp1_base[GP1_CONTROL_REG];

	*gp1_cntrl = 0;
	*gp1_cntrl = GP1_CR_VP_HLT;
	}


gp1_pp_start(gp1_base, cont_flag)
	short *gp1_base;
	int cont_flag;

	{
	register short *gp1_cntrl = &gp1_base[GP1_CONTROL_REG];

	*gp1_cntrl = 0;
	if (cont_flag)
		*gp1_cntrl = GP1_CR_PP_CONT;
	else
		*gp1_cntrl = GP1_CR_PP_STRT0 | GP1_CR_PP_CONT;
	return(0);
	}


gp1_pp_halt(gp1_base)
	short *gp1_base;

	{
	register short *gp1_cntrl = &gp1_base[GP1_CONTROL_REG];

	*gp1_cntrl = 0;
	*gp1_cntrl = GP1_CR_PP_HLT;
	}

int gp1_stblk_alloc(fd)
	int fd;

	{
	int i;

	if( ioctl(fd, GP1IO_GET_STATIC_BLOCK, &i) )
		return (-1);
	else
		return(i);
	}

gp1_stblk_free(fd, num)
	int num;
	{

	if ( ioctl(fd, GP1IO_FREE_STATIC_BLOCK, &num) )
		return(-1);
	else
		return(0);
	}
