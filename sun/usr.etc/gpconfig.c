#ifndef lint
static	char sccsid[] = "@(#)gpconfig.c 1.3 87/01/09 Copyr 1985 Sun Micro";
#endif
/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * gpconfig.c	- binds frame buffers to the GP
 */

/*
 * Right now only cg2 devices (including cg3) can use the GP.
 * When new devices are added, this code should change appropriately.
 */

#include <pixrect/pixrect_hs.h>
#include <pixrect/gp1reg.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sun/fbio.h>
#include <sun/gpio.h>
#include <stdio.h>
#include <strings.h>

#define TRUE	1
#define FALSE	0

#define GP1_TYPE_ORIG		0
#define GP1_TYPE_PLUS		1
#define GP1_TYPE_PLUS_PLUS	2

#define	UCODE_1024		"/usr/lib/gp1cg2.1024.ucode"
#define	UCODE_1152		"/usr/lib/gp1cg2.1152.ucode"
#define	UCODE_1024_PLUS		"/usr/lib/gp1+cg2.1024.ucode"
#define	UCODE_1152_PLUS		"/usr/lib/gp1+cg2.1152.ucode"
#define	UCODE_1024_PLUS_PLUS	"/usr/lib/gp1++cg2.1024.ucode"
#define	UCODE_1152_PLUS_PLUS	"/usr/lib/gp1++cg2.1152.ucode"

