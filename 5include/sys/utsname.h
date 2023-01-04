/*	@(#)utsname.h 1.1 86/09/24 SMI; from S5R2 6.1	*/

struct utsname {
	char	sysname[9];
	char	nodename[9];
	char	release[9];
	char	version[9];
	char	machine[9];
};
extern struct utsname utsname;
