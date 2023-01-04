#ifndef lint
static	char sccsid[] = "@(#)sd.c 1.5 87/02/27";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * SCSI device driver for diag.
 */

#include "diag.h"
#include <sys/types.h>
#include <sys/dkbad.h>
#include <sys/buf.h>
#include <sun/dklabel.h>
#include <sun/dkio.h>
#include <sundev/screg.h>
#include <sundev/sireg.h>
#include <sundev/sdreg.h>
#include <sundev/scsi.h>
#include "def.h"
#include "sd.h"

#define SC_FORMAT_UNIT		0x04
#define SC_REASSIGN_BLOCK	0x07
#define SC_TRANSLATE		0x0f	/* not supported for CCS */
#define SC_MODE_SELECT 		0x15
#define SC_MODE_SENSE 		0x1a
#define SC_INQUIRY		0x12
#define SC_RECEIVE_DIAGNOSTIC	0x1c
#define SC_SEND_DIAGNOSTIC	0x1d
#define SC_VERIFY 		0x2f
#define SC_READ_CAPACITY	0x25
#define SC_READ_DEFECT_DATA	0x37
#define SC_READ_LONG		0xe8	/* not really used */

/* Page Codes */
#define ERR_RECOVERY_PARMS	1
#define DISCO_RECO_PARMS	2
#define FORMAT_PARMS		3
#define GEOMETRY_PARMS		4
#define CERTIFICATION_PARMS	6

#define	SCSI_HOST_ADDR	0x80

#define SI_OB_DVMA_OFFSET	(DVMA - KERNELBASE)

#define UDCT_PA		(struct udc_table *)0x100
#define UDCT_VA		(struct udc_table *)(DVMA + (int)UDCT_PA)

#define	BYTES_PER_TRACK	10416	/* fixed by ST-506 interface */
#define ESDI_BYTES_PER_TRACK 22000	/* pretty close to most esdi disks */

#define INTERLEAVE	1	/* interleave factor on disk */

#define	MAX_PRINT	20	/* max # of defects to print before pause */

#define	DEFECT_MAGIC	0xdefe	/* arbitrary */

extern struct defect_list defect[];
#define NO_QUERY	0	/* used with reassign for surface analysis  */
#define QUERY		1	/* or user input */
#define NO_QADD		2	/* no query and don't add to list (format) */

int mult = 1;		/* 1 for 512 byte sectors, 2 for 256 byte sectors */
int bias_inc;		/* increment when biasing defects added by hand */
int bias_dec;		/* decrement when biasing defects added by hand */
int bias_cnt = 1;	/* number to entries to generate for each defect */
int code_1 = FALSE;	/* flag for SCSI code group 1 commmands */
int scsi_silent = 0;	/* silent flag for reading defect list (hack) */
int esdi_virgin;	/* set for a brand new disk (can't get internal list) */
int esdi_corrupt;	/* set for a disk w/ no manufacturer's list */

char *md21 = "EMULEX";		/* for vendor identification */
char *acb4520 = " ADAPTEC";

/*
 * Execute a command on the drive.
 */
/*ARGSUSED*/
SCcmd(cmd, p1, p2, p3, p4, p5, p6, p7, p8)
{
	register int i;

	switch (cmd) {
	case STATUS:
		scsi_pr_status();
		return 0;

	case INIT:
		/* first reset the SCSI bus */
		scsi_reset();
		return SCexec(SC_TEST_UNIT_READY, 0, 0, 0, 0, 0, 0);

	case FORMAT:
		return (scsi_format());

	case VERIFY:
		return SCexec(SC_READ, p1, p2,  p3, p4, DBUF_PA, 0);

	case MAP:
		printf("map not supported.\n");
		scsi_abort();
		return 0;

	case SEEK:
		if (SCexec(SC_SEEK, p1, 0, 0, 0, 0, 0))
			return -1;
		/*
		 * Seek worked.  Now wait for ctlr to be
		 * not busy, i.e. for arm to get there.
		 */
		for (i = 0; i < 100; i++)
			if (SCexec(SC_TEST_UNIT_READY, 0, 0, 0, 0, 0, 1) == 0)
				return 0;
		return -1;

	case READ:
		return SCexec(SC_READ, p1, p2, p3, p4, p5, 0);

	case WRITE:
		return SCexec(SC_WRITE, p1, p2, p3, p4, p5, 0);

	case RESTORE:
		return SCexec(SC_REZERO_UNIT, 0, 0, 0, 0, 0, 0);

	default:
		return -1;		/* not available */
	}
}

/*
 * Common code for executing commands.
 */
SCexec(cmd, cyl, head, sec, cnt, buf, errflag)
	int cmd, cyl, head, sec, cnt, buf, errflag;
{
	struct scsi_cdb cdb;
	struct scsi_scb scb;
	struct scsi_sense sense;
	struct scsi_ext_sense *sense7;
	int sector, max_sector, retry = 0;
	int len;

	if ((!scsi_silent) && infomsgs)
		printf("SCexec. cmd %x cyl %d head %d sec %d cmd %d buf %x\n",
			cmd, cyl, head, sec, cmd, buf);

	do {
		bzero((char *)&cdb, sizeof cdb);
		cdb.cmd = cmd;
		cdb.lun = unit;
		sector = ((cyl * (nhead * nsect - nspare)) +
		    (head * nsect) + sec) * mult;
		if (controller == C_ADAPTEC)
			max_sector = (ncyl + acyl) * nhead * nsect - 1
			    - format_parms.list_len / 8;
		else
			max_sector = (ncyl + acyl) * nhead * nsect - 1;
		if (sector > max_sector && cmd != SC_REQUEST_SENSE
				&& cmd != SC_TEST_UNIT_READY) {
			if (!scsi_silent) {
			printf("%d/%d/%d past end of disk \n", cyl, head, sec);
			}
			return 0;
		}	
		cdb.low_addr = sector & 0xff;
		sector >>= 8;
		cdb.mid_addr = sector & 0xff;
		sector >>= 8;
		cdb.high_addr = sector;
		cdb.count = cnt * mult;
		if (cmd == SC_READ || cmd == SC_WRITE)
			len = cnt * SECSIZE;
		else
			len = cnt;
		if (scsi_doit(&cdb, &scb, &sense, buf, len))
			break;
		if (cmd==SC_TEST_UNIT_READY && scb.chk && sense.adr_val==0 &&
				controller == C_ADAPTEC && (!scsi_silent)) {
			printf("Bad format on volume.\n");
			return 0;
		}
		if (cmd == SC_TEST_UNIT_READY && scb.chk && sense.class == 7
			&& retry < 2 && controller != C_ADAPTEC) {
			sense7 = (struct scsi_ext_sense *)&sense;
			if (sense7->key == 6) {
				/*
				 * CCS gets a Unit Attention on first 
				 * test unit ready.
				 */
				return 0;
			}
		}
		if (cmd == SC_SEEK && scb.busy && scb.chk == 0)
			return 0;	/* ctlr is busy until seek complete */

		if (errflag && scb.busy && scb.chk == 0)
			return 0;

		if ((!scsi_silent) && errors) {
			pr_scb(&scb);
			pr_sense(&sense);
			printf("%s error, cyl=%d, head=%d, sector=%d retry=%d\n",
				SC_cmdlist[cmd], cyl, head, sec, retry);
		}
	} while (retry++ < max_retries);

	if (retry > max_retries)
		retry = max_retries;

	if (scb.chk == 0) {
		if ((!scsi_silent) && infomsgs)
			printf("%s worked, cyl=%d head=%d sector=%d retry=%d\n",
				SC_cmdlist[cmd], cyl, head, sec, retry);
		return 0;
	}
	if (!scsi_silent) {
		printf("%s failed, cyl=%d, head=%d, sector=%d ",
			SC_cmdlist[cmd], cyl, head, sec);
		pr_scb(&scb);
		if (scb.chk)
			pr_sense(&sense);
	}
	return -1;
}

/*
 * Issue command to bus and get results.
 */
scsi_doit(cdb, scb, sense, buf, len)
	struct scsi_cdb *cdb;
	struct scsi_scb *scb;
	struct scsi_sense *sense;
	int len;
{
	if (si_ha_type) {
		return si_doit((struct scsi_si_reg *)devaddr, cdb, scb, 
			sense, buf, len);
	} else {
		return sc_doit((struct scsi_ha_reg *)devaddr, cdb, scb, 
			sense, buf, len);
	}
}

/*
 * Scsi reset.
 */
scsi_reset()
{
	if (si_ha_type) {
		si_reset((struct scsi_si_reg *)devaddr);
	} else {
		sc_reset((struct scsi_ha_reg *)devaddr);
	}
	printf("reset the SCSI bus...");
	if (controller == C_CCS_EMULEX)
		DELAY(1000000);	/*
				 * Small delay for controller internals
				 * may be needed!?
				 * Might need to be larger!?
				 */
	printf("\n");
	/* reset code group one command block semaphore, just in case */
	code_1 = FALSE;	
	SCexec(SC_TEST_UNIT_READY, 0, 0, 0, 0, 0, 0);
}

/*
 * Abort command due to SCSI error.
 */
scsi_abort()
{

	if (infomsgs) {
		printf("SCSI abort? ");
		if (!confirm())
			return;
	}
	scsi_reset();
	_longjmp(abort_jmp, 1);
	/*NOTREACHED*/
}

scsi_pr_status()
{
	if (si_ha_type) {
	    si_pr_csr("status", (int)(((struct scsi_si_reg *)devaddr)->csr));
	} else {
	    sc_pr_icr("status", (int)(((struct scsi_ha_reg *)devaddr)->icr));
	}
}

/*
 * Format
 */
