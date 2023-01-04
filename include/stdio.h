/*	@(#)stdio.h 1.1 86/09/24 SMI; from UCB 1.4 06/30/83	*/

# ifndef FILE
#define	BUFSIZ	1024
#define _SBFSIZ	8
extern	struct	_iobuf {
	int	_cnt;
	unsigned char *_ptr;
	unsigned char *_base;
	int	_bufsiz;
	short	_flag;
	char	_file;		/* should be short */
} _iob[];

#define _IOFBF	0
#define	_IOREAD	01
#define	_IOWRT	02
#define	_IONBF	04
#define	_IOMYBUF	010
#define	_IOEOF	020
#define	_IOERR	040
#define	_IOSTRG	0100
#define	_IOLBF	0200
#define	_IORW	0400
#define	NULL	0
#define	FILE	struct _iobuf
#define	EOF	(-1)

#define	stdin	(&_iob[0])
#define	stdout	(&_iob[1])
#define	stderr	(&_iob[2])

#define	getc(p)		(--(p)->_cnt>=0? ((int)*(p)->_ptr++):_filbuf(p))
#define	getchar()	getc(stdin)
#define putc(x,p) (--(p)->_cnt>=0? (int)(*(p)->_ptr++=(unsigned char)(x)):_flsbuf((unsigned char)(x),p))
#define	putchar(x)	putc((x),stdout)
#define	feof(p)		(((p)->_flag&_IOEOF)!=0)
#define	ferror(p)	(((p)->_flag&_IOERR)!=0)
#define	fileno(p)	((p)->_file)
#define	clearerr(p)	(void) ((p)->_flag &= ~(_IOERR|_IOEOF))

extern FILE	*fopen();
extern FILE	*fdopen();
extern FILE	*freopen();
extern FILE	*popen();
extern FILE	*tmpfile();
extern long	ftell();
extern char	*fgets();
extern char	*gets();
#ifdef vax
char	*sprintf();		/* too painful to do right */
#endif
extern char	*ctermid();
extern char	*cuserid();
extern char	*tempnam();
extern char	*tmpnam();

#define L_ctermid	9
#define L_cuserid	9
#define P_tmpdir	"/usr/tmp/"
#define L_tmpnam	(sizeof(P_tmpdir) + 15)
# endif
