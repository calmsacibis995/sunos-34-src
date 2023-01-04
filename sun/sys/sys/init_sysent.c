/*	@(#)init_sysent.c 1.1 86/09/25 SMI; from UCB 6.1 83/08/17	*/

/*
 * System call switch table.
 */

#include "../h/param.h"
#include "../h/systm.h"

int	nosys();
int	nullsys();
int	errsys();

/* 1.1 processes and protection */
int	gethostid(),sethostname(),gethostname(),getpid();
int	setdomainname(), getdomainname();
int	fork(),rexit(),execv(),execve(),wait();
int	getuid(),setreuid(),getgid(),getgroups(),setregid(),setgroups();
int	getpgrp(),setpgrp();

/* 1.2 memory management */
int	sbrk(),sstk();
int	getpagesize(),smmap(),mremap(),munmap(),mprotect(),madvise(),mincore();

/* 1.3 signals */
int	sigvec(),sigblock(),sigsetmask(),sigpause(),sigstack(),sigcleanup();
int	kill(), killpg();

/* 1.4 timing and statistics */
int	gettimeofday(),settimeofday();
int	adjtime();
int	getitimer(),setitimer();

/* 1.5 descriptors */
int	getdtablesize(),dup(),dup2(),close();
int	select(),getdopt(),setdopt(),fcntl(),flock();

/* 1.6 resource controls */
int	getpriority(),setpriority(),getrusage(),getrlimit(),setrlimit();
#ifdef QUOTA
int	oldquota(), quotactl();
#else
#define oldquota nullsys	/* for backward compatability with old login */
#endif QUOTA

/* 1.7 system operation support */
int	mount(),unmount(),swapon();
int	sync(),reboot();
#ifdef SYSACCT
int	sysacct();
#endif SYSACCT

/* 2.1 generic operations */
int	read(),write(),readv(),writev(),ioctl();

/* 2.2 file system */
int	chdir(),chroot();
int	mkdir(),rmdir(),getdirentries();
int	creat(),open(),mknod(),unlink(),stat(),fstat(),lstat();
int	chown(),fchown(),chmod(),fchmod(),utimes();
int	link(),symlink(),readlink(),rename();
int	lseek(),truncate(),ftruncate(),access(),fsync();
int	statfs(),fstatfs();

/* 2.3 communications */
int	socket(),bind(),listen(),accept(),connect();
int	socketpair(),sendto(),send(),recvfrom(),recv();
int	sendmsg(),recvmsg(),shutdown(),setsockopt(),getsockopt();
int	getsockname(),getpeername(),pipe();

int	umask();		/* XXX */

/* 2.3.1 SystemV-compatible IPC */
#ifdef IPCSEMAPHORE
int	semsys();
#endif
#ifdef IPCMESSAGE
int	msgsys();
#endif
#ifdef IPCSHMEM
int	shmsys();
#endif

/* 2.4 processes */
int	ptrace();

/* 2.5 terminals */

#ifdef COMPAT
/* emulations for backwards compatibility */
#define	compat(n, name)	n, o/**/name

int	owait();		/* now receive message on channel */
int	otime();		/* now use gettimeofday */
int	ostime();		/* now use settimeofday */
int	oalarm();		/* now use setitimer */
int	outime();		/* now use utimes */
int	opause();		/* now use sigpause */
int	onice();		/* now use setpriority,getpriority */
int	oftime();		/* now use gettimeofday */
int	osetpgrp();		/* ??? */
int	otimes();		/* now use getrusage */
int	ossig();		/* now use sigvec, etc */
int	ovlimit();		/* now use setrlimit,getrlimit */
int	ovtimes();		/* now use getrusage */
int	osetuid();		/* now use setreuid */
int	osetgid();		/* now use setregid */
int	ostat();		/* now use stat */
int	ofstat();		/* now use fstat */
#else
#define	compat(n, name)	0, nosys
#endif

/* BEGIN JUNK */
#ifdef vax
int	resuba();
#ifdef TRACE
int	vtrace();
#endif TRACE
#endif vax
int	profil();		/* 'cuz sys calls are interruptible */
int	vhangup();		/* should just do in exit() */
int	vfork();		/* awaiting fork w/ copy on write */
int	obreak();		/* awaiting new sbrk */
int	ovadvise();		/* awaiting new madvise */
int	indir();		/* indirect system call */
int	ustat();		/* System V compatibility */
int	umount();		/* still more System V (and 4.2?) compatibility */

#ifdef DEBUG
int debug();
#endif
/* END JUNK */

#ifdef NFS
/* nfs */
int	async_daemon();		/* client async daemon */
int	nfs_svc();		/* run nfs server */
int	nfs_getfh();		/* get file handle */
int	exportfs();		/* export file systems */
#endif