scsi_format()
{
	char buf[LINEBUFSZ];
	jmp_buf save_buf;
	register int i;

	printf("SCSI format.\n");
	esdi_virgin = 0;
	esdi_corrupt = 0;
	scsi_silent++;
	if (controller == C_CCS_EMULEX)
		read_ccs_list();
	else
		read_list();
	scsi_silent = 0;
	for (i = 0; i < sizeof save_buf / sizeof save_buf[0]; i++)
		save_buf[i] = abort_jmp[i];
	_setjmp(abort_jmp);
	for (;;) {
		printf("format> ");
				/* needed if reset just occurred */
		gets(buf);
		switch (buf[0]) {
		case '?':
		case 'h':
			printf("SCSI Format subcommands:\n");
			printf("\tf: format disk\n");
			printf("\tp: print defect lists\n");
			printf("\ta: add defect to physical list\n");
			printf("\tb: bias added defects\n");
			printf("\td: delete defect from lists\n");
			printf("\tc: clear defect lists\n");
			printf("\ts: surface analysis\n");
			printf("\tr: reassign logical block\n");
			printf("\tt: translate logical block # to physical\n");
			printf("\tq: quit format\n");
			break;
		case 'f':
			format();
			break;
		case 'p':
			print_list();
			break;
		case 'd':
			delete_from_list();
			break;
		case 'a':
			add_to_list();
			break;
		case 'b':
			bias_added_defects();
			break;
		case 'c':
			clear_list();
			break;
		case 's':
			if (controller == C_ADAPTEC)
				surf_anal();
			else
				ccs_anal();	/* CCS verify test */
			break;
		case 'r':
			reassign(0, QUERY);
			break;
		case 't':
			translate();
			break;
		case 'q':
			for (i = 0; i < sizeof save_buf / sizeof save_buf[0];
			    i++) {
				abort_jmp[i] = save_buf[i];
			}
			return 0;
		default:
			printf("Unknown command.\n");
			printf("For help, type 'h' or '?'\n");
			break;
		}
	}
	/*NOTREACHED*/
}

format()
{
	struct scsi_cdb cdb;
	struct scsi_scb scb;
	struct scsi_sense sense;
	struct scsi_ext_sense sense7;
	int i, rc, len, lba;

	printf("DISK FORMAT - DESTROYS ALL DISK DATA!\n");
	printf("are you sure? ");
	if (!confirm())
		return;
	/*
	 * First, make sure the drive is ready.
	 * It may take a couple of tries if it is unformatted.
	 */
	bzero((char *)&cdb, sizeof cdb);
	cdb.cmd = SC_TEST_UNIT_READY;
	cdb.lun = unit;
	i = 0;
	while (scsi_doit(&cdb, &scb, &sense7, 0, 0) == 0) {
		if (++i > 100) {
			printf("Unit not ready.\n");
			return;
		}
	}
	/*
	 * Rezero unit make sure it's stay on track 0
	 * before formatting the drive.
	 */
	bzero((char *)&cdb, sizeof cdb);
	cdb.cmd = SC_REZERO_UNIT;
	cdb.lun = unit;
	i = 0;
	while (scsi_doit(&cdb, &scb, &sense, 0, 0) == 0) {
		if (++i > 3) {
			if (controller == C_ADAPTEC && (sense.class == 1)
					&& (sense.code == 0xC)) {
				printf("unformatted drive.\n");
				break;
			} else {
				printf("rezero unit failed after 3 retries.\n");
				if (scb.chk)
					pr_sense(&sense);
				return;
			}
		}
	} 

	mult = 1;
		/* a mode select routine for Adaptec or CCS only */
	if (controller == C_ADAPTEC) {
		ad_mode_select();
	} else
		ccs_mode_select();
retry:	if (controller == C_ADAPTEC)
		format_parms.reserved = 0;	/* controller demands this */
	else if (esdi_corrupt)
		format_parms.reserved = 0xd0;	/* use only our list */
	else
		format_parms.reserved = 0x90;	/* use both lists */
	bzero((char *)&cdb, sizeof cdb);
	cdb.cmd = SC_FORMAT_UNIT;
	cdb.lun = unit;
	cdb.count = INTERLEAVE;
	cdb.high_addr = 0x1C;
	len = format_parms.list_len + 4;
	if (format_parms.list_len == 0 && controller == C_ADAPTEC) {
		cdb.high_addr = 0x08; 
		len = 0;
	}
	*(struct format_parms *)DBUF_VA = format_parms;
	if (infomsgs) {
		printf("\tformat parms: ");
		for (i = 0; i < 16; ++i) {
			printf("%x ", ((u_char *)DBUF_VA)[i]);
		}
		printf("\n");
	}
	printf("formatting ...");
	rc = scsi_doit(&cdb, &scb, &sense7, DBUF_PA, len);
	if (rc == 0 && esdi_virgin && 
			sense7.error_code == 0x1c) {
		printf("\nWarning: Manufacturer's defect list is missing\n");
		printf("Formatting with only grown defects\n");
		esdi_corrupt = 1;
		esdi_virgin = 0;
		goto retry;
	}
	if (rc == 0) {
		printf(" format failed.\n");
		pr_scb(&scb);
		if (scb.chk)
			pr_sense(&sense7);
	} else
		printf(" done.\n");
	if (esdi_virgin) {
		scsi_silent++;
		read_ccs_list();
		scsi_silent = 0;
		esdi_virgin = 0;
	}
	if (controller == C_ADAPTEC)
		write_list();
	else {
		if (grown_defs.list_len > 0) {
			printf("Reassigning grown defects...\n");
			for (i = 0; i < (grown_defs.list_len / 8); i++) {
				lba = grown_defs.defect[i].cyl *
					(nsect * nhead - nspare);
				lba += grown_defs.defect[i].head * nsect;
				lba += grown_defs.defect[i].grown_sect;
				reassign(lba, NO_QADD);
			}
			printf(" done.\n");
		}
		scsi_putdef();
	}
}

/*
 * Mode select for Adaptec.
 */
ad_mode_select()
{
	struct scsi_cdb cdb;
	struct scsi_scb scb;
	struct scsi_sense sense;
	struct mode_set_parms {
		long	edl_len;
		long	reserved;
		long	bsize;
		unsigned fmt_code :8;
		unsigned ncyl     :16;
		unsigned nhead    :8;
		short	rwc_cyl;
		short	wprc_cyl;
		char	ls_pos;
		char	sporc;
	} *mp;

	bzero((char *)&cdb, sizeof cdb);
	cdb.cmd = SC_MODE_SELECT;
	cdb.lun = unit;
	cdb.count = 22;	  /* Byte count to transfer from DBUF_VA. */
	mp = (struct mode_set_parms *)DBUF_VA;
	mp->edl_len = 8;
	mp->reserved = 0;
	mp->bsize = SECSIZE;
	mp->fmt_code = 1;
	mp->ncyl = ncyl + acyl;
	mp->nhead = nhead;
	mp->rwc_cyl = physpart;/* this is actually write precomp-see manual */
	mp->wprc_cyl = 0;
	mp->ls_pos = 0;
	mp->sporc = basehead;	/* this is the seek stepping rate */
	if (scsi_doit(&cdb, &scb, &sense, (int)DBUF_PA, 22) == 0) {
		printf("mode select failed.\n");
		pr_scb(&scb);
		pr_sense(&sense);
		return;
	}
}

/*
 * Mode select for CCS 
 */
ccs_mode_select()
{
	struct scsi_cdb cdb;
	struct scsi_scb scb;
	struct scsi_sense sense;
	struct scsi_ext_sense sense7;
	struct ms_format_big {
		struct	ccs_modesel_head ccs_modesel_head;
		struct	ccs_format ccs_format;
		struct	ccs_disco_reco ccs_disco_reco;
	} ms_format_big;	
	int i, x, tmp_length;
 
	bzero((char *)&cdb, sizeof cdb);
	bzero((char *)&ms_format_big, sizeof ms_format_big);
	cdb.cmd = SC_MODE_SELECT;
	cdb.lun = unit;
       	x = cdb.count = sizeof ms_format_big;
	ms_format_big.ccs_modesel_head.block_desc_length = 8;
	ms_format_big.ccs_modesel_head.block_length = 512;
	ms_format_big.ccs_format.page_header.page_code = FORMAT_PARMS;
	ms_format_big.ccs_format.page_header.page_length = 0x16;
	ms_format_big.ccs_format.tracks_per_zone = 1;
		/* emulex uses 1 spare sector per track */
	ms_format_big.ccs_format.alt_sect_zone = 1; 
		/* emulex needs extra tracks for bookeeping */
	ms_format_big.ccs_format.alt_tracks_vol = nhead;
		/* feed back the number retrieved from mode sense */
	ms_format_big.ccs_format.sect_track = nsect + 1;
	ms_format_big.ccs_format.data_sect = 512;
	ms_format_big.ccs_format.interleave = 1;
	ms_format_big.ccs_disco_reco.page_header.page_code = DISCO_RECO_PARMS;
	ms_format_big.ccs_disco_reco.page_header.page_length = 0xa;
	ms_format_big.ccs_disco_reco.bus_inactivity_limit = 30;
	*(struct ms_format_big *)DBUF_VA1 = ms_format_big;
	if (infomsgs) {
		for (i = 0; i < x; ++i)
			printf(" %x ", ((u_char *)DBUF_VA1)[i]);
		printf("\n");
	}
	if (scsi_doit(&cdb, &scb, &sense7, (int)DBUF_PA1, x) == 0) {
		printf("mode select failed.\n");
		pr_scb(&scb);
		pr_sense(&sense7);
		return(0);
	}
}

/*
 * Print defect list.
 */
