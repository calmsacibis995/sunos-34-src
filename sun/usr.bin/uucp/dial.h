/*	dial.h	1.1	86/09/25	*/

extern int alarmtr();
extern int Dnf;

#define MAXPH 60

	/* This structure tells about a device */
struct Devices {
	char D_type[32];
	char D_line[32];
	char D_calldev[32];
	char D_class[32];
	int D_speed;
};
