/*
 * Include file Sun-2/120 Video Board Diagnostic source code.
 *
 * @(#)video120.h 1.1 86/09/25 SMI
 *
 */

#define TRUE            1
#define FALSE           0
#define	EVEC_LEVEL6     0x078
#define INFINITY        0x7fffffff
#define KBD_CMD_RESET   0x01
#define MAXERRS         101
#define MAXWORDS        65536
#define MAXBYTES        131072
#define MAXBAUD         76800
#define CRADDR          0xEE3800
#define SHADBUF         0x0E0000
#define VMADDR          0xEC0000
#define SCCADDR         0xEEC000
#define BAUDHI          15
#define BAUDLO          13
#define LOOP1           17
#define LOOP2           23
#define WAIT_TIME       100000
#define LOBAUD(baud)    ((unsigned char) ((4915200/32)/(baud) -2))
#define HIBAUD(baud)    ((unsigned char) (((4915200/32)/(baud) -2) >>8))
#define exit            (*romp->v_exit_to_mon)



typedef struct bytnode {   /* byte node      */
  char tstname[40];        /* test name      */
  unsigned char *addr;     /* byte address   */
  unsigned char exp;       /* expected value */
  unsigned char obs;       /* observed value */
} BYTNODE;

typedef struct wrdnode {   /* word node      */
  char tstname[40];        /* test name      */
  unsigned short *addr;    /* word address   */
  unsigned short exp;      /* expected value */
  unsigned short obs;      /* observed value */
} WRDNODE;

typedef struct sccnode {   /* scc node       */
  char tstname[40];        /* test name      */
  unsigned char exp;       /* expected value */
  unsigned char obs;       /* observed value */
} SCCNODE;