print_list()
{
	register struct defect *dp;
	int i, j, n, ndef = 0;
	char buf[LINEBUFSZ];

	i = j = 0;
	n = format_parms.list_len / 8;
	if (n > 0) {
		ndef ++;
		printf("\nDefect List - Physical Format\n");
		printf("Defect\tCylinder\tHead\t\tBytes From Index\n");
		for (i = 0; i < n; i++) {
			dp = &format_parms.defect[i];
			if (dp->bytes_from_index == -1)
				printf("%d\t%d\t\t%d\t\ttrack mapped\n", i + 1,
					dp->cyl, dp->head);
			else
				printf("%d\t%d\t\t%d\t\t%d\n", i + 1, dp->cyl, 
					dp->head, dp->bytes_from_index);
			if (((i + 1) % MAX_PRINT) == 0 && i + 1 < n) {
				printf("Press return to continue ");
				gets(buf);
			}
		}
	}
	n = grown_defs.list_len / 8;
	if (n > 0) {
		ndef++;
		printf("\nReassigned Block List - Logical Format\n");
		printf("Defect\tCylinder\tHead\t\tSector\n");
		for (j = 0; j < n; j++) {
			dp = &grown_defs.defect[j];
			printf("%d\t%d\t\t%d\t\t%d\n", i + j + 1, dp->cyl,
			    dp->head, dp->grown_sect);
		if (((i + j) % MAX_PRINT) == 0 && j + 1 < n && i + j > 0) {
				printf("Press return to continue ");
				gets(buf);
			}
		}
	}
	if (!ndef)
		printf("\nNo defects\n");
	printf("\n");
}

bias_added_defects()
{

	if (controller != C_ADAPTEC) {
		printf("Not supported for this controller\n");
		return;
	}
	bias_dec = pgetn("Enter byte_following_index decrement: ");
	bias_inc = pgetn("Enter byte_following_index increment: ");
	bias_cnt = (bias_dec > 0 && bias_inc > 0) ? 2 : 1;
}

/*
 * Add a defect to the list.
 */
add_to_list()
{
	register struct defect *dp;
	int bytes_per_track;
	
	if (controller != C_ADAPTEC) {
		printf("new defects are usually entered with 'reassign'\n");
		bytes_per_track = ESDI_BYTES_PER_TRACK;
	} else
		bytes_per_track = BYTES_PER_TRACK;
	if ((controller == C_ADAPTEC &&
	    format_parms.list_len/8 >= (ST506_NDEFECT - bias_cnt)) ||
	    (format_parms.list_len + grown_defs.list_len)/8 >= ESDI_NDEFECT) {
		printf("TOO MANY DEFECTS.  IGNORED.  DISK UNUSABLE.\n");
		return;
	}
	dp = &format_parms.defect[format_parms.list_len/8];
	for (;;) {
		dp->cyl =	      pgetn("cylinder?	 ");
		if (dp->cyl >= ncyl + acyl)
			printf("\007Cylinder number out of range.\n");
		else
			break;
	}
	for (;;) {
		dp->head =	     pgetn("head?	     " );
		if (dp->head >= nhead)
			printf("\007Head number out of range.\n");
		else
			break;
	}
	for (;;) {
		dp->bytes_from_index = pgetn("bytes from index? ");
		if (dp->bytes_from_index < 0 || 
		    dp->bytes_from_index >= bytes_per_track)
			printf("\007Byte number out of range.\n");
		else
			break;
	}
	format_parms.list_len += 8;

	if (bias_cnt == 2) {
		dp[1] = dp[0];
		format_parms.list_len += 8;
	}
	if (bias_dec && dp->bytes_from_index >= bias_dec) {
		dp->bytes_from_index -= bias_dec;
		dp++;
	}
	if (bias_inc && dp->bytes_from_index+bias_inc < bytes_per_track) {
		dp->bytes_from_index += bias_inc;
		dp++;
	}

	sort_list(&format_parms);
	printf("Must format for this change to take effect\n");
}

/*
 * Delete a defect from the list.
 */
delete_from_list()
{
	int i, n, m;

/* users didn't like this...
	print_list();
*/
	i = pgetn("Enter defect number to be deleted: ");
	n = format_parms.list_len / 8;
	m = grown_defs.list_len / 8;
	if (i < 1 || i > m + n) {
		printf("Out of range.\n");
		return;
	}
	if (i > n) {
		i -= n;
		while (i < m) {
			grown_defs.defect[i - 1] = grown_defs.defect[i];
			i++;
		}
		grown_defs.list_len -= 8;
	} else {
		while (i < n) {
			format_parms.defect[i - 1] = format_parms.defect[i];
			i++;
		}
		format_parms.list_len -= 8;
	}
	printf("Must format for this change to take effect\n");
}

/*
 * Clear all defects from disk.
 */
clear_list()
{

	if (controller != C_ADAPTEC) {
		printf("Resetting defect lists to manufacturer's original\n");
		read_defect_list(RDEF_MANUF);
	} else
		format_parms.list_len = 0;
	grown_defs.list_len = 0;
	printf("Must format for this change to take effect\n");
}

/*
 * Surface analysis of disk for CCS.
 */
ccs_anal()
{
	struct scsi_scb scb;
	struct scsi_ext_sense sense7;
	int npass, nbad, max_lba, i, x, tmp_sense;
	
	nbad = 0;
	printf("SURFACE ANALYSIS\n");
	npass = getnpass(npassdata);
	max_lba =  (ncyl + acyl) * nhead * nsect - 1;
		/* this is a group one or 10 byte command descripter block */
	bzero((char *)&code1_cdb, sizeof code1_cdb);
	code1_cdb.cmd = SC_VERIFY;
	code1_cdb.lun = unit;
	code1_cdb.num_blks = 250;	/* verify 250 blocks at a time */
	for (i = 0; i < (max_lba - 250); i += 250) {
		if ((i % 1000) == 0)	/* print out every thousand checked */

			printf("\r analyzing blocks %d+", i);
		code1_cdb.lba = i;
		for (x = 0; x < npass; ++x) {
			code_1 = TRUE;
			if (scsi_doit(&code1_cdb, &scb, &sense7,
					(int)DBUF_PA1, 0) == 0) {
				code_1 = FALSE;
				if (errors || infomsgs) {
					pr_scb(&scb);
					pr_sense(&sense7);
				}
				if (tmp_sense = (sense7.info_1 << 24) |
				(sense7.info_2 << 16) | (sense7.info_3 << 8) |
					sense7.info_4) {
				printf("\nblock %d failed.\n", tmp_sense);
					/* reassign a block if it failed */
					reassign(tmp_sense, NO_QUERY);
					nbad += 1;
					/* verify left over (250-) blocks */
			nbad +=	verify_blks(tmp_sense += 1, i + 249 -tmp_sense);
				} else { 
					nbad +=	verify_blks(i, 250);
				}
				code1_cdb.num_blks = 250;
			}
			code_1 = FALSE;
		}
	}
	if (x = max_lba - i) {
		nbad += verify_blks(i, x);
	}
	code_1 = FALSE;
	printf("\nSurface analysis complete.\n");
	printf("%d bad sectors found.\n", nbad);
}

/*
 * analyze single blocks
 * another good candidate for cleanup
 */
verify_blks(start_blk, nblks)
{
	int nbad, a;
	struct scsi_scb scb;
	struct scsi_ext_sense sense7;
	
	nbad = 0;
	code1_cdb.num_blks = 1;
	for (a = 0; a < nblks; ++a) {
		code1_cdb.lba = start_blk + a;
		code_1 = TRUE;
		if (scsi_doit(&code1_cdb, &scb, &sense7, (int)DBUF_PA1, 0) == 0) {
			code_1 = FALSE;
			if (errors || infomsgs) {
				pr_scb(&scb);
				pr_sense(&sense7);
			}
			printf("\nblock %d failed.\n",  a + start_blk);
			reassign(a + start_blk, NO_QUERY);
			nbad++;
		}
	}
	code_1 == FALSE;
	return(nbad);
}
/*
 * Surface analysis of disk for Adaptec.
 */
surf_anal()
{
	int cyl, head, npass, nbad;

	nbad = 0;
	printf("SURFACE ANALYSIS - DESTROYS ALL DISK DATA!\n");
	printf("are you sure? ");
	if (!confirm())
		return;
	npass = getnpass(npassdata);

	for (cyl = 0; cyl < ncyl; cyl++) {
		printf("\rcyl %d ", cyl);
		for (head = 0; head < nhead; head++) {
			nbad += anal_track(cyl, head, npass);
		}
	}
	printf("Surface analysis complete.\n");
	if (nbad) {
		printf("%d bad sectors found.\n", nbad);
		printf("Use the 'f' command to format the disk.\n");
	}
}

anal_track(cyl, head, npass)
	int cyl, head;
	int npass;
{
	register int *ip, *dp, *dpe;
	register int data, pass, nbad, secnt;

	nbad = 0;
	dp = (int *)DBUF_VA;
	secnt = (head == nhead - 1) ? nsect - nspare : nsect;
	dpe = &dp[secnt * SECSIZE / sizeof (int)];
	for (pass = 0; pass < npass; pass++) {
		data = passdata[pass % npassdata];
		for (ip = dp; ip < dpe;)
			*ip++ = data;
		if (devcmd(WRITE, cyl, head, 0, secnt, DBUF_PA) ||
		    devcmd(READ, cyl, head, 0, secnt, DBUF_PA)) {
			nbad += anal_secs(cyl, head, npass);
			return (nbad);
		}
		for (ip = dp; ip < dpe;) {
			if (*ip++ != data) {
				printf("data readback error.\n");
				nbad += anal_secs(cyl, head, npass);
				return (nbad);
			}
		}
	}
	return (nbad);
}

/*
 * Analyze individual sectors of a track.
 */
anal_secs(cyl, head, npass)
	int cyl, head;
	int npass;
{
	register int *ip, *dp, *dpe;
	register int data, pass, nbad;
	register int sec, secnt;

	nbad = 0;
	dp = (int *)DBUF_VA;
	dpe = &dp[SECSIZE / sizeof (int)];
	secnt = (head == nhead - 1) ? nsect - nspare : nsect;
	for (sec = 0; sec < secnt; sec++) {
		for (pass = 0; pass < npass; pass++) {
			data = passdata[pass % npassdata];
			for (ip = dp; ip < dpe;)
				*ip++ = data;
			if (devcmd(WRITE, cyl, head, sec, 1, DBUF_PA) ||
			    devcmd(READ, cyl, head, sec, 1, DBUF_PA)) {
				bad_sec(cyl, head, sec, 1);
				nbad++;
				break;
			}
			for (ip = dp; ip < dpe;) {
				if (*ip++ != data) {
					printf("data readback error.\n");
					bad_sec(cyl, head, sec, 1);
					nbad++;
					/* need multi-level break here */
					if (sec++ == secnt) {
						return (nbad);
					}
					pass = -1;
					break;
				}
			}
		}
	}
	return (nbad);
}

