/*	@(#)shmsys.c 1.4 87/02/25 SMI; from S5R2 1.3	*/

#include	<syscall.h>
#include	<sys/types.h>
#include	<sys/ipc.h>
#include	<sys/shm.h>


/* shmsys dispatch argument */
#define	SHMAT	0
#define	SHMCTL	1
#define	SHMDT	2
#define SHMGET	3

	/* PRE-VM-REWRITE */
#define SHMALIGNMENT 4

#include	<errno.h>
static unsigned _shm_pgsz = 0;
static int _shm_malloc = 1;		/* true if pre-vm-rewrite */
char *_malloc_at_addr();
	/* END ... PRE-VM-REWRITE */

char *
shmat(shmid, shmaddr, shmflg)
int shmid;
char *shmaddr;
int shmflg;
{
	/* PRE-VM-REWRITE */
	struct shmid_ds	tmp_shmid;
	register unsigned size;
	register int tmperr, ret;

	/*
	 * First, get the required address alignment.
	 * If this fails, then this is probably a 3.2 binary running
	 * on a post-vm-rewrite kernel, in which case the shmat() is
	 * implemented in the kernel.
	 */
	if (_shm_pgsz == 0) {
		tmperr = errno;
		errno = 0;
		_shm_pgsz = syscall(SYS_shmsys, SHMALIGNMENT);
		if (errno != 0)
			_shm_malloc = 0;	/* must be post-vm-rewrite */
		errno = tmperr;
	}

	/* If post-vm-rewrite, just issue the system call */
	if (!_shm_malloc)
		return ((char *)
		    syscall(SYS_shmsys, SHMAT, shmid, shmaddr, shmflg));

	if (shmctl(shmid, IPC_STAT, &tmp_shmid) == -1) {
		return ((char *) -1);
	}

	size = ((tmp_shmid.shm_segsz + _shm_pgsz - 1) / _shm_pgsz) * _shm_pgsz;

	if (shmaddr != 0) {
		if (shmflg & SHM_RND)
			(unsigned)shmaddr &= ~(_shm_pgsz - 1);
		if (((unsigned)shmaddr & (_shm_pgsz - 1)) ||
		    (shmaddr == (char *)0) ||
		    (_malloc_at_addr(shmaddr, size) != shmaddr)) {
			errno = EINVAL;
			return ((char *) -1);
		}
	} else if ((shmaddr = (char *) memalign(_shm_pgsz, size)) == 0) {
		return ((char *) -1);
	}

	if ((ret = syscall(SYS_shmsys, SHMAT, shmid, shmaddr, shmflg)) != -1) {
		return ((char *) ret);
	}
	tmperr = errno;
	if (free(shmaddr) == -1)
		perror("shmat: free(3) error");
	errno = tmperr;
	return ((char *) ret);

	/* END ... PRE-VM-REWRITE */
}

shmctl(shmid, cmd, buf)
int shmid, cmd;
struct shmid_ds *buf;
{
	return (syscall(SYS_shmsys, SHMCTL, shmid, cmd, buf));
}

shmdt(shmaddr)
char *shmaddr;
{
	/* PRE-VM-REWRITE */
	register int ret;
	/* END ... PRE-VM-REWRITE */

	if ((ret = syscall(SYS_shmsys, SHMDT, shmaddr)) == -1)
		return (ret);

	/* PRE-VM-REWRITE */
	if (_shm_malloc) {			/* if pre-vm-rewrite, */
		register int tmperr;

		tmperr = errno;
		if (free(shmaddr) == -1)	/* free malloc'ed space */
			perror("shmdt: free(3) error");
		errno = tmperr;
	}
	return (ret);
	/* END ... PRE-VM-REWRITE */
}

shmget(key, size, shmflg)
key_t key;
int size, shmflg;
{
	return (syscall(SYS_shmsys, SHMGET, key, size, shmflg));
}
