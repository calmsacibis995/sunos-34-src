/*
 * @(#)sm_statd.h 1.1 86/09/24
 */

struct stat_chge {
	char *name;
	int state;
};
typedef struct stat_chge stat_chge;

#define SM_NOTIFY 6