/*
 * Parameters of Adaptec bytes from index formula.
 */
#define	FIRST	150
#define	INCR	(SECSIZE + 66)
#define	LAST	(FIRST + (nsect - 1) * INCR)

/*
 * We have found a bad sector.
 * Put it into the defect list.
 */
bad_sec(cyl, head, sec, add)
	int cyl, head, sec;
	int add;
{
	struct defect defect1, defect2;

	if (scsi_translate(cyl, head, sec)) {
		defect1 = *(struct defect *)DBUF_VA;
		printf("cyl %d head %d bfi %d (physical)\n", defect1.cyl,
			defect1.head, defect1.bytes_from_index);
		if (add)
			new_defect((struct defect *)DBUF_VA);
		return;
	}
	if (scsi_translate(cyl, head, sec-1) == 0) {
		printf("bad_sec: cannot translate before %d/%d/%d (logical)\n",
			cyl, head, sec);
		scsi_abort();
	}
	defect1 = *(struct defect *)DBUF_VA;
	if (scsi_translate(cyl, head, sec+1) == 0) {
		printf("bad_sec: cannot translate after %d/%d/%d (logical)\n",
			cyl, head, sec);
		scsi_abort();
	}
	defect2 = *(struct defect *)DBUF_VA;
	/*
	 * Add a sector to the first.
	 */
	if (defect1.bytes_from_index != LAST) {
		defect1.bytes_from_index += INCR;
	} else {
		defect1.bytes_from_index = FIRST;
		if (defect1.head != 0) {
			defect1.head --;
		} else {
			defect1.head = nhead - 1;
			defect1.cyl--;
		}
	}
	/*
	 * Subtract a sector from the second.
	 */
	if (defect2.bytes_from_index != FIRST) {
		defect2.bytes_from_index -= INCR;
	} else {
		defect2.bytes_from_index = LAST;
		if (defect2.head != 0) {
			defect2.head--;
		} else {
			defect2.head = nhead - 1;
			defect2.cyl--;
		}
	}
	/*
	 * If equal, we win.
	 */
	if (defect1.cyl == defect2.cyl && defect1.head == defect2.head &&
	    defect1.bytes_from_index == defect2.bytes_from_index) {
		printf("Interpolated cyl %d head %d bfi %d (physical)\n",
			defect1.cyl, defect1.head, defect1.bytes_from_index);
		if (add)
			new_defect(&defect1);
		return;
	} else {
		printf("defect interpolation failed\n");
		scsi_abort();
	}
}

/*
 * Add a new defect to the list.
 */
new_defect(dp)
	struct defect *dp;
{

	if ((controller == C_ADAPTEC &&
	     format_parms.list_len/8 >= (ST506_NDEFECT - 1)) ||
	    (format_parms.list_len + grown_defs.list_len)/8 >= ESDI_NDEFECT) {
		printf("TOO MANY DEFECTS!\n\nDISK UNUSABLE!\n");
		scsi_abort();
	}
	format_parms.defect[format_parms.list_len/8] = *dp;
	format_parms.list_len += 8;
	sort_list(&format_parms);
}

/*
 * Read defect list from disk.
 */
read_list()
{
	int sec;

	grown_defs.list_len = 0;
	for (sec = 0; sec < nsect; sec += 2) {
		if (devcmd(READ, ncyl, 0, sec, 2, DBUF_PA) == 0) {
			format_parms = *(struct format_parms *)DBUF_VA;
			if (format_parms.reserved != DEFECT_MAGIC) {
				continue;
			}
			format_parms.reserved = 0;
			return;
		}
	}
	printf("No defect list found.\n");
	format_parms.list_len = 0;
}

/*
 * Write defect list onto disk.
 */
write_list()
{
	int sec;

	format_parms.reserved = DEFECT_MAGIC;
	*(struct format_parms *)DBUF_VA = format_parms;
	for (sec = 0; sec < nsect; sec += 2)
		(void) devcmd(WRITE, ncyl, 0, sec, 2, DBUF_PA);
	format_parms.reserved = 0;
	scsi_putdef();
}

/*
 * Write the new defect list onto disk.
 */
scsi_putdef()
{
	int i, j;

	for (i = 0; i < (format_parms.list_len / 8); i++) {
		defect[i].cyl = format_parms.defect[i].cyl;
		defect[i].head = format_parms.defect[i].head;
		defect[i].sect = -1;
		defect[i].nbits = -1;
		defect[i].bfi = format_parms.defect[i].bytes_from_index;
	}
	for (j = 0; j < (grown_defs.list_len / 8); j++) {
		defect[i + j].cyl = grown_defs.defect[j].cyl;
		defect[i + j].head = grown_defs.defect[j].head;
		defect[i + j].sect = grown_defs.defect[j].grown_sect;
		defect[i + j].nbits = -1;
		defect[i + j].bfi = -1;
	}
	putdef(i + j);
}
 
/*
 * Reassign a spare sector
 */
reassign(lba, format)
	int lba, format;
{
	struct scsi_cdb cdb;
	struct scsi_scb scb;
	struct scsi_ext_sense sense7;
	register struct defect *def;
	int i, cyl, head, sec;

	if (controller == C_ADAPTEC) {
		printf("Not supported for this controller\n");
		return;
	}
	reassign_parms.reserved = 0;	/* set up header */	
	reassign_parms.list_len = 4;
	if (format == QUERY) {	/* ask user which block to reassign */
		lba = reassign_parms.new_defect[0].lba =
			pgetn("logical block address?	");
	} else {		/* else use the block number as sent */
		reassign_parms.new_defect[0].lba = lba;
	}
	scsi_silent++;
	cyl = lba / (nhead * nsect - nspare);
	sec = lba % (nhead * nsect - nspare);
	head = sec / nsect;
	sec %= nsect;
	for (i = 0; i < 10; i++) {
		if (SCcmd(READ, cyl, head, sec, 1, DBUF_PA) == 0)
			break;
	}
	scsi_silent = 0;
	*(struct reassign_parms*)DBUF_VA1 = reassign_parms;
	if (infomsgs) {
		printf("\treassign: ");
		for (i = 0; i < 8; ++i) {
			printf(" %x ", ((u_char *)DBUF_VA1)[i]);
		}
		printf("\n");
	}
	bzero((char *)&cdb, sizeof cdb);
	cdb.cmd = SC_REASSIGN_BLOCK;
	cdb.lun = unit;
	if (scsi_doit(&cdb, &scb, &sense7, (int)DBUF_PA1, 8) == 0) {
		printf("reassign block failed.\n");
		pr_scb(&scb);
		pr_sense(&sense7);
		return;
	}
	printf(" logical block %d, (0x%x) reassigned\n", lba, lba );
	(void) SCcmd(WRITE, cyl, head, sec, 1, DBUF_PA);
	if (format != NO_QADD) {
		def = &grown_defs.defect[grown_defs.list_len/8];
		def->cyl = cyl;
		def->head = head;
		def->grown_sect = sec;
		grown_defs.list_len += 8;
		sort_list(&grown_defs);
		scsi_putdef();
	}
}

/*
 * Find the defects on a CCS drive.
 */

read_ccs_list()
{
	int count, i, len;

	format_parms.list_len = 0;
	grown_defs.list_len = 0;
	count = rddeflab();
	if (count > 0) {
		for (i = 0; i < count; i++)
			if (defect[i].sect == -1) {
				len = format_parms.list_len / 8;
				format_parms.defect[len].cyl = defect[i].cyl;
				format_parms.defect[len].head = defect[i].head;
				format_parms.defect[len].bytes_from_index =
				    defect[i].bfi;
				format_parms.list_len += 8;
			} else {
				len = grown_defs.list_len / 8;
				grown_defs.defect[len].cyl = defect[i].cyl;
				grown_defs.defect[len].head = defect[i].head;
				grown_defs.defect[len].grown_sect =
				    defect[i].sect;
				grown_defs.list_len += 8;
			}
		read_defect_list(RDEF_CKLEN);
	} else {
		if (!esdi_virgin)
			printf("No defect list found.\n");
		read_defect_list(RDEF_ALL);
	}
}

/*
 * Read Defect List as per CCS definition.
 */
