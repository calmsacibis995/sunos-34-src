#include <sys/time.h>
#include <signal.h>
#include <stdio.h>
#include <sys/file.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include </usr/include/sundev/fpareg.h>
#define FILLER 0x31415926
#define REG_RD_MSW 0xE0000C00
#define REG_RD_LSW 0xE0000C04
#define REG_CPY_DP 0xE0000884
#define REG_CPY_SP 0xE0000880
#define REG_WRM_LP 0xE80
#define REG_WRL_LP 0xE84
#define MAP_CKSUM_ADDR 4*1024-1
#define REG_CKSUM_ADDR 2*1024-1
#define REG_CKSUM_MAX 0x7CF
#define CHECKSUM 0xBEADFACE
#define IOCTL_FAILED 0
#define BUS_ERROR 1
char *error_message[] = {
	"FPA ioctl failed",
	"Download got FPA Bus Error"
};
extern int errno;
struct fpa_device *fpa = (struct fpa_device *) 0xE0000000;
int fpa_fd;
int quiet_flag = 0;
main(argc, argv)
int argc;
char *argv[];
{
char *ufile;
char *mfile;
char *cfile;
int uflg = 0;
int mflg = 0;
int cflg = 0;
int rflg = 0;
int i;
int got_fpeerr();
int got_segverr();
char *dummy;

	signal(SIGFPE, got_fpeerr);
	signal(SIGSEGV, got_segverr);
	for(i = 1; i < argc; i++)
	{
		if(argv[i][0] != '-')
			goto usage;
		else
			switch(argv[i][1])
			{
			case 'u':
				uflg++;
				ufile = argv[++i];
				break;
			case 'm':
				mflg++;
				mfile = argv[++i];
				break;
			case 'c':
				cflg++;
				cfile = argv[++i];
				break;
			case 'r':
				rflg++;
				break;
			case 'q':
				quiet_flag++;
				break;
			default:
				fprintf(stderr, "Bad flag: %c\n", argv[i][1]);
				goto usage;
			}
	}
	if((fpa_fd = open("/dev/fpa", O_RDWR, 0)) == -1)
	{
		switch(errno)
		{
		case ENOENT:
			fprintf(stderr, "can't open /dev/fpa: No 68881 present\n");
			break;
		case ENETDOWN:
			fprintf(stderr, "can't open /dev/fpa: FPA disabled due to probable hardware problems\n");
			break;
		case EIO:
			fprintf(stderr, "can't open /dev/fpa: other FPA process active\n");
			break;
		case EBUSY:
			fprintf(stderr, "can't open /dev/fpa: kernel out of FPA contexts\n");
			break;
		default:
			perror("can't open /dev/fpa");
			break;
		}
		exit(-1);
	}
	if(uflg)
		fpa1_uload(ufile);
	if(mflg)
		fpa1_mload(mfile);
	if(cflg)
		fpa1_cload(cfile);
	if(rflg)
		fpa1_rev();
	close(fpa_fd);
	exit(0);
usage:
	printf("Usage: dload [-u ufile] [-m mfile] [-c cfile] [-r] [-q]\n");
	exit(-1);
}
long ucode[4096 * 3];
/*		Download Micro-code To FPA
/*  filename must be of the following format:
/*    an integer specifying no. of lines
/*    n lines of 96 bits specifying the bits in the ucode 
/*  a checksum will be appended at the end of good data */
fpa1_uload(filename)
	char *filename;
{
FILE *fp;
short nlines;		/* no. of lines of instructions */
register long *ptr;
u_int *ucode_add;
int got_buserr();
char *dummy;

	signal(SIGBUS, got_buserr);
	if((fp=fopen(filename,"r"))==NULL)
	{
		fprintf(stderr, "can't open %s\n", filename);
		exit(-1);
	}
	if(ioctl(fpa_fd, FPA_ACCESS_OFF, dummy) == -1)
	{
		if(errno == EPIPE)
		{
			fprintf(stderr, "can't clear FPA access enable bit: FPA pipe not cleared\n");
			fpa_shutdown(IOCTL_FAILED);
		} else
		{
			perror("can't clear FPA access enable bit");
		}
		exit(-1);
	}
	if(ioctl(fpa_fd, FPA_LOAD_ON, dummy) == -1)
	{
		if(errno == EPIPE)
		{
			fprintf(stderr, "can't set FPA load enable bit: FPA pipe not cleared\n");
			fpa_shutdown(IOCTL_FAILED);
		} else
		{
			perror("can't set FPA load enable bit");
		}
		exit(-1);
	}
	if(fread(&nlines, sizeof(nlines), 1, fp) != 1)
					/* number of microcode lines  */
	{
		fprintf(stderr, "FPA microcode download file has bad format\n");
		exit(-1);
	}
	if(nlines > (4 * 1024 - 1))
	{
		fprintf(stderr, "FPA microcode has overflowed available space\n");
		exit(-1);
	}
	if(fread(ucode, sizeof(long),  3 * nlines, fp) != 3 * nlines)
	{
		fprintf(stderr, "FPA Microcode download file has bad format\n");
		exit(-1);
	}
	ucode[3 * nlines] = CHECKSUM ^ cksum(nlines, ucode, 3);
	ucode[3 * nlines + 1] = CHECKSUM ^ cksum(nlines, ucode + 1, 3);
	ucode[3 * nlines + 2] = CHECKSUM ^ cksum(nlines, ucode + 2, 3);
	ucode_add = 0;
	if(!quiet_flag)
		printf("Downloading FPA microcode from %s\n", filename);
	nlines = 4 * 1024;
	for (ptr = ucode; nlines > 0; nlines--)
	{
		fpa->fp_load_ptr = (long)ucode_add | FPA_BIT_71_64;
		fpa->fp_ld_ram = *ptr++;
		fpa->fp_load_ptr = (long)ucode_add | FPA_BIT_63_32;
		fpa->fp_ld_ram = *ptr++;
		fpa->fp_load_ptr = (long)ucode_add | FPA_BIT_31_0;
		fpa->fp_ld_ram = *ptr++;
		ucode_add++;
	}
	fclose(fp);
	if(!quiet_flag)
		printf("Downloaded  FPA microcode from %s\n", filename);
	return(0);
}
got_buserr()
{
	printf("Got Bus error while trying to access FPA\n");
	fpa_shutdown(IOCTL_FAILED);
}
long map[4096];
/*		Download The Mapping RAM
/*  filename is of the following format:
/*    an integer specifying number of lines, followed by
/*    nlines long's containing 24 mapping ram bits in lower order
/*  a checksum will be inserted at address MAP_CKSUM_ADDR */
fpa1_mload(filename)
	char *filename;
{
FILE *fp;
int nlines;
register long *ptr;
u_long *map_add;
int got_buserr();
char *dummy;

	signal(SIGBUS, got_buserr);
	if((fp=fopen(filename,"r"))==NULL)
	{
		fprintf(stderr, "can't open %s\n", filename);
		exit(-1);
	}
	if(ioctl(fpa_fd, FPA_ACCESS_OFF, dummy) == -1)
	{
		if(errno == EPIPE)
		{
			fprintf(stderr, "can't clear FPA access enable bit: FPA pipe not cleared\n");
			fpa_shutdown(IOCTL_FAILED);
		} else
		{
			perror("can't clear FPA access enable bit");
		}
		exit(-1);
	}
	if(ioctl(fpa_fd, FPA_LOAD_ON, dummy) == -1)
	{
		if(errno == EPIPE)
		{
			fprintf(stderr, "can't set FPA load enable bit: FPA pipe not cleared\n");
			fpa_shutdown(IOCTL_FAILED);
		} else
		{
			perror("can't set FPA load enable bit");
		}
		exit(-1);
	}
	nlines = 4 * 1024;
	if(fread(map, sizeof(long),  nlines, fp) != nlines)
	{
		fprintf(stderr, "FPA mapping ram download file has bad format\n");
		exit(-1);
	}
	map[MAP_CKSUM_ADDR] = CHECKSUM ^ cksum(nlines, map, 1) ^ cksum(1, &map[MAP_CKSUM_ADDR], 1);	/* calculate the checksum of all locations BUT this one */
	if(!quiet_flag)
		printf("Downloading FPA mapping ram from %s\n", filename);
	map_add = (u_long *) 0;
	for (ptr = map; nlines > 0; nlines--)
	{
		fpa->fp_load_ptr = (long)map_add | FPA_BIT_23_0;
		fpa->fp_ld_ram = *ptr++;
		map_add++;
	}
	fclose(fp);
	if(!quiet_flag)
		printf("Downloaded  FPA mapping ram from %s\n", filename);
	return(0);
}
long const[2 * 1024][2];  /* this initializes data to 0; maybe NaN is better */
/*		Download The Constants
/*  filename is of the following format:
/*    all lines start with a name which starts with s, d, c_s, or c_d.
/*    following the name is a tab
/*    if the name starts with s or c_s then the tab is followed by a
/*	hex, eight-digit number
/*    if the name starts with d or c_d then the tab is followed by a
/*	hex sixteen-digit number
/*  a checksum will be inserted at address REG_CKSUM_ADDR */
fpa1_cload(filename)
	char *filename;
{
FILE *fp;
int nlines;
int const_add;
int got_buserr();
char *dummy;
int args;
u_int constant1, constant2;
int count = 0;
int offset;
char string[100];
char name[100];

	signal(SIGBUS, got_buserr);
	if((fp=fopen(filename,"r"))==NULL)
	{
		fprintf(stderr, "can't open %s\n", filename);
		exit(-1);
	}
	if(ioctl(fpa_fd, FPA_ACCESS_ON, dummy) == -1)
	{
		if(errno == EPIPE)
		{
			fprintf(stderr, "can't set FPA access enable bit: FPA pipe not cleared\n");
			fpa_shutdown(IOCTL_FAILED);
		} else
		{
			perror("can't set FPA access enable bit");
		}
		exit(-1);
	}
	if(ioctl(fpa_fd, FPA_LOAD_OFF, dummy) == -1)
	{
		if(errno == EPIPE)
		{
			fprintf(stderr, "can't clear FPA load enable bit: FPA pipe not cleared\n");
			fpa_shutdown(IOCTL_FAILED);
		} else
		{
			perror("can't clear FPA load enable bit");
		}
		exit(-1);
	}
	while(fgets(string, 100, fp) != NULL)
	{
		if((args = sscanf(string, "%s%x%8x%8x", name, &const_add, &constant1, &constant2)) <=2)
			continue;
		switch(*name)
		{
		case 's':
			offset = 0x400;
			ck_loc(name, const_add+offset);
			const[const_add+offset][0] = constant1;
			const[const_add+offset][1] = FILLER;
			break;
		case 'd':
			offset = 0x600;
			ck_loc(name, const_add+offset);
			const[const_add+offset][0] = constant1;
			const[const_add+offset][1] = constant2;
			break;
		case 'c':
			if(name[2] == 's')
			{
				offset = 0x500;
				ck_loc(name, const_add+offset);
				const[const_add+offset][0] = constant1;
				const[const_add+offset][1] = FILLER;
				break;
			} else if(name[2] == 'd')
			{
				offset = 0x700;
				ck_loc(name, const_add+offset);
				const[const_add+offset][0] = constant1;
				const[const_add+offset][1] = constant2;
				break;
			}
		default:
			fprintf(stderr, "FPA constant file has bad format\n");
			exit(-1);
		}
		count++;
	}
	if(!quiet_flag)
		printf("   FPA read in %d constants\n", count);
	const[REG_CKSUM_ADDR][0] = CHECKSUM ^ cksum(REG_CKSUM_MAX - 0x400 +1, &const[0x400][0], 2);
	const[REG_CKSUM_ADDR][1] = CHECKSUM ^ cksum(REG_CKSUM_MAX - 0x400 +1, &const[0x400][1], 2);
	if(!quiet_flag)
		printf("Downloading FPA constants from %s\n", filename);
	nlines = 2 * 1024;
	for (const_add = 1024; const_add < 2 * 1024; const_add++)
	{
		fpa->fp_load_ptr = (long)(const_add << 2);
		*(long *)((long)fpa + REG_WRM_LP) = const[const_add][0];
		*(long *)((long)fpa + REG_WRL_LP) = const[const_add][1];
	}
	if(!quiet_flag)
		printf("Downloaded FPA constants from %s\n", filename);
	fpa->fp_initialize = 0;
	fpa->fp_restore_mode3_0 = 0x2;
	if(ioctl(fpa_fd, FPA_ACCESS_OFF, dummy) == -1)
	{
		if(errno == EPIPE)
		{
			fprintf(stderr, "can't clear FPA access enable bit: FPA pipe not cleared\n");
			fpa_shutdown(IOCTL_FAILED);
		} else
		{
			perror("can't clear FPA access enable bit");
		}
		exit(-1);
	}
	if(ioctl(fpa_fd, FPA_LOAD_OFF, dummy) == -1)
	{
		if(errno == EPIPE)
		{
			fprintf(stderr, "can't clear FPA load enable bit: FPA pipe not cleared\n");
			fpa_shutdown(IOCTL_FAILED);
		} else
		{
			perror("can't clear FPA load enable bit");
		}
		exit(-1);
	}
	if(ioctl(fpa_fd, FPA_INIT_DONE, dummy) == -1)
	{
		if(errno == EPIPE)
		{
			fprintf(stderr, "can't clear FPA load enable bit: FPA pipe not cleared\n");
		} else
		{
			perror("can't clear FPA load enable bit");
		}
		fpa_shutdown(IOCTL_FAILED);
		exit(-1);
	}
	if(!quiet_flag)
		printf("FPA initialized\n");
	fclose(fp);
	return(0);
}
ck_loc(name, index)
char *name;
long index;
{
	if(const[index][0] || const[index][1])
		fprintf(stderr, "Warning! FPA Reloading constant: %12s at address %3x\n", name, index);
}
fpa1_rev()
{
	*(long *)REG_CPY_DP = 0xfc00;
	printf("Microcode = level: %lx, date: %06lx\n", *(long *)REG_RD_MSW, *(long *)REG_RD_LSW);
	*(long *)REG_CPY_DP = 0xfc40;
	printf("Constants = level: %lx, date: %06lx\n", *(long *)REG_RD_MSW, *(long *)REG_RD_LSW);
}
long cksum(nlines, ptr, inc)
int nlines;
long *ptr;
{
long sum = 0;

	while(nlines--)
	{
		sum ^= *ptr;
		ptr += inc;
	}
	return(sum);
}
got_fpeerr()
{
	signal(SIGFPE, got_fpeerr);
	fprintf(stderr, "Got a fpe error\n");
	fprintf(stderr, "The FPA IERR is: %xl\n", fpa->fp_ierr);
	if(errno == EPIPE)
	{
		fpa_shutdown(BUS_ERROR);
	}
	exit(-1);
}
got_segverr()
{
	signal(SIGSEGV, got_segverr);
	fprintf(stderr, "Got a segmentation error\n");
	exit(-1);
}