struct sysent sysent[] = {
	1, indir,			/*   0 = indir */
	1, rexit,			/*   1 = exit */
	0, fork,			/*   2 = fork */
	3, read,			/*   3 = read */
	3, write,			/*   4 = write */
	3, open,			/*   5 = open */
	1, close,			/*   6 = close */
	compat(0,wait),			/*   7 = old wait */
	2, creat,			/*   8 = creat */
	2, link,			/*   9 = link */
	1, unlink,			/*  10 = unlink */
	2, execv,			/*  11 = execv */
	1, chdir,			/*  12 = chdir */
	compat(0,time),			/*  13 = old time */
	3, mknod,			/*  14 = mknod */
	2, chmod,			/*  15 = chmod */
	3, chown,			/*  16 = chown; now 3 args */
	1, obreak,			/*  17 = old break */
	compat(2,stat),			/*  18 = old stat */
	3, lseek,			/*  19 = lseek */
	0, getpid,			/*  20 = getpid */
	0, nosys,			/*  21 = old mount */
	1, umount,			/*  22 = old umount */
	compat(1,setuid),		/*  23 = old setuid */
	0, getuid,			/*  24 = getuid */
	compat(1,stime),		/*  25 = old stime */
	5, ptrace,			/*  26 = ptrace */
	compat(1,alarm),		/*  27 = old alarm */
	compat(2,fstat),		/*  28 = old fstat */
	compat(0,pause),		/*  29 = opause */
	compat(2,utime),		/*  30 = old utime */
	0, nosys,			/*  31 = was stty */
	0, nosys,			/*  32 = was gtty */
	2, access,			/*  33 = access */
	compat(1,nice),			/*  34 = old nice */
	compat(1,ftime),		/*  35 = old ftime */
	0, sync,			/*  36 = sync */
	2, kill,			/*  37 = kill */
	2, stat,			/*  38 = stat */
	compat(2,setpgrp),		/*  39 = old setpgrp */
	2, lstat,			/*  40 = lstat */
	2, dup,				/*  41 = dup */
	0, pipe,			/*  42 = pipe */
	compat(1,times),		/*  43 = old times */
	4, profil,			/*  44 = profil */
	0, nosys,			/*  45 = nosys */
	compat(1,setgid),		/*  46 = old setgid */
	0, getgid,			/*  47 = getgid */
	compat(2,ssig),			/*  48 = old sig */
	0, nosys,			/*  49 = reserved for USG */
	0, nosys,			/*  50 = reserved for USG */
#ifdef SYSACCT
	1, sysacct,			/*  51 = turn acct off/on */
#else
	0, errsys,			/*  51 = not configured */
#endif SYSACCT
	0, nosys,			/*  52 = old set phys addr */
	0, nosys,			/*  53 = old lock in core */
	3, ioctl,			/*  54 = ioctl */
	1, reboot,			/*  55 = reboot */
	0, nosys,			/*  56 = old mpxchan */
	2, symlink,			/*  57 = symlink */
	3, readlink,			/*  58 = readlink */
	3, execve,			/*  59 = execve */
	1, umask,			/*  60 = umask */
	1, chroot,			/*  61 = chroot */
	2, fstat,			/*  62 = fstat */
	0, nosys,			/*  63 = used internally */
	1, getpagesize,			/*  64 = getpagesize */
	5, mremap,			/*  65 = mremap */
	0, vfork,			/*  66 = vfork */
	0, read,			/*  67 = old vread */
	0, write,			/*  68 = old vwrite */
	1, sbrk,			/*  69 = sbrk */
	1, sstk,			/*  70 = sstk */
	6, smmap,			/*  71 = mmap */
	1, ovadvise,			/*  72 = old vadvise */
	2, munmap,			/*  73 = munmap */
	3, mprotect,			/*  74 = mprotect */
	3, madvise,			/*  75 = madvise */
	1, vhangup,			/*  76 = vhangup */
	compat(2,vlimit),		/*  77 = old vlimit */
	3, mincore,			/*  78 = mincore */
	2, getgroups,			/*  79 = getgroups */
	2, setgroups,			/*  80 = setgroups */
	1, getpgrp,			/*  81 = getpgrp */
	2, setpgrp,			/*  82 = setpgrp */
	3, setitimer,			/*  83 = setitimer */
	0, wait,			/*  84 = wait */
	1, swapon,			/*  85 = swapon */
	2, getitimer,			/*  86 = getitimer */
	2, gethostname,			/*  87 = gethostname */
	2, sethostname,			/*  88 = sethostname */
	0, getdtablesize,		/*  89 = getdtablesize */
	2, dup2,			/*  90 = dup2 */
	2, getdopt,			/*  91 = getdopt */
	3, fcntl,			/*  92 = fcntl */
	5, select,			/*  93 = select */
	2, setdopt,			/*  94 = setdopt */
	1, fsync,			/*  95 = fsync */
	3, setpriority,			/*  96 = setpriority */
	3, socket,			/*  97 = socket */
	3, connect,			/*  98 = connect */
	3, accept,			/*  99 = accept */
	2, getpriority,			/* 100 = getpriority */
	4, send,			/* 101 = send */
	4, recv,			/* 102 = recv */
	0, nosys,			/* 103 = old socketaddr */
	3, bind,			/* 104 = bind */
	5, setsockopt,			/* 105 = setsockopt */
	2, listen,			/* 106 = listen */
	compat(2,vtimes),		/* 107 = old vtimes */
	3, sigvec,			/* 108 = sigvec */
	1, sigblock,			/* 109 = sigblock */
	1, sigsetmask,			/* 110 = sigsetmask */
	1, sigpause,			/* 111 = sigpause */
	2, sigstack,			/* 112 = sigstack */
	3, recvmsg,			/* 113 = recvmsg */
	3, sendmsg,			/* 114 = sendmsg */
#ifdef TRACE
	2, vtrace,			/* 115 = vtrace */
#else
	0, nosys,			/* 115 = nosys */
#endif
	2, gettimeofday,		/* 116 = gettimeofday */
	2, getrusage,			/* 117 = getrusage */
	5, getsockopt,			/* 118 = getsockopt */
#ifdef vax
	1, resuba,			/* 119 = resuba */
#else
	0, nosys,			/* 119 = nosys */
#endif
	3, readv,			/* 120 = readv */
	3, writev,			/* 121 = writev */
	2, settimeofday,		/* 122 = settimeofday */
	3, fchown,			/* 123 = fchown */
	2, fchmod,			/* 124 = fchmod */
	6, recvfrom,			/* 125 = recvfrom */
	2, setreuid,			/* 126 = setreuid */
	2, setregid,			/* 127 = setregid */
	2, rename,			/* 128 = rename */
	2, truncate,			/* 129 = truncate */
	2, ftruncate,			/* 130 = ftruncate */
	2, flock,			/* 131 = flock */
	0, nosys,			/* 132 = nosys */
	6, sendto,			/* 133 = sendto */
	2, shutdown,			/* 134 = shutdown */
	5, socketpair,			/* 135 = socketpair */
	2, mkdir,			/* 136 = mkdir */
	1, rmdir,			/* 137 = rmdir */
	2, utimes,			/* 138 = utimes */
	0, sigcleanup,			/* 139 = signalcleanup */
	2, adjtime,			/* 140 = adjtime */
	3, getpeername,			/* 141 = getpeername */
	2, gethostid,			/* 142 = gethostid */
	0, nosys,			/* 143 = old sethostid */
	2, getrlimit,			/* 144 = getrlimit */
	2, setrlimit,			/* 145 = setrlimit */
	2, killpg,			/* 146 = killpg */
	0, nosys,			/* 147 = nosys */
	0, oldquota,	/* XXX */	/* 148 = old quota */
	0, oldquota,	/* XXX */	/* 149 = old qquota */
	3, getsockname,			/* 150 = getsockname */
#ifdef DEBUG
	1, debug,			/* 151 = debug */
#else
	0, nosys,			/* 151 = nosys */
#endif
	0, nosys,			/* 152 = nosys */
	0, nosys,			/* 153 = nosys */
#ifdef NFS
	0, nosys,			/* 154 = old nfs_mount */
	1, nfs_svc,			/* 155 = nfs_svc */
#else
	0, nosys,			/* 154 = nosys */
	0, nosys,			/* 155 = nosys */
#endif
	4, getdirentries,		/* 156 = getdirentries */
	2, statfs,			/* 157 = statfs */
	2, fstatfs,			/* 158 = fstatfs */
	1, unmount,			/* 159 = unmount */
#ifdef NFS
	0, async_daemon,		/* 160 = async_daemon */
	2, nfs_getfh,			/* 161 = get file handle */
#else
	0, nosys,			/* 160 = nosys */
	0, nosys,			/* 161 = nosys */
#endif
	2, getdomainname,		/* 162 = getdomainname */
	2, setdomainname,		/* 163 = setdomainname */
	0, nosys,			/* 164 = old pcfs_mount */
#ifdef QUOTA
	4, quotactl,			/* 165 = quotactl */
#else
	0, errsys,			/* 165 = not configured */
#endif QUOTA
#ifdef NFS
	3, exportfs,			/* 166 = exportfs */
#else
	0, errsys,			/* 166 = not configured */
#endif
	4, mount,			/* 167 = mount */
	2, ustat,			/* 168 = ustat */
#ifdef IPCSEMAPHORE
	5, semsys,			/* 169 = semsys */
#else
	0, errsys,			/* 169 = not configured */
#endif
#ifdef IPCMESSAGE
	6, msgsys,			/* 170 = msgsys */
#else
	0, errsys,			/* 170 = not configured */
#endif
#ifdef IPCSHMEM
	4, shmsys,			/* 171 = shmsys */
#else
	0, errsys,			/* 171 = not configured */
#endif
};
int	nsysent = sizeof (sysent) / sizeof (sysent[0]);