read_defect_list(type)
	int type;
{
	struct scsi_scb scb;
	struct scsi_ext_sense sense7;
	struct format_parms *fp;
	int i, x, num_defects, defect_length, total_defects;
 
	if (type != RDEF_CKLEN)
		printf("Extracting defects from controller...\n");
	total_defects = 0;
	bzero((char *)&code1_cdb, sizeof code1_cdb);
	code1_cdb.cmd = SC_READ_DEFECT_DATA;
	code1_cdb.lun = unit;
	code1_cdb.lba = 0x14000000;   /* physical manufacturer list first */
	code1_cdb.num_blks = 4;  /* get length of list */
	code_1 = TRUE;
	if (scsi_doit(&code1_cdb, &scb, &sense7, (int)DBUF_PA1, 4) == 0) {
		esdi_virgin = 1;
		format_parms.list_len = 0;
		printf("Disk appears unformatted\n");
		return;
	}
	code_1 = FALSE;
	if (((struct format_parms *)DBUF_VA1)->list_len == 0)
		esdi_corrupt = 1;
	if (type == RDEF_CKLEN)
		return;
	format_parms = *(struct format_parms *)DBUF_VA1;
	defect_length = format_parms.list_len + 4;
	total_defects += format_parms.list_len / 8;

	bzero((char *)&code1_cdb, sizeof code1_cdb);
	code1_cdb.cmd = SC_READ_DEFECT_DATA;
	code1_cdb.lun = unit;
	code1_cdb.lba = 0x14000000;	/* physical list */ 
			/* get the actual list this time */
	code1_cdb.reserved2 = ((defect_length & 0xff00) >> 8); 
	code1_cdb.num_blks = defect_length & 0x00ff; 
	code_1 = TRUE;
	if (scsi_doit(&code1_cdb, &scb, &sense7, (int)DBUF_PA1, defect_length) == 0) {
		printf("read defect list failed.\n");
		format_parms.list_len = 0;
		pr_scb(&scb);
		pr_sense(&sense7);
		return;
	}
	code_1 = FALSE;
	format_parms = *(struct format_parms *)DBUF_VA1;
	if (infomsgs) {
		defect_length = format_parms.list_len + 4;
		for (i = 0; i < defect_length; ++i)
			printf(" %x ", ((u_char *)DBUF_VA1)[i]);
		printf("\n");
	}
	if (type == RDEF_MANUF) {
		printf("%d defects.\n", total_defects);
		return;
	}

	bzero((char *)&code1_cdb, sizeof code1_cdb);
	code1_cdb.cmd = SC_READ_DEFECT_DATA;
	code1_cdb.lun = unit;
	code1_cdb.lba = 0x0c000000;	/* physical grown list */ 
	code1_cdb.num_blks = 4;  /* get length of list */
	code_1 = TRUE;
	if (scsi_doit(&code1_cdb, &scb, &sense7, (int)DBUF_PA1, 4) == 0) {
		printf("read defect list failed.\n");
		format_parms.list_len = 0;
		pr_scb(&scb);
		pr_sense(&sense7);
		return;
	}
	code_1 = FALSE;
	fp = (struct format_parms *)DBUF_VA1;
	defect_length = fp->list_len + 4;

	bzero((char *)&code1_cdb, sizeof code1_cdb);
	code1_cdb.cmd = SC_READ_DEFECT_DATA;
	code1_cdb.lun = unit;
	code1_cdb.lba = 0x0c000000;	/* physical grown list */ 
			/* get list this time */
	code1_cdb.reserved2 = ((defect_length & 0xff00) >> 8); 
	code1_cdb.num_blks = defect_length & 0x00ff; 
	code_1 = TRUE;
	if (scsi_doit(&code1_cdb, &scb, &sense7, (int)DBUF_PA1, defect_length) == 0) {
		printf("read defect list failed.\n");
		format_parms.list_len = 0;
		pr_scb(&scb);
		pr_sense(&sense7);
		return;
	}
	code_1 = FALSE;
	fp = (struct format_parms *)DBUF_VA1;
	if (infomsgs) {
		defect_length = fp->list_len + 4;
		for (i = 0; i < defect_length; ++i)
			printf(" %x ", ((u_char *)DBUF_VA1)[i]);
		printf("\n");
	}
		/* merge defects into printable list */
	total_defects += fp->list_len / 8;
	num_defects = fp->list_len / 8;
	for (i = 0; i < num_defects; ++i) {
		new_defect(&fp->defect[i]);
	}
	printf("%d defects.\n", total_defects);
	scsi_putdef();
}

/*
 * Comparison routine for defects.
 */
compar(d1, d2)
	struct defect *d1, *d2;
{

	if (d1->cyl < d2->cyl)
		return -1;
	if (d1->cyl > d2->cyl)
		return 1;
	if (d1->head < d2->head)
		return -1;
	if (d1->head > d2->head)
		return 1;
	if (d1->bytes_from_index < d2->bytes_from_index)
		return -1;
	if (d1->bytes_from_index > d2->bytes_from_index)
		return 1;
	return 0;
}

/*
 * Sort defect list.
 */
sort_list(list)
	register struct format_parms *list;
{
	register struct defect *dp, *dlast;
	register int dups;

	qsort((char *)&list->defect[0], list->list_len / 8,
		sizeof (struct defect), compar);
	if (list == &grown_defs)
		return;
	/*
	 * Now purge any duplicate entries
	 */
	do {
		dups = 0;
		dlast = &list->defect[list->list_len / 8] - 1;
		for (dp = &list->defect[0]; dp < dlast; dp++) {
			if (dp[0].cyl == dp[1].cyl &&
			    dp[0].head == dp[1].head &&
			    dp[0].bytes_from_index == dp[1].bytes_from_index) {
				dups++;
				printf("%s cyl %d head %d bfi %d ignored\n",
				    "duplicate entry", dp->cyl,
				    dp->head, dp->bytes_from_index);
				while (++dp < dlast)
					dp[0] = dp[1];
				list->list_len -= 8;
				break;
			}
		}
	} while (dups);
}

/*
 * Translate block number into cyl/hd/bfi.
 */
translate()
{
	int cyl, head, sec, b;

	if (controller != C_ADAPTEC) {
		printf("Not supported for this controller\n");
		return;
	}
	b = pgetbn("logical block number? ");
	cyl = b / (nhead * nsect - nspare);
	sec = b % (nhead * nsect - nspare);
	head = sec / nsect;
	sec %= nsect;
	bad_sec(cyl, head, sec, 0);
}

/*
 * Translate cyl, head, sector into cyl, head, bytes from index.
 * (This routine is probably incorrect for 256 byte sectors.)
 */
scsi_translate(cyl, head, sec)
	int head, sec;
{
	struct scsi_cdb cdb;
	struct scsi_scb scb;
	struct scsi_sense sense;
	int sector, n;

	bzero((char *)&cdb, sizeof cdb);
	cdb.cmd = SC_TRANSLATE;
	cdb.lun = unit;
	sector = ((cyl * (nhead * nsect - nspare)) +
	    (head * nsect) + sec) * mult;
	cdb.low_addr = sector & 0xff;
	sector >>= 8;
	cdb.mid_addr = sector & 0xff;
	sector >>= 8;
	cdb.high_addr = sector;
	n = 0;
	while (scsi_doit(&cdb, &scb, &sense, DBUF_PA, sizeof (struct defect)) 
		== 0) {
		if (n++ > 10) {
			printf("scsi translate failed!\n");
			return 0;
		}
	}
	return 1;
}

/*
 * Print out a command description block.
 */
pr_cdb(cdb)
	struct scsi_cdb *cdb;
{
	struct code1_cdb *cdb1;
	int i, cmd;
	char *cp;

	if (code_1 == TRUE) {
		cdb1 = (struct code1_cdb *)cdb;
		cp = (char *)cdb1;
		printf("\tcdb1:");
		for (i = 0; i < sizeof (*cdb1); i++)
			printf(" %x", cp[i] & 0xff);
		printf("\n");
	} else {
		cp = (char *)cdb;
		printf("\tcdb:");
		for (i = 0; i < sizeof (*cdb); i++)
			printf(" %x", cp[i] & 0xff);
		cmd = cdb->cmd;
		printf(" (%s)", SC_cmdlist[cmd]);
		printf("\n");
	}
}

/*
 * Print out status completion block.
 */
pr_scb(scbp)
struct scsi_scb *scbp;
{
	char *cp;

	cp = (char *)scbp;
	printf("scb: %x", cp[0]);
	if (scbp->ext_st1) {
		printf(" %x", cp[1]);
		if (scbp->ext_st2)
			printf(" %x", cp[2]);
	}
	if (scbp->is)
		printf(" intermediate status");
	if (scbp->busy)
		printf(" busy");
	if (scbp->cm)
		printf(" condition met");
	if (scbp->chk)
		printf(" check");
	if (scbp->ext_st1 && scbp->ha_er)
		printf(" host adapter detected error");
	printf("\n");
}

/*
 * Print out sense info.
 */
pr_sense(sense)
	struct scsi_sense *sense;
{
	struct scsi_ext_sense *sense7;
	int i;
	char sense_length, *cp;

	if (controller == C_ADAPTEC) {
		sense_length = 4;
		cp = (char *)sense;
	} else {
		sense_length = 13;
		sense7 = (struct scsi_ext_sense *)sense;
		cp = (char *)sense7;
	}
	printf("sense bytes: ");
	for (i = 0; i < sense_length; i++)
		printf("%x ", cp[i] & 0xff);
	printf("\n");
	if (sense->class <= 6) {
		printf("sense class %d code %x ", sense->class, sense->code);
		if (sense->adr_val)
			printf("block no %d\n", (sense->high_addr << 16) |
				(sense->mid_addr << 8) | sense->low_addr);
		else
			printf("adr_val=0.\n");
		if (sense->code < adap_SC_errct[sense->class])
			printf("\t%s\n",adap_SC_errors[sense->class][sense->code]);
	} else if (sense->class == 7) {
		/*
		 * Hack to extract old-style class and code from error_code.
		 */
		register u_char ercl = (sense7->error_code & 0x70) >> 4;
		register u_char ercd = sense7->error_code & 0x0f;

		printf("sense class 7 ");
		if (sense7->fil_mk)
			printf("file mark ");
		if (sense7->eom)
			printf("end of medium ");
		printf("\n");
		printf("\tsense key %x ", sense7->key);
		if (sense7->key < 
		    sizeof SC_sense7_keys / sizeof SC_sense7_keys[0])
			printf(" %s\n", SC_sense7_keys[sense7->key]);
		else
			printf(" (invalid key)\n");
		printf("\tblock no. %d\n", (sense7->info_1 << 24) |
			(sense7->info_2 << 16) | (sense7->info_3 << 8) |
			sense7->info_4);
		if (ercd < adap_SC_errct[ercl] && 
				controller == C_ADAPTEC) {
		printf("sense class %d code %x ", ercl, ercd);
			printf("\t%s\n",adap_SC_errors[ercl][ercd]); 
		} else
		if (ercd < ccs_SC_errct[ercl] && 
				controller == C_CCS_EMULEX) {
		printf("sense class %d code %x ", ercl, ercd);
			printf("\t%s\n", ccs_SC_errors[ercl][ercd]); 
		}
	} else {
		printf("Invalid sense class %d\n", sense->class);
	}
}

/*
 * Issue command to bus and get results.
 */
sc_doit(har, cdb, scb, sense, buf, len)
	struct scsi_ha_reg *har;
	struct scsi_cdb *cdb;
	struct scsi_scb *scb;
	struct scsi_sense *sense;
{
	struct code1_cdb *cdb1;
	char *cp;
	register int i, b, actual_len;