char msg[80] = "FPA Failed Download - ";

fpa_shutdown(val)
	int val;
{
char *time_of_day();

	strcat(msg, error_message[val]);
	strcat(msg, " - ");
	strcat(msg, time_of_day());
	
	broadcast_msg(msg);
	log_msg(msg);
		/* disable the fpa */
	if(ioctl(fpa_fd, FPA_FAIL, msg) == -1)
	{
		perror("can't disable fpa");
	}
	exit(-1);
}

char *time_of_day()
{
        long     temptime;
 
        time(&temptime); 
        return(ctime(&temptime));

}

log_msg(msg)
	char *msg;
{
	FILE    *input_file, *fopen();

	if ((input_file = fopen("/usr/adm/diaglog","a")) == NULL)
	{
		perror("could not open /usr/adm/diaglog");
	} else
	{
		fputs(msg,input_file);
		fclose(input_file);		
	}
}
char	file_name[30] = "/tmp/\0";	
char	wall_str[30] = "wall \0";

broadcast_msg(msg)
char *msg;
{
	FILE	*input_file, *fopen();

		/* create temp file with error message */
	tmpnam(&file_name[5]);
	input_file = fopen(file_name,"w");	
	fputs(msg, input_file);
	fclose(input_file);

	strcpy(&wall_str[5], file_name);
	system(wall_str);

	unlink(file_name);
}