main(argc, argv)
	int argc;
	char *argv[];
{
	register unsigned short *gp1_ucode;
	register short *ptr;
	register int i;
	int gp1_fd, fb_fd, argindex, fbunit;
	int set_fbtype, set_fbwidth;
	int board_binding_count, setdevfbflag, setbufflag, ucode_loaded;
	int gp1type;
	unsigned short *gp1_base;
	int bufunit, devfbunit;
	unsigned short ucode[24];
	short gp_physaddr, fb_physaddr[4];
	short okay_to_bind;
	char gpname[128], fbname[128], bndname[128];
	char fbdevname[128], bufname[128];
	char *ucode_filename;
	caddr_t gp1_va, gp1_shmem;
	struct fbtype fb;
	struct fbinfo fbinfo;
	struct fbgattr fbgattr;

	/* test argument count */
	if (argc == 1) {
		fprintf(stderr, "usage: %s gpunit [ [-b] [-f] fbunit ...]\n",
		    argv[0]);
		exit (-1);
	}

	/* initialize physical addresses to be poked into microcode */
	gp_physaddr    = -1;
	fb_physaddr[0] = -1;
	fb_physaddr[1] = -1;
	fb_physaddr[2] = -1;
	fb_physaddr[3] = -1;

	/* open with O_NDELAY flag to avoid checking a fb board */
	/* use gp1_open ... since we will need the base address */
	strcpy(gpname, "/dev/");
	strcat(gpname, argv[1]);
	strcat(gpname, "a");
	if (gp1_open(gpname, &gp1_base, &gp1_fd, &gp1_va)) {
			fprintf(stderr, "open failed for %s\n", gpname);
			exit(1);
	}
	
	/* if sanity check for gp fb type fails then continue */
        if (ioctl(gp1_fd, FBIOGTYPE, &fb) == -1) {
		fprintf(stderr,
		    "problem determining frame buffer type for %s\n", gpname);
		close(gp1_fd);
		exit(1);
	}
	
	if (fb.fb_type !=  FBTYPE_SUN2GP) {
		fprintf(stderr, "incorrect frame buffer type for %s\n", gpname);
		close(gp1_fd);
		exit(1);
	}

	/* get physical address of the GP */
	if (ioctl(gp1_fd, FBIOGINFO, &fbinfo) == -1) {
		fprintf(stderr, "problem getting physical addr of %s\n",gpname);
		close(gp1_fd);
		exit(1);
	}
	gp_physaddr = (fbinfo.fb_physaddr >> 16);

	board_binding_count = 0;
	set_fbtype   = -1;
	set_fbwidth  = -1;
	bufunit      = -1;
	devfbunit    = -1;
	fbunit       = -1;
	setdevfbflag = 0;
	setbufflag   = 0;
	ucode_loaded = 0;
	ucode_filename = NULL;

	gpname[strlen(gpname)-1] = '\0';

	/* for all listed devices */
	for (argindex = 2; argindex < argc; argindex++) {

		/* check for flags */
		if (!strcmp(argv[argindex], "-f")) {
			setdevfbflag  = 1;
			continue;
		}

		if (!strcmp(argv[argindex], "-b")) {
			setbufflag = 1;
			continue;
		}

		if (!strcmp(argv[argindex], "-u")) {
			ucode_filename = argv[++argindex];
			continue;
		}

		fbunit++;

		/* can only configure 4 frame buffers */
		if (fbunit >= 4) {
			fprintf(stderr,
			    "can only bind 4 frame buffers to the GP.\n");
			continue;
		}

		strcpy(fbname, "/dev/");
		strcat(fbname, argv[argindex]);
		fb_fd = open(fbname, O_RDWR, 0);
		/* if open of fb  device fails then continue */
		if (fb_fd == -1) {
			fprintf(stderr, "open failed for %s, not bound.\n",
			    fbname);
			continue;
		}

		/* if sanity check on frame buffer type fails then continue */
		if (ioctl(fb_fd, FBIOGTYPE, &fb) == -1) {
			fprintf(stderr,
		"problem determining frame buffer type for %s, not bound.\n",
			    fbname);
			close(fb_fd);
			continue;
		}

	        if (fb.fb_type !=  FBTYPE_SUN2COLOR) {
			fprintf(stderr,
			    "invalid frame buffer type for %s, not bound.\n",
			    fbname);
			close(fb_fd);
			continue;
		}

		/* board must be like all previously bound boards */
		if (set_fbtype != -1 &&
		    (fb.fb_type != set_fbtype || fb.fb_width != set_fbwidth)) {
			fprintf(stderr,
	"problem binding %s. all frame buffers must be of the same type\n",
			    fbname);
			continue;
		}

		if (ioctl (fb_fd, FBIOGATTR, &fbgattr) == -1) {
			fprintf(stderr,"Problem getting buffer attr, %s %s.\n",
			    gpname, fbname);
			continue;
		}

		if (ioctl (gp1_fd, FBIOSATTR, &(fbgattr.sattr)) == -1) {
			fprintf(stderr,"Problem setting buffer attr, %s %s.\n",
			    gpname, fbname);
			continue;
		}

		/* load microcode */
		if (!ucode_loaded) {

		    /* need to reset the GP */
		    gp1_reset (gp1_base);

		    /* 
		     * determine type of gp1 by poking location 1
		     * with wraparound addresses
		     */

		    /* load 8K case */
		    gp1_base[GP1_UCODE_ADDR_REG] = 0;
		    gp1_base[GP1_UCODE_DATA_REG] = 8;

		    /* load 32K case */
		    gp1_base[GP1_UCODE_ADDR_REG] = 1<<14;
		    gp1_base[GP1_UCODE_DATA_REG] = 32;

		    /* load 16K case */
		    gp1_base[GP1_UCODE_ADDR_REG] = 1<<13;
		    gp1_base[GP1_UCODE_DATA_REG] = 16;

		    /* now figure out what we have by checking for wraparound */
		    gp1_base[GP1_UCODE_ADDR_REG] = 0;
		    switch (gp1_base[GP1_UCODE_DATA_REG] & 0xFF) {
			case 8:
				gp1type = GP1_TYPE_PLUS_PLUS;
				printf("GP has 32K of microcode memory\n");
				break;
			case 32:
				gp1type = GP1_TYPE_PLUS;
				printf("GP has 16K of microcode memory\n");
				break;
			case 16:
				gp1type = GP1_TYPE_ORIG;
				printf("GP has 8K of microcode memory\n");
				break;
		    }

		    if (ucode_filename == NULL) {

			/* load square screen */
			if (set_fbwidth == 1024) {
			  switch (gp1type) {
			    case GP1_TYPE_PLUS_PLUS:
				if (fopen(UCODE_1024_PLUS_PLUS, "r") != 0)
					ucode_filename = UCODE_1024_PLUS_PLUS;
				else if (fopen(UCODE_1024_PLUS, "r") != 0)
					ucode_filename = UCODE_1024_PLUS;
				else
					ucode_filename = UCODE_1024;
				break;
			    case GP1_TYPE_PLUS:
				if (fopen(UCODE_1024_PLUS, "r") != 0)
					ucode_filename = UCODE_1024_PLUS;
				else
					ucode_filename = UCODE_1024;
				break;
			    case GP1_TYPE_ORIG:
					ucode_filename = UCODE_1024;
				break;
			  }
			} else {

			/* load 1152 x 900 screen */
			  switch (gp1type) {
			    case GP1_TYPE_PLUS_PLUS:
				if (fopen(UCODE_1152_PLUS_PLUS, "r") != 0)
					ucode_filename = UCODE_1152_PLUS_PLUS;
				else if (fopen(UCODE_1152_PLUS, "r") != 0)
					ucode_filename = UCODE_1152_PLUS;
				else
					ucode_filename = UCODE_1152;
				break;
			    case GP1_TYPE_PLUS:
				if (fopen(UCODE_1152_PLUS, "r") != 0)
					ucode_filename = UCODE_1152_PLUS;
				else
					ucode_filename = UCODE_1152;
				break;
			    case GP1_TYPE_ORIG:
					ucode_filename = UCODE_1152;
				break;
			  }
			}
		    }

		    /* load microcode file */
		    if (gp1_load(gp1_base, ucode_filename)) {
		    	fprintf(stderr, 
		    	    "problem loading gp/cg2 microcode file %s\n",
			    ucode_filename);
			close(fb_fd);
			close(gp1_fd);
			exit (1);
		    }

		    printf("%s microcode file loaded\n", ucode_filename);

		    ucode_loaded = 1;
		}

		/* let the gp driver know fb specific information */
		if (ioctl(fb_fd, FBIOGINFO, &fbinfo) == -1) {
			fprintf(stderr, "problem binding %s to %s\n",
			    fbname, gpname);
			close(fb_fd);
			continue;
		}

		/* see if fb unit is already bound */
		okay_to_bind = TRUE;
		for (i = 0; i < fbunit; i++) {
			if ((fbinfo.fb_physaddr >> 16) == fb_physaddr[i])
				okay_to_bind = FALSE;
		}
		if (!okay_to_bind) {
			fprintf(stderr, "cannot bind the same fb (%s) twice\n",
			    fbname);
			close(fb_fd);
			continue;
		}
		
		fbinfo.fb_unit = fbunit;
		if (ioctl(gp1_fd, GP1IO_PUT_INFO, &fbinfo) == -1) {
			fprintf(stderr, "problem binding %s to %s\n",
			    fbname, gpname);
			close(fb_fd);
			continue;
		}

		/* set the fb unit using the gbuffer (if present) */
		fb_physaddr[fbunit] = (fbinfo.fb_physaddr >> 16);
		if (setbufflag) {
			int gbufflag;
 
			ioctl(gp1_fd, GP1IO_CHK_FOR_GBUFFER, &gbufflag);
			if (gbufflag == 1) {
				bufunit = fbunit;
			}
			setbufflag = 0;
		}


		/* set the fb unit using the /dev/fb indirection */
		/* will redirect AFTER the GP has been loaded and started */
		if (setdevfbflag) {
			devfbunit = fbunit;
			setdevfbflag = 0;
		}

		/* another board has been successfully bound */
		strcpy(bndname, gpname);
		gpname[strlen(gpname)+1] = '\0';
		bndname[strlen(gpname)] = 'a' + fbunit;
		printf("%s and %s bound as %s\n", gpname, fbname, bndname);
		board_binding_count++;

		/* if 1st board configured then all boards must be this type */
		if (set_fbtype == -1) {
			set_fbtype = fb.fb_type;
			set_fbwidth = fb.fb_width;
		}

		/* done with the fb unit for now */
		close(fb_fd);
	}

	if (board_binding_count != 0) {

		/* mark which frame buffer has the buffer (if any) */
		ioctl(gp1_fd, GP1IO_SET_USING_GBUFFER, &bufunit);

		/* rip up the start of the microcode and retile with */
		/* physical addresses of the gp and bound color boards */
		gp1_ucode = &gp1_base[GP1_UCODE_DATA_REG];

		/* rip up the microcode */
		gp1_base[GP1_UCODE_ADDR_REG] = 4;
		for (i = 0; i < 24; i++)
			ucode[i] = *gp1_ucode;

		/* update gp and (4) fb  physical addresses */
		/* and the index of fb using the gbuffer */
		ucode[3]  = gp_physaddr;
		ucode[7]  = fb_physaddr[0];
		ucode[11] = fb_physaddr[1];
		ucode[15] = fb_physaddr[2];
		ucode[19] = fb_physaddr[3];
		ucode[23] = bufunit;

		/* retile the microcode */
		gp1_base[GP1_UCODE_ADDR_REG] = 4;
		for (i = 0; i < 24; i++)
			*gp1_ucode = ucode[i];

		gp1_vp_start(gp1_base, 0);
		gp1_pp_start(gp1_base, 0);

		/* print out who got the buffer */
		if (bufunit != -1) {
			strcpy(bufname, gpname);
			bufname[strlen(bndname)-1] = 'a' + bufunit;
			printf("%s to use the graphics buffer\n", bufname);
		}

		/* redirect /dev/fb if requested */
		if (devfbunit != -1) {
			if (ioctl(gp1_fd, GP1IO_REDIRECT_DEVFB, &devfbunit)) {
				fprintf(stderr, "problem redirecting /dev/fb\n");
			} else {
				strcpy(fbdevname, gpname);
				fbdevname[strlen(bndname)-1] = 'a' + devfbunit;
				printf("/dev/fb redirected to %s\n", fbdevname);
			}
		}
	}

	close(gp1_fd);
	exit (0);
}
