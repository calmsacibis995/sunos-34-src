/*	@(#)semsys.c 1.1 86/09/24 SMI; from S5R2 1.3	*/

#include	<syscall.h>
#include	<sys/types.h>
#include	<sys/ipc.h>
#include	<sys/sem.h>

/* semsys dispatch argument */
#define SEMCTL  0
#define SEMGET  1
#define SEMOP   2


semctl(semid, semnum, cmd, arg)
int semid, cmd;
int semnum;
union semun arg;
{
	return(syscall(SYS_semsys, SEMCTL, semid, semnum, cmd, arg));
}

semget(key, nsems, semflg)
key_t key;
int nsems, semflg;
{
	return(syscall(SYS_semsys, SEMGET, key, nsems, semflg));
}

semop(semid, sops, nsops)
int semid;
struct sembuf (*sops)[];
int nsops;
{
	return(syscall(SYS_semsys, SEMOP, semid, sops, nsops));
}