	while (har->icr & ICR_BUSY) {
		if (maygetchar() == ('c' & 037)) {
			if (infomsgs)
				printf("icr BUSY, waiting to select\n");
			scsi_abort();
		}
		for (i = 0; i < 30; i++)
			;
	}
	har->data = (1 << target) | SCSI_HOST_ADDR;
	har->icr = ICR_SELECT;
	sc_wait(har, ICR_BUSY);
	har->icr = ICR_WORD_MODE | ICR_DMA_ENABLE;
	har->dma_addr = buf & 0xFFFFF;
	har->dma_count = ~len;
	/* pass cdb to bus a byte at a time */
	if (code_1 == TRUE) {
		cdb1 = (struct code1_cdb *)cdb;
		if (infomsgs)
			pr_cdb(cdb1);
		cp = (char *)cdb1;
		for (i = 0; i < sizeof (*cdb1); i++)
			sc_putbyte(har, ICR_COMMAND, *cp++);
	} else {
		if (infomsgs)
			pr_cdb(cdb);
		cp = (char *)cdb;
		for (i = 0; i < sizeof (*cdb); i++)
			sc_putbyte(har, ICR_COMMAND, *cp++);
	}
	sc_wait(har, ICR_INTERRUPT_REQUEST);
	if (har->icr & ICR_ODD_LENGTH) {
		if (cdb->cmd == SC_READ
			|| cdb->cmd == SC_RECEIVE_DIAGNOSTIC
			|| cdb->cmd == SC_REQUEST_SENSE 
			|| cdb->cmd == SC_MODE_SENSE 
			|| cdb->cmd == SC_TRANSLATE 
			|| cdb->cmd == SC_READ_DEFECT_DATA 
			|| cdb->cmd == SC_READ_CAPACITY
			|| cdb->cmd == SC_INQUIRY) {
			cp = (char *)(DVMA + buf + len  - 1);
			*cp = (char)har->data;
			har->dma_count = ~(~har->dma_count - 1);
		} else {
			har->dma_count = ~(~har->dma_count + 1);
		}
	}
	cp = (char *)scb;
	i = 0;
	for (;;) {
		b = sc_getbyte(har, ICR_STATUS);
		if (b < 0)
			break;
		if (i < sizeof (struct scsi_scb))
			cp[i++] = b;
	}
	i = sc_getbyte(har, ICR_MESSAGE_IN);
	if (i != SC_COMMAND_COMPLETE) {
		printf("Invalid SCSI message: %x\n", i);
		scsi_abort();
	}
	if (scb->chk == 0) {
		actual_len = len - ~har->dma_count;
		if (actual_len != len) {
			int len_is_problem;	
	
			if (cdb->cmd == SC_REQUEST_SENSE) {
				if (actual_len >= 3)
					len_is_problem = 0;
				else
					len_is_problem = 1;
				if (len - actual_len == 7) {
					printf("was correct controller specified?\n");
				}
			}
			if (infomsgs || (errors && len_is_problem)) {
				printf("Incorrect dma count is %d should be %d ",
					actual_len, len);
				pr_cdb(cdb);
			}
			if (len_is_problem)
				scsi_abort();
		}
		if (scb->busy) {
			if (infomsgs)
				printf("scsi device busy\n");
			return 0;
		}
		return 1;
	} else {
		if (cdb->cmd == SC_REQUEST_SENSE) {
			printf("chk condition on sense: invalid.\n");
			scsi_abort();
		}
		if (infomsgs)
			printf("Command problem, getting sense.\n");
		if (controller == C_ADAPTEC)
		(void) SCexec(SC_REQUEST_SENSE, 0, 0, 0, 16, DBUF_PA, 0);
		else {		/* CCS sense length of 13 */
		code_1 = FALSE;
		(void) SCexec(SC_REQUEST_SENSE, 0, 0, 0, 13, DBUF_PA, 0);
		}
		*sense = *(struct scsi_sense *)DBUF_VA;
		return 0;
	}
}

/*
 * Put a byte into the scsi command register.
 */
sc_putbyte(har, bits, c)
	struct scsi_ha_reg *har;
{
	int icr;

	sc_wait(har, ICR_REQUEST);
	icr = har->icr;
	if ((icr & ICR_BITS) != bits) {
		printf("sc_putbyte error.\n");
		sc_pr_icr("icr is     ", icr);
		sc_pr_icr("waiting for", bits);
		scsi_abort();
	}
	har->cmd_stat = c;
	/* DELAY(10000);	/* for ADES */
}

/*
 * Get a byte from the scsi command/status register.
 */
sc_getbyte(har, bits)
	struct scsi_ha_reg *har;
{
	int icr;

	sc_wait(har, ICR_REQUEST);
	icr = har->icr;
	if ((icr & ICR_BITS) != bits) {
		if (bits == ICR_STATUS)
			return -1;	/* no more status */
		printf("sc_getbyte error.\n");
		sc_pr_icr("icr is     ", icr);
		sc_pr_icr("waiting for", bits & ICR_REQUEST);
		scsi_abort();
	}
	return (har->cmd_stat);
}

/*
* Wait for a condition on the scsi bus.
*/
sc_wait(har, cond)
	struct scsi_ha_reg *har;
{
	int i, icr;

	while (((icr = har->icr) & cond) != cond) {
		if (maygetchar() == ('c' & 037) || icr & ICR_BUS_ERROR) {
			if (infomsgs || errors && (icr & ICR_BUS_ERROR)) {
				printf("SCSI icr condition not met.\n");
				sc_pr_icr("icr is   ", (int)har->icr);
				sc_pr_icr("should be", cond);
				printf("data %x cmd %x dma_count(resid) %x\n",
					har->data, har->cmd_stat,
					~har->dma_count);
			}
			scsi_abort();
			/* If no abort, delay again. */
		}
		for (i = 0; i < 30; i++) /* delay a bit */
			;
	}
}

/*
 * Scsi reset.
 */
sc_reset(har)
	register struct scsi_ha_reg *har;
{
	har->icr = ICR_RESET;
	DELAY(50);
	har->icr = 0;
}

/*
 * Print out the scsi host adapter interface control register.
 */
sc_pr_icr(cp, i)
	char *cp;
{

	printf("\t%s: %x ", cp, i);
	if (i & ICR_PARITY_ERROR)
		printf("Parity err ");
	if (i & ICR_BUS_ERROR)
		printf("Bus err ");
	if (i & ICR_ODD_LENGTH)
		printf("Odd len ");
	if (i & ICR_INTERRUPT_REQUEST)
		printf("Int req ");
	if (i & ICR_REQUEST) {
		printf("Req ");
		switch (i & ICR_BITS) {
		case 0:
			printf("Data out ");
			break;
		case ICR_INPUT_OUTPUT:
			printf("Data in ");
			break;
		case ICR_COMMAND_DATA:
			printf("Command ");
			break;
		case ICR_COMMAND_DATA | ICR_INPUT_OUTPUT:
			printf("Status ");
			break;
		case ICR_MESSAGE | ICR_COMMAND_DATA:
			printf("Msg out ");
			break;
		case ICR_MESSAGE | ICR_COMMAND_DATA | ICR_INPUT_OUTPUT:
			printf("Msg in ");
			break;
		default:
			printf("DCM: %x ", i & ICR_BITS);
			break;
		}
	}
	if (i & ICR_PARITY)
		printf("Parity ");
	if (i & ICR_BUSY)
		printf("Busy ");
	if (i & ICR_SELECT)
		printf("Sel ");
	if (i & ICR_RESET)
		printf("Reset ");
	if (i & ICR_PARITY_ENABLE)
		printf("Par ena ");
	if (i & ICR_WORD_MODE)
		printf("Word mode ");
	if (i & ICR_DMA_ENABLE)
		printf("Dma ena ");
	if (i & ICR_INTERRUPT_ENABLE)
		printf("Int ena ");
	printf("\n");
}

/*
 * Issue command to bus and get results.
 */
si_doit(sir, cdb, scb, sense, buf, len)
	struct scsi_si_reg *sir;
	struct scsi_cdb *cdb;
	struct scsi_scb *scb;
	struct scsi_sense *sense;
{
	struct code1_cdb *cdb1;
	char *cp;
	register int i, b, actual_len;
	register struct udc_table *udct = UDCT_VA;
	int bufaddr;
	u_char junk;

	if (code_1 == TRUE) {
		cdb1 = (struct code1_cdb *)cdb;
	}
	/* select target */
	sir->sbc_wreg.odr = (1 << target) | SI_HOST_ID;
	sir->sbc_wreg.icr = SBC_ICR_DATA;
	sir->sbc_wreg.icr |= SBC_ICR_SEL;

	/* wait for target to acknowledge our selection */
	si_sbc_wait(&sir->sbc_rreg.cbsr, SBC_CBSR_BSY, 1);
	sir->sbc_wreg.icr = 0;

	/* do initial dma setup */
	sir->bcr = 0;
	if (len > 0) {
		if ((cdb->cmd == SC_READ)
			|| (cdb->cmd == SC_REQUEST_SENSE)
			|| (cdb->cmd == SC_MODE_SENSE)
			|| (cdb->cmd == SC_RECEIVE_DIAGNOSTIC) 
			|| (cdb->cmd == SC_READ_LONG)
			|| (cdb->cmd == SC_TRANSLATE) 
			|| (cdb->cmd == SC_READ_DEFECT_DATA) 
			|| (cdb->cmd == SC_READ_CAPACITY) 
			|| (cdb->cmd == SC_INQUIRY)) {
			sir->csr &= ~SI_CSR_SEND;
		} else {
			sir->csr |= SI_CSR_SEND;
		}
		sir->csr &= ~SI_CSR_FIFO_RES;
		sir->csr |= SI_CSR_FIFO_RES;
		sir->bcr = len;
		if (onboard == 0)
			sir->bcrh = 0;
	}

