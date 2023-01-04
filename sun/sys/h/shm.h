/*	@(#)shm.h 1.1 86/09/25 SMI; from S5R2 6.1 */

/*
 *	IPC Shared Memory Facility.
 */

/*
 *	Shared Memory Operation Flags.
 */

#define	SHM_RDONLY	010000	/* attach read-only (else read-write) */
#define	SHM_RND		020000	/* round attach address to SHMLBA */

/*
 * Shmctl Command Definitions.
 */

#define SHM_LOCK	3	/* Lock segment in core */
#define SHM_UNLOCK	4	/* Unlock segment */

/*
 *	Implementation Constants.
 */
#define	SHMLBA	ctob(1)		/* segment low boundary address multiple */
				/* (SHMLBA must be a power of 2) */

/*
 *	Structure Definitions.
 */

/*
 *	There is a shared mem id data structure for each segment in the system.
 */

struct shmid_ds {
	struct ipc_perm	shm_perm;	/* operation permission struct */
	int		shm_segsz;	/* size of segment in bytes */
/*	struct region	*shm_reg; */	/* ptr to region structure */
/*	char		pad[4];	*/	/* for swap compatibility */
	ushort		shm_lpid;	/* pid of last shmop */
	ushort		shm_cpid;	/* pid of creator */
	ushort		shm_nattch;	/* region attach counter */
/*	ushort		shm_cnattch; */	/* used only for shminfo */
	time_t		shm_atime;	/* last shmat time */
	time_t		shm_dtime;	/* last shmdt time */
	time_t		shm_ctime;	/* last change time */

	/* PRE-VM-REWRITE */
	/* NOTE: shm_kaddr added / shm_reg removed */
	uint		shm_kaddr;	/* region address in kernel v-space */
};



#ifdef KERNEL
/*
 *	Permission Definitions.
 */

#define	SHM_W	0200	/* write permission */
#define	SHM_R	0400	/* read permission */

/*
 *	ipc_perm Mode Definitions.
 */

#define	SHM_INIT	01000	/* grow segment on next attach */
#define	SHM_DEST	02000	/* destroy segment when # attached = 0 */

	/* PRE-VM-REWRITE */
#define SHM_LOCKED	004000	/* shmid locked */
#define SHM_LOCKWAIT	010000	/* shmid wanted */

#define	PSHM	(PZERO + 1)	/* sleep priority */

/* define resource locking macros */
#define SHMLOCK(sp) { \
	while ((sp)->shm_perm.mode & SHM_LOCKED) { \
		(sp)->shm_perm.mode |= SHM_LOCKWAIT; \
		(void) sleep((caddr_t)(sp), PSHM); \
	} \
	(sp)->shm_perm.mode |= SHM_LOCKED; \
}

#define SHMUNLOCK(sp) { \
	(sp)->shm_perm.mode &= ~SHM_LOCKED; \
	if ((sp)->shm_perm.mode & SHM_LOCKWAIT) { \
		(sp)->shm_perm.mode &= ~SHM_LOCKWAIT; \
		curpri = PSHM; \
		wakeup((caddr_t)(sp)); \
	} \
}
	/* END ... PRE-VM-REWRITE */

/*
 *	Shared Memory information structure
 */
struct	shminfo {
	int	shmmax,		/* max shared memory segment size */
		shmmin,		/* min shared memory segment size */
		shmmni,		/* # of shared memory identifiers */
		shmseg,		/* max attached shared memory	  */
				/* segments per process		  */
		shmall;		/* max total shared memory system */
				/* wide (in clicks)		  */
};
struct shminfo	shminfo;	/* configuration parameters */

/*
 *	Configuration Parameters
 * These parameters are tuned by editing the system configuration file.
 * The following lines establish the default values.
 */
#ifndef	SHMPOOL
#define	SHMPOOL	512	/* max total shared memory system wide (in Kbytes) */
#endif
#ifndef	SHMSEG
#define	SHMSEG	6	/* max attached shared memory segments per process */
#endif
#ifndef	SHMMNI
#define	SHMMNI	100	/* # of shared memory identifiers */
#endif

/* The following parameters are assumed not to require tuning */
#define	SHMMIN	1			/* min shared memory segment size */
#define	SHMMAX	(SHMPOOL * 1024)	/* max shared memory segment size */
#define	SHMALL	(SHMMAX / ctob(1))	/* total shared memory (in clicks) */


/*
 * Structures allocated in machdep.c
 */
struct shmid_ds	*shmem;		/* shared memory id pool */

	/* PRE-VM-REWRITE */
struct shmid_ds	**shm_shmem;	/* shared memory process table */
	/* END ... PRE-VM-REWRITE */

#endif KERNEL
