
#ifndef lint
static	char sccsid[] = "@(#)setup_message.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "setup_runtime.h"

char	*setup_message_table[] = {

	/* SETUP_EOK (0) */
	"",

	/* SETUP_EBADNAME (1) */
	"Illegal workstation name \"%s\".",

	/* SETUP_EBADEADDR (2) */
	"Illegal Ethernet address \"%s\".",			

	/* SETUP_EBADDOMAIN (3) */
	"Illegal domain name.",					

	/* SETUP_EBADNETWORK (4) */
	"For class %c networks the high order byte must be between %d and %d.",

	/* SETUP_EBADSELECT (5) */
	"Bad selection number.",				

	/* SETUP_EBADHOSTNUMBER (6) */
	"Bad host number.",					

	/* SETUP_EBADTSHOSTNUMBER (7) */
	"Bad tape server host number.",				

	/* SETUP_EHARDPARTTOBIG (8) */
	"Maximum size for hard partition %s is %s.",		

	/* SETUP_EHARDPARTNEG (9) */
	"Hard partition sizes cannot be negative.",		

	/* SETUP_EHARDPARTOVERLAP (10) */
	"Overlapping of hard partitions not allowed.",		

	/* SETUP_ENOAUTOHOST (11) */
	"Automatic allocation of host numbers is not enabled.",	

	/* SETUP_ENOMOREHOSTS (12) */
	"No more host numbers are available to allocate.",	

	/* SETUP_EHOSTOUTOFRANGE (13) */
	"For class %c networks host numbers must be between %d and %d.",

	/* SETUP_EHOSTNUMBERUSED (14) */
	"Host number %d is already in use.",			

	/* SETUP_ENONETWORK (15) */
	"Host numbers cannot be assigned before a network number.", 

	/* SETUP_EDUPNAME (16) */
	"Workstation name \"%s\" is already in use.",

	/* SETUP_ENULLNAME (17) */
	"Blank workstation names are not allowed.",

	/* SETUP_ENULLNETWORK (18) */
	"Blank network numbers are not allowed.",

	/* SETUP_ENOSERVERFORWS (19) */
	"Servers must be configured to serve their own architectures.",

	/* SETUP_ERESHARDPART (20) */
	"Hard partition %s type cannot be directly changed.",

	/* SETUP_ENOFREESPACE (21) */
	"Not enough free space on %s to make partition %s larger.",

	/* SETUP_ENOFLOAT (22) */
	"Cannot \"float\" %s because it has overlapping partitions.",

	/* SETUP_ENOTSERVER (23) */
	"Clients cannot be allocated unless you are a file server.",

	/* SETUP_ENORESMIN (24) */
	"The minimum size for the %s partition is %s.",

	/* SETUP_ENTIREDISK (25) */
	"Partition `c' is the entire disk and you cannot directly change it.",

	/* SETUP_EFREEHOG (26) */
	"Partition %s is the free hog and you cannot directly change it.",

	/* SETUP_EFLOATING (27) */
	"You cannot directly change offsets when the disk is \"floating\".",

	/* SETUP_EROOTCHANGED (28) */
	"You cannot change the offset or size of the root partition.",

	/* SETUP_ECHANGEND (29) */
	"Not enough space available to change the client's ND partition.",

	/* SETUP_EBADENET (30) */
	"Unable to detect enthernet interface \"%s\".",

	/* SETUP_ECHECKBAD (31) */
	"The following errors must be corrected:",

	/* SETUP_EWS_NONE (32) */
	"    The workstation type cannot be `None'.",

	/* SETUP_EWS_NONAME (33) */
	"    The workstation must be assigned a name.",

	/* SETUP_EWS_NOHOSTNUM (34) */
	"    The workstation must be assigned a host number.",

	/* SETUP_EWS_NOETHERNET (35) */
	"    An ethernet board must be selected.",

	/* SETUP_EWS_NOYPSERVER_NAME (36) */
	"    The yellow pages master server's name must be specified.",

	/* SETUP_EWS_NOYPSERVER_INTERNET (37) */
	"    The yellow pages master server's internet address be specified.",

	/* SETUP_ECLIENT_NOHOSTNUM (38) */
	"    Client `%s' must be assigned a host number.",

	/* SETUP_ECLIENT_NOETHERNET (39) */
	"    Client `%s' must be assigned an ethernet address.",

	/* SETUP_ECLIENT_NOROOT (40) */
	"    Client `%s' must be given a root partition.",

	/* SETUP_ECLIENT_NOROOTSIZE (41) */
	"    Client `%s's root partition cannot be zero size.",

	/* SETUP_ECLIENT_NOSWAP (42) */
	"    Client `%s' must be given a swap partition.",

	/* SETUP_ECLIENT_NOSWAPSIZE (43) */
	"    Client `%s's swap partition cannot be zero size.",

	/* SETUP_ENOTCHANGEABLE (44) */
	"Item is read-only.",

	/* SETUP_ENO_OSWG (45) */
	"Could not find software category `%s'.",

	/* SETUP_EOPEN_FAILED (46) */
	"Could not open file `%s' for %s.",

	/* SETUP_ENO_TOC (47) */
	"Bad tape - no table-of-contents.",

	/* SETUP_EBAD_TOC_NOHDR (48) */
	"Bad table-of-contents - no header.",

	/* SETUP_EBAD_TOC_NOARCH (49) */
	"Bad table-of-contents - no architecture.",

	/* SETUP_ENO_TAPE (50) */
	"Tape is not mounted, or drive is not online.",

	/* SETUP_EWRONG_ARCH (51) */
	"Wrong tape mounted - expected architecture `%s', but got `%s'.",

	/* SETUP_EWRONG_TAPENUM (52) */
	"Wrong tape mounted - expected tape number %d, but got %d.",

	/* SETUP_EREAD_LABEL (53) */
	"Could not read the label on device `%s`.",

	/* SETUP_EWRITE_LABEL (54) */
	"Could not read the label on device `%s`.",

	/* SETUP_EINSTALL_ABORT (55) */
	"Fatal error - installation aborted.",

	/* SETUP_ENO_SERVERARCH (56) */
	"Could not find the server's architecture.",

	/* SETUP_EARCHNOTSERVED (57) */
	"Server is not currently configured to serve %s.",

	/* SETUP_ENOSPACEFORND (58) */
	"Not enough space on any ND partitions to fit client's partition.",

	/* SETUP_EFREEHOGONFLT (59) */
	"Floating on %s cannot be turned off because %s is a free hog.",

	/* SETUP_ERESERVEONFLT (60) */
	"Floating on %s cannot be turned off because %s is reserved.",

	/* SETUP_EMNTPOINT (61) */
	"Mount point cannot be assigned to %s if it is not a UNIX partition.",

	/* SETUP_EMOVENOTFREE (62) */
	"Cannot move %s to %s because %s is not FREE.",

	/* SETUP_EMOVENOTFLOAT (63) */
	"Cannot move %s to %s because %s cannot be made a free hog.",

	/* SETUP_EMOVENOSPACE (64) */
	"Cannot move %s to %s because %s is not large enough.",

	/* SETUP_ENOHOGWOFLOAT (65) */
	"Cannot assign a free hog on disk %s because it is not floating.",

	/* SETUP_ENOFLOATWOVER (66) */
	"Cannot float disk %s because overlapped partitions are allowed.",

	/* SETUP_ENOOVERWFLOAT (67) */
	"Cannot allow overlapped partitions on disk %s because it is floating.",

	/* SETUP_ECANOTHOGRES (68) */
	"Partition %s is reserved and cannot become the free hog.",

	/* SETUP_ESERVERWCLNT (69) */
	"Your workstation must be a \"File Server\" while any clients exist.",

	/* SETUP_EFULLMNTPOINT (70) */
	"Mount points must be fully qualified pathnames.",

	/* SETUP_EOSWGDONTFIT (71) */
	"Not enough space on partition %s for optional software \"%s\".",

	/* SETUP_ENOETHER (72) */
	"You must first select an ethernet board.",

	/* SETUP_ENOWSTYPE (73) */
	"You must first select a workstation type.",

	/* SETUP_ENOCLIENT (74) */
	"Client \"%s\" does not exist; cannot apply card \"%s\".",

	/* SETUP_EDUPNAME (75) */
	"Card name \"%s\" is already in use.",

	/* SETUP_EBADTAPEDEV (76) */
	"    Could not make contact with tape `%s'.",

	/* SETUP_EARCHHASCLIENTS (77) */
	"Architecture \"%s\" is used to serve clients and cannot be removed.",

	/* SETUP_EDUPEADDR (78) */
	"Ethernet address \"%s\" is already in use by client \"%s\".",

	/* SETUP_ENEEDETHERNET (79) */
	"Ethernet type cannot be \"NONE\" while serving clients.",

	/* SETUP_ENOCARD (80) */
	"Cannot set default card to \"%s\" because the card does not exist.",

	/* SETUP_ECHANGENDPART (81) */
	"Cannot directly change partition \"%s\" when it's used for ND.",

	/* SETUP_ENDPARTFREEHOG (82) */
	"Cannot make partition \"%s\" the free hog when it's used for ND.",

	/* SETUP_EETHERADDR (83) */
	"Could not get the ethernet address - %s",

	/* SETUP_ECANOTMOVEND (84) */
	"Cannot move ND partition to %s because %s can't be made large enough.",

	/* SETUP_EWS_NODOMAIN (85) */
	"    The workstation must be assigned a yellow pages domain name.",

	/* SETUP_BEGINSTALL (86) */
	"Beginning the installation.",

	/* SETUP_NOT_SUPERUSER (85) */
	"You must be super-user to actually install anything.",
};


/* SETUP_VARARGS */
runtime_message(errno, arg2, arg3, arg4, arg5, arg6, arg7)
Setup_errno	errno;
{
	sprintf(setup_msgbuf, setup_message_table[ord(errno)], 
		arg2, arg3, arg4, arg5, arg6, arg7);
}