	/* put command onto scsi bus */
	if (code_1 == TRUE) {
		if (infomsgs) {
			pr_cdb(cdb1);
		}
		cp = (char *)cdb1;
		if (si_putbyte(sir, PHASE_COMMAND, cp,
				sizeof(struct code1_cdb)) == 0) {
			printf("SCSI could not put command on scsi bus\n");
			scsi_abort();
		}
	} else {
		if (infomsgs) {
			pr_cdb(cdb);
		}
		cp = (char *)cdb;
		if (si_putbyte(sir, PHASE_COMMAND, cp,
				sizeof(struct scsi_cdb)) == 0) {
			printf("SCSI could not put command on scsi bus\n");
			scsi_abort();
		}
	}

	/* finish dma setup and wait for dma completion */
	if (len > 0) {
		if (onboard) {
			/* setup udc dma info */
			bufaddr = buf + SI_OB_DVMA_OFFSET;
			udct->haddr = ((bufaddr & 0xff0000) >> 8) | 
			    UDC_ADDR_INFO;
			udct->laddr = bufaddr & 0xffff;
			udct->hcmr = UDC_CMR_HIGH;
			udct->count = len / 2;
			if ((cdb->cmd == SC_READ)
				|| (cdb->cmd == SC_REQUEST_SENSE)
				|| (cdb->cmd == SC_MODE_SENSE)
				|| (cdb->cmd == SC_RECEIVE_DIAGNOSTIC)
				|| (cdb->cmd == SC_READ_LONG)
				|| (cdb->cmd == SC_TRANSLATE) 
				|| (cdb->cmd == SC_READ_DEFECT_DATA) 
				|| (cdb->cmd == SC_READ_CAPACITY) 
				|| (cdb->cmd == SC_INQUIRY)) {
				udct->rsel = UDC_RSEL_RECV;
				udct->lcmr = UDC_CMR_LRECV;
			} else {
				udct->rsel = UDC_RSEL_SEND;
				udct->lcmr = UDC_CMR_LSEND;
				if (len & 1) {
					udct->count++;
				}
			}

			/* initialize chain address register */
			DELAY(SI_UDC_WAIT);
			sir->udc_raddr = UDC_ADR_CAR_HIGH;
			DELAY(SI_UDC_WAIT);
			sir->udc_rdata = ((int)udct & 0xff0000) >> 8;
			DELAY(SI_UDC_WAIT);
			sir->udc_raddr = UDC_ADR_CAR_LOW;
			DELAY(SI_UDC_WAIT);
			sir->udc_rdata = (int)udct & 0xffff;

			/* initialize master mode register */
			DELAY(SI_UDC_WAIT);
			sir->udc_raddr = UDC_ADR_MODE;
			DELAY(SI_UDC_WAIT);
			sir->udc_rdata = UDC_MODE;

			/* issue start chain command */
			DELAY(SI_UDC_WAIT);
			sir->udc_raddr = UDC_ADR_COMMAND;
			DELAY(SI_UDC_WAIT);
			sir->udc_rdata = UDC_CMD_STRT_CHN;
		} else {
			if ((int)buf & 2) {
				sir->csr |= SI_CSR_BPCON;
			} else {
				sir->csr &= ~SI_CSR_BPCON;
			}
			sir->iv_am = VME_SUPV_DATA_24;
			sir->dma_addr = buf & 0xfffff;
			sir->dma_count = len;
		}

		/* setup sbc and start dma */
		sir->sbc_wreg.mr |= SBC_MR_DMA;
		if ((cdb->cmd == SC_READ)
			|| (cdb->cmd == SC_REQUEST_SENSE)
			|| (cdb->cmd == SC_MODE_SENSE)
			|| (cdb->cmd == SC_RECEIVE_DIAGNOSTIC)
			|| (cdb->cmd == SC_READ_LONG)
			|| (cdb->cmd == SC_TRANSLATE) 
			|| (cdb->cmd == SC_READ_DEFECT_DATA) 
			|| (cdb->cmd == SC_READ_CAPACITY) 
			|| (cdb->cmd == SC_INQUIRY)) {
			sir->sbc_wreg.tcr = TCR_DATA_IN;
			sir->sbc_wreg.ircv = 0;
		} else {
			sir->sbc_wreg.tcr = TCR_DATA_OUT;
			sir->sbc_wreg.icr = SBC_ICR_DATA;
			sir->sbc_wreg.send = 0;
		}
		if (onboard == 0)
			sir->csr |= SI_CSR_DMA_EN;

		/* wait for dma to complete */
		si_wait(&sir->csr, 
		    SI_CSR_SBC_IP|SI_CSR_DMA_IP|SI_CSR_DMA_CONFLICT, 1);
		if (onboard == 0)
			sir->csr &= ~SI_CSR_DMA_EN;

		/* check reason for dma completion */
		if (sir->csr & SI_CSR_SBC_IP) {
			/* dma operation should end with a phase mismatch */
			si_sbc_wait(&sir->sbc_rreg.bsr, SBC_BSR_PMTCH, 0);
		} else {
			printf("SCSI dma error: ");
			if (sir->csr & SI_CSR_DMA_CONFLICT) {
				printf("invalid reg accessed during dma\n");
			} else if (sir->csr & SI_CSR_DMA_BUS_ERR) {
				printf("bus error\n");
			} else {
				if (onboard)
					printf("unknown dma failure\n");
				else
					printf("dma overrun\n");
			}
			si_dma_cleanup(sir);
			scsi_abort();
		}

		/* handle special dma recv situations */
		if ((cdb->cmd == SC_READ)
			|| (cdb->cmd == SC_REQUEST_SENSE)
			|| (cdb->cmd == SC_MODE_SENSE)
			|| (cdb->cmd == SC_RECEIVE_DIAGNOSTIC)
			|| (cdb->cmd == SC_TRANSLATE) 
			|| (cdb->cmd == SC_READ_DEFECT_DATA) 
			|| (cdb->cmd == SC_READ_CAPACITY) 
			|| (cdb->cmd == SC_INQUIRY)) {
		    if (onboard) {
			sir->udc_raddr = UDC_ADR_COUNT;
			si_wait(&sir->csr, SI_CSR_FIFO_EMPTY, 1);

			/* if odd byte recv, must grab last byte by hand */
			if ((len - sir->bcr) & 1) {
				cp = (char *)(DVMA + buf + (len - sir->bcr) - 1);
				*cp = (sir->fifo_data & 0xff00) >> 8;
				if (infomsgs)
					printf("\todd length byte = %x\n", *cp);

			/* udc may not dma last word */
			} else if (((sir->udc_rdata*2) - sir->bcr) == 2) {
				cp = (char *)(DVMA + buf + (len - sir->bcr));
				*(cp - 2) = (sir->fifo_data & 0xff00) >> 8;
				*(cp - 1) = sir->fifo_data & 0x00ff;
				if (infomsgs)
					printf("\todd length byte = %x\n", *cp);
			}
		    } else if ((sir->csr & SI_CSR_LOB) != 0) {
			cp = (char *)(DVMA + buf + (len - sir->bcr));
			if ((sir->csr & SI_CSR_BPCON) == 0) {
			    switch (sir->csr & SI_CSR_LOB) {
			    case SI_CSR_LOB_THREE:
				    *(cp - 3) = (sir->bpr & 0xff000000) >> 24;
				    *(cp - 2) = (sir->bpr & 0x00ff0000) >> 16;
				    *(cp - 1) = (sir->bpr & 0x0000ff00) >> 8;
				    break;
			    case SI_CSR_LOB_TWO:
				    *(cp - 2) = (sir->bpr & 0xff000000) >> 24;
				    *(cp - 1) = (sir->bpr & 0x00ff0000) >> 16;
				    break;
			    case SI_CSR_LOB_ONE:
				    *(cp - 1) = (sir->bpr & 0xff000000) >> 24;
				    break;
			    }
			} else {
				*(cp - 1) = (sir->bpr & 0x0000ff00) >> 8;
			}
			if (infomsgs)
				printf("\todd length byte = %x\n", *cp);
		    }
		}

		/* clear sbc interrupt */
		junk = sir->sbc_rreg.clr;

		/* cleanup after a dma operation */
		si_dma_cleanup(sir);
	}

	/* get status */
	cp = (char *)scb;
	for (i = 0;;) {
		b = si_getbyte(sir, PHASE_STATUS);
		if (b < 0) {
			break;
		}
		if (i < STATUS_LEN) {
			cp[i++] = b;
		}
	}
	b = si_getbyte(sir, PHASE_MSG_IN);
	if (b != SC_COMMAND_COMPLETE) {
		printf("Invalid SCSI message: %x\n", b);
		scsi_abort();
	}
	if (scb->chk == 0) {
		actual_len = len - sir->bcr;
		if (actual_len != len) {
			int len_is_problem;	
	
			if (cdb->cmd == SC_REQUEST_SENSE) {
				if (actual_len >= 3)
					len_is_problem = 0;
				else
					len_is_problem = 1;
				if (len - actual_len == 7) {
					printf("was correct controller specified?\n");
				}
			}
			if (infomsgs || (errors && len_is_problem)) {
				printf("Incorrect dma count is %d should be %d ",
					actual_len, len);
				pr_cdb(cdb);
			}
			if (len_is_problem)
				scsi_abort();
		}
		if (scb->busy) {
			if (infomsgs)
				printf("scsi device busy\n");
			return 0;
		}
		return 1;
	} else {
		if (cdb->cmd == SC_REQUEST_SENSE) {
			printf("chk condition on sense: invalid.\n");
			scsi_abort();
		}
		if (infomsgs)
			printf("Command completion problem, getting sense.\n");
		if (controller == C_ADAPTEC)
		(void) SCexec(SC_REQUEST_SENSE, 0, 0, 0, 16, DBUF_PA, 0);
		else {		/* CCS sense length is 13 */
		code_1 = FALSE;
		(void) SCexec(SC_REQUEST_SENSE, 0, 0, 0, 13, DBUF_PA, 0);
		}
		*sense = *(struct scsi_sense *)DBUF_VA;
		return 0;
	}
}

/*
 * Reset some register information after a dma operation.
 */
si_dma_cleanup(sir)
	register struct scsi_si_reg *sir;
{
	if (onboard) {
		sir->udc_raddr = UDC_ADR_COMMAND;
		DELAY(SI_UDC_WAIT);
		sir->udc_rdata = UDC_CMD_RESET;
	} else {
		sir->csr &= ~SI_CSR_DMA_EN;
		sir->dma_addr = 0;
		sir->dma_count = 0;
	}
	sir->sbc_wreg.mr &= ~SBC_MR_DMA;
	sir->sbc_wreg.icr = 0;
	sir->sbc_wreg.tcr = 0;
}

/*
 * Wait for a condition(s) to be (de)asserted in the csr.
 */
si_wait(reg, cond, set)
	register u_short *reg;
	register u_short cond;
	register int set;
{
	register int i;
	register u_short regval;

	while (1) {
		regval = *reg;
		if ((set == 1) && (regval & cond)) {
			/*
			 * how do the following delays prevent the 3/50 from
			 * hanging on certain commands?
			 */
			DELAY(50);
			return (1);
		}
		if ((set == 0) && !(regval & cond)) {
			DELAY(50);
			return (1);
		} 
		if (maygetchar() == ('c' & 037)) {
			if (infomsgs || errors) {
				if (set)
					printf("SCSI csr cond not set.\n");
				else
					printf("SCSI csr cond not UNset.\n");
				si_pr_csr("csr is ", regval);
				si_pr_csr("waiting for ", cond);
			}
			scsi_abort();
		}
		for (i = 0; i < 30; i ++)	/* delay a bit */
			;
	}
}

/*
 * Wait for a condition(s) to be (de)asserted on the scsi bus.
 */
si_sbc_wait(reg, cond, set)
	register caddr_t reg;
	register u_char cond;
	register int set;
{
	register int i;
	register u_char regval;

	while (1) {
		regval = *reg;
		if ((set == 1) && (regval & cond)) {
			/*
			 * how do the following delays prevent the 3/50 from
			 * hanging on certain commands?
			 */
			DELAY(50);
			return (1);
		}
		if ((set == 0) && !(regval & cond)) {
			DELAY(50);
			return (1);
		} 
		if (maygetchar() == ('c' & 037)) {
			if (infomsgs || errors) {
				if (set)
					printf("SCSI sbc cond not set.\n");
				else
					printf("SCSI sbc cond not UNset.\n");
				if ((int)reg & 1) {
					si_pr_sbc_bsr("sbc bsr is ", regval);
					si_pr_sbc_bsr("waiting for ", cond);
				} else {
					si_pr_sbc_cbsr("sbc cbsr is ", regval);
					si_pr_sbc_cbsr("waiting for ", cond);
				}
			}
			scsi_abort();
		}
		for (i = 0; i < 30; i ++)	/* delay a bit */
			;
	}
}

/*
 * Put a byte onto the scsi bus.
 */
si_putbyte(sir, phase, data, numbytes)
	register struct scsi_si_reg *sir;
	register u_short phase;
	register u_char *data;
	register int numbytes;
{
	register int i;

	/* set up tcr so a phase match will occur */
	if (phase == PHASE_COMMAND) {
		sir->sbc_wreg.tcr = TCR_COMMAND;
	} else if (phase == PHASE_MSG_OUT) {
		sir->sbc_wreg.tcr = TCR_MSG_OUT;
	} else {
		if (infomsgs)
			printf("si_putbyte, bad phase specified\n");
		return (0);
	}

	/* put all desired bytes onto scsi bus */
	for (i = 0; i < numbytes; i++) {
		/* wait for target to request a byte */
		si_sbc_wait(&sir->sbc_rreg.cbsr, SBC_CBSR_REQ, 1);

		/* load data for transfer */
		sir->sbc_wreg.odr = *data++;
		sir->sbc_wreg.icr = SBC_ICR_DATA;

		/* make sure phase match occurred */
		if ((sir->sbc_rreg.bsr & SBC_BSR_PMTCH) == 0) {
			if (infomsgs)
				printf("si_putbyte: phase mismatch\n");
			return (0);
		}

		/* complete req/ack handshake */
		sir->sbc_wreg.icr |= SBC_ICR_ACK;
		si_sbc_wait(&sir->sbc_rreg.cbsr, SBC_CBSR_REQ, 0);
		sir->sbc_wreg.icr = 0;
	}
	sir->sbc_wreg.tcr = 0;
	return (1);
}

/*
 * Get a byte from the scsi bus.
 */
si_getbyte(sir, phase)
	register struct scsi_si_reg *sir;
	register u_short phase;
{
	register u_char data;

	/* set up tcr so a phase match will occur */
	if (phase == PHASE_STATUS) {
		sir->sbc_wreg.tcr = TCR_STATUS;
	} else if (phase == PHASE_MSG_IN) {
		sir->sbc_wreg.tcr = TCR_MSG_IN;
	} else {
		if (infomsgs)
			printf("si_getbyte, bad phase specified\n");
		return (-1);
	}

	/* wait for target request */
	si_sbc_wait(&sir->sbc_rreg.cbsr, SBC_CBSR_REQ, 1);

	/* check for correct information phase on scsi bus */
	if (phase != (sir->sbc_rreg.cbsr & CBSR_PHASE_BITS)) {
		if (phase != PHASE_STATUS) {
			if (infomsgs)
				printf("si_getbyte: phase mismatch\n");
		} else {
			sir->sbc_wreg.tcr = 0;
		}
		return (-1);
	}

	/* grab data */
	data = sir->sbc_rreg.cdr;
	sir->sbc_wreg.icr = SBC_ICR_ACK;

	/* complete req/ack handshake */
	si_sbc_wait(&sir->sbc_rreg.cbsr, SBC_CBSR_REQ, 0);
	sir->sbc_wreg.icr = 0;
	sir->sbc_wreg.tcr = 0;
	return (data);
}

/*
 * Reset SCSI control logic.
 */
si_reset(sir)
	register struct scsi_si_reg *sir;
{
	register u_char junk;

	/* reset bcr, fifo, udc, and sbc */
	sir->bcr = 0;
	sir->csr = 0;
	DELAY(10);
	sir->csr = SI_CSR_SCSI_RES|SI_CSR_FIFO_RES;
	if (onboard == 0) {
		sir->dma_addr = 0;
		sir->dma_count = 0;
	}

	/* issue scsi bus reset */
	sir->sbc_wreg.icr = SBC_ICR_RST;
	DELAY(50);
	sir->sbc_wreg.icr = 0;
	junk = sir->sbc_rreg.clr;
}

/*
 * Print out the bus and status register for the SBC on the Sun3/50.
 */
si_pr_sbc_bsr(cp, i)
	char *cp;
	u_char i;
{
	printf("\t%s: %x ", cp, i);
	if (i & SBC_BSR_EDMA)
		printf("End of Dma, ");
	if (i & SBC_BSR_RDMA)
		printf("Dma Request, ");
	if (i & SBC_BSR_PERR)
		printf("Parity Error, ");
	if (i & SBC_BSR_INTR)
		printf("Interrupt Pending, ");
	if (i & SBC_BSR_PMTCH)
		printf("Phase Match, ");
	if ((i & SBC_BSR_PMTCH) == 0)
		printf("Phase Mismatch, ");
	if (i & SBC_BSR_BERR)
		printf("Bus error, ");
	if (i & SBC_BSR_ATN)
		printf("Attention, ");
	if (i & SBC_BSR_ACK)
		printf("Acknowledge, ");
	printf("\n");
}

/*
 * Print out the current bus and status register for the SBC on the Sun3/50.
 */
si_pr_sbc_cbsr(cp, i)
	char *cp;
	u_char i;
{
	printf("\t%s: %x ", cp, i);
	if (i & SBC_CBSR_RST)
		printf("Reset, ");
	if (i & SBC_CBSR_BSY)
		printf("Busy, ");
	if (i & SBC_CBSR_REQ)
		printf("Request, ");
	if (i & SBC_CBSR_MSG)
		printf("Message, ");
	if (i & SBC_CBSR_CD)
		printf("Command/Data, ");
	if (i & SBC_CBSR_IO)
		printf("Input/Output, ");
	if (i & SBC_CBSR_SEL)
		printf("Select, ");
	printf("\n");
}

/*
 * Print out the control and status register for the Sun3/50.
 */
si_pr_csr(cp, i)
	char *cp;
	u_short i;
{
	printf("\t%s: %x ", cp, i);
	if ((onboard) && (i & SI_CSR_DMA_ACTIVE))
		printf("Dma active, ");
	if (i & SI_CSR_FIFO_FULL)
		printf("Fifo full, ");
	if (i & SI_CSR_FIFO_EMPTY)
		printf("Fifo empty, ");
	if (i & SI_CSR_SBC_IP)
		printf("Sbc intr, ");
	if (i & SI_CSR_DMA_IP) {
		if (i & SI_CSR_DMA_BUS_ERR)
			printf("Dma bus error, ");
		else if (i & SI_CSR_DMA_CONFLICT)
			printf("Dma invalid reg access, ");
		else if (onboard == 0)
			printf("Dma overrun, ");
		else
			printf("Dma unknown error, ");
	}
	if (onboard == 0) {
		if (i & SI_CSR_LOB)
			printf("Dma bytes leftover, ");
		if (i & SI_CSR_BPCON)
			printf("Dma words, ");
		else
			printf("Dma longwords, ");
		if (i & SI_CSR_DMA_EN)
			printf("Dma enabled, ");
	}
	if (i & SI_CSR_SEND)
		printf("Dma to dev, ");
	else
		printf("Dma from dev, ");
	if (i & SI_CSR_INTR_EN)
		printf("Intr enabled, ");
	if ((i & SI_CSR_FIFO_RES) == 0)
		printf("Fifo in reset, ");
	if ((i & SI_CSR_SCSI_RES) == 0)
		printf("Scsi in reset, ");
	printf("\n");
}
