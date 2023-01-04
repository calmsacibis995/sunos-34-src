
/*
  This diagnostic is a quick and dirty test of the two main areas of the
  model 160 colorboard (frame buffer and colormap) with some rasterops thrown
  in for taste. It does not test all of the frame buffer (takes too long!), nor
  does it test the zoom and pan circuitry. 
*/

#include <signal.h>
#include <stdio.h>
#include <sys/file.h>
#include <suntool/gfx_hs.h> 

#define INFO                    0
#define WARNING                 1
#define FATAL                   2
#define ERROR                   3
#define NO_SD_LOG_DIR           1
#define NO_OPEN_LOG             2
#define DEV_NOT_OPEN            3
#define RED_FAILED           	4 
#define GREEN_FAILED           	5 
#define BLUE_FAILED           	6 
#define FRAME_BUFFER_ERROR	7
#define END_ERROR               8
#define USAGE_ERROR             99
#define TEST_NAME               "c160"
#define LOGFILE_NAME    	"log.c160.XXXXXX"

#define cnvrt(a) ((unsigned char)(a * 255))

char device_name[15];
char *device = device_name;
int  atn = FALSE;
int sending_atn_msg = FALSE;
int  debug = FALSE;
int  pass = 0;
int errors = 0;
int verbose = TRUE;
int  stop_on_error = TRUE;

int failed = 0, logfd = 0;
char logfile_name[50];
char *logfile = logfile_name;
extern char *getenv();

int exec_by_sysdiag = FALSE;
int verify = FALSE;
int load_test = FALSE;
int simulate_error = 0;
char perror_msg_buffer[30];
char *perror_msg = perror_msg_buffer;
char tmp_msg_buffer[30];
char *tmp_msg = tmp_msg_buffer;
char msg_buffer[200];
char *msg = msg_buffer;
int  retry_cmp_error = FALSE;
int  display_read_data = FALSE;

static char     sccsid[] = "@(#)c160.c 1.1 9/25/86 Copyright 1985 Sun Micro";

char file_name_buffer[30];
char *file_name = file_name_buffer;

int fake_it = FALSE;

struct color {
	unsigned char red;
	unsigned char grn;
	unsigned char blu;
} clr;

int  clean_up = 0;

int planes = 0xff, a, b, c, x, y, tmp, xdim, ydim, pcnt;
unsigned char rmap[256], gmap[256], bmap[256], rmap1[256], gmap1[256], bmap1[256], rmap2[256], gmap2[256], bmap2[256];
struct pixrect *screen;
struct pixrect *mpr;
struct pixrect pr = {0, 0, 0, 0, 0};
struct cursor cr = {0, 0, 0, &pr};

static	int fixup_colormap();
static	int gfx_fd;

 
main(argc, argv)
int     argc;
char    *argv[];
{
   int 		arrcount, match;
   extern 	finish();
	
   struct	gfxsubwindow *g;
   struct	screen s;
   int		scrfd;
   struct	inputmask im;
   int		pid;
   char		name[WIN_NAMESIZE];	
 
   signal(SIGHUP, finish);
   signal(SIGTERM, finish);
   signal(SIGINT, finish);
                         
   if (getenv("SD_LOG_DIRECTORY")) exec_by_sysdiag = TRUE;
   if (getenv("SD_LOAD_TEST"))
        if (strcmp(getenv("SD_LOAD_TEST"), "yes") == 0) load_test = TRUE;
   if (getenv("SUN_MANUFACTURING")) {
      if (strcmp(getenv("SUN_MANUFACTURING"), "yes") == 0) {
         display_read_data = TRUE;
         retry_cmp_error = TRUE;
      }  
      if (strcmp(getenv("RUN_ON_ERROR"), "enabled") == 0) {
         stop_on_error = FALSE;
      }  
   } 

   sprintf(perror_msg, "%s: perror says", TEST_NAME);
   strcpy(device, "");
 
   if (argc > 1)
      for(arrcount=1; arrcount < argc; arrcount++) {
                        match = 0;
                        if (strcmp(argv[arrcount], "sd") == 0) {
                                match = TRUE;
                                exec_by_sysdiag = TRUE;
                                verbose = FALSE;
                        }
                        if (strncmp(argv[arrcount], "atn", 3) == 0) {
                                match = TRUE;
                                atn = TRUE;
                                if (argv[arrcount][3] != 't')
                                    sending_atn_msg = TRUE;
                                verbose = FALSE;
                        }
                        if (strcmp(argv[arrcount], "d") == 0) {
                                match = TRUE;
                                debug = TRUE;
                                verbose = TRUE;
                        }
                        if (strcmp(argv[arrcount], "dd") == 0) {
                                match = TRUE;
                                display_read_data = TRUE;
                        }
                        if (strcmp(argv[arrcount], "v") == 0) {
                                match = TRUE;
                                verify = TRUE;
                        }
                        if (strcmp(argv[arrcount], "lt") == 0) {
                                match = TRUE;
                                load_test = TRUE;
                        }
                        if (strcmp(argv[arrcount], "re") == 0) {
                                match = TRUE;
                                stop_on_error = FALSE;
                                retry_cmp_error = TRUE;
                        }
                        if (argv[arrcount][0] == 'e') {
                                simulate_error = atoi(&argv[arrcount][1]);
                                if (simulate_error > 0 &&
						simulate_error < END_ERROR) {
                                   match = TRUE;
                                }
                        }
                        if (!match) {
                           printf("Usage: %s [v] [sd/atn] [re] [lt] [dd] [d] [e{1-%d}]\n", TEST_NAME, END_ERROR - 1);
                           exit(USAGE_ERROR);
                        }
      }  

   if (verify) {               /* verify mode */
        if (verbose) printf("%s: Verify mode.\n", TEST_NAME);
        exit(0);
   }
   if (atn || verbose) { 
       if (atn) send_msg_to_atn(INFO, "Started.");
       throwup(0, "%s: Started.", TEST_NAME);  
   }              
	
/* This section sets the window system up so that the background windows remain intact */


	 if (getenv("WINDOW_GFX") != 0 && (g = gfxsw_init(0, 0))) {
		/*
		 * Set so mouse input will get stopped at this window
		 * instead of being directed to underlaying windows.
		 */
		input_imnull(&im);
		win_setinputcodebit(&im,MS_LEFT);
		win_setinputcodebit(&im, LOC_MOVEWHILEBUTDOWN);
		win_setinputcodebit(&im,MS_MIDDLE);
		win_setinputcodebit(&im,MS_RIGHT);
		gfxsw_setinputmask(g, &im, &im,
			win_nametonumber(getenv("WINDOW_ME")), 1, 1);
		gfx_fd = g->gfx_windowfd;
		/* Set environment so that SunWindows programs will run */
		win_fdtoname(gfx_fd, name);	
		we_setgfxwindow(name);	
		we_setmywindow(name);	
		/*
		 * Take gfx window and remove from current place in display tree
		 * and reinstall to cover the entire window.
		 */
		win_screenget(gfx_fd, &s);
		win_lockdata(gfx_fd);
		win_remove(gfx_fd);
		win_setlink(gfx_fd, WL_PARENT, win_nametonumber(s.scr_rootname));
		scrfd = open(s.scr_rootname, 0);
		win_setlink(gfx_fd, WL_OLDERSIB, win_getlink(scrfd, WL_TOPCHILD));
		close(scrfd);
		win_setlink(gfx_fd, WL_YOUNGERSIB, WIN_NULLLINK);
		win_setrect(gfx_fd, &s.scr_rect);
		win_insert(gfx_fd);
		win_unlockdata(gfx_fd);
		/* Give screen sized window an invisible cursor */
		win_setcursor(gfx_fd, &cr);
		clean_up = TRUE;
	   }

        if (simulate_error == DEV_NOT_OPEN) 
	   strcpy(file_name, "/dev/fb.invalid");
        else strcpy(file_name, "/dev/fb");
     
        if ((screen = pr_open(file_name)) == 0) {
           perror(perror_msg);
           if (atn) send_msg_to_atn(FATAL,
               "Couldn't open file '%s', pass %d, errors %d.",
                file_name, pass, errors);
           throwup(-DEV_NOT_OPEN, "%s: Couldn't open file '%s',%s errors %d.",
               TEST_NAME, file_name, print_pass()? tmp_msg:"", errors);
        }

   while (atn || pass == 0) {
      pass++;
      exec_diag();
      if (load_test) break;
      if (!verbose) sleep (5);
      if (atn || verbose)
        printf("%s: pass %d, errors %d.\n", TEST_NAME, pass, errors);
   }                     
   if (clean_up) {
      /* Grabio has hidden semantic of reloading colormap */
      win_grabio(gfx_fd);
      /* Release io lock */
      win_releaseio(gfx_fd);
   }
   if (atn || verbose)   
      throwup(0, "%s: Stopped, pass %d, errors %d.", TEST_NAME, pass, errors);
   exit(0);
}


/* This is the diagnostic section proper */

exec_diag()
{
	int tx, ty, i, z;
	int check_pix;

	xdim = 1152;
	ydim = 900;

	pr_putattributes(screen, &planes);
	loadmap();
				/* write a pixrect of a certain color and dimension to screen */
	for(x=y=0,a=65,b=50,c=0; x<(xdim/2),y<(ydim/2),a>0,b>0; x+=a,y+=b,++c,a-=4,b-=3) {
		tx = xdim-(2*x);
		ty = ydim-(2*y);
		pr_rop(screen,x,y,tx,ty,PIX_SRC|PIX_COLOR(c),0,0,0); 	/* do it */
		if (load_test || verbose) check_pix = 50;
		else check_pix = 1;
		pix_chka(c,x,y,tx,ty,check_pix);	/* verify it */
	}
	llmap();
}

loadmap()	/* set up color map with all permutations of color to be displayed */
{
	int i, j;
	float a,b;
	struct color *hsv_rgb(), *col;

	a = b = 1.0;
		for (i = 0, j = 0; i < 80 ;++i,j+=18,a-=.0125) {
			col = hsv_rgb(j,a,b);
			rmap[i] = col->red;
			gmap[i] = col->grn;
			bmap[i] = col->blu;
			if (j == 360) {
				j = 0;
			}
		}
	a = b = 1.0;
		for (; i < 160 ;++i,j+=18,b-=.0125) {
			col = hsv_rgb(j,a,b);
			rmap[i] = col->red;
			gmap[i] = col->grn;
			bmap[i] = col->blu;
			if (j == 360) {
				j = 0;
			}
		}
	a = b = 1.0;
		for (; i < 240 ;++i,j+=18,a-=.0125,b-=.0125) {
			col = hsv_rgb(j,a,b);
			rmap[i] = col->red;
			gmap[i] = col->grn;
			bmap[i] = col->blu;
			if (j == 360) {
				j = 0;
			}
		}
	pr_putcolormap(screen, 0, 256, rmap, gmap, bmap);
}


llmap()
{
	int i, x, z;

		for (i = 0; i < 220; ++i) {	/* zoom through colormap forward, 20 colors at a time */
			for (x = 19; x >=0; --x) {
				rmap1[x] = rmap[i + x];
				gmap1[x] = gmap[i + x];
				bmap1[x] = bmap[i + x];
				pr_putcolormap(screen, 0, 20, rmap1, gmap1, bmap1);
				pr_getcolormap(screen, 0, 20, rmap2, gmap2, bmap2);
				map_chk(20);

			}
		}
		for (i = 220; i >= 0; --i) {	/* zoom through colormap backward, 20 colors at a time */
			for (x = 0; x < 20; ++x) {
				rmap1[x] = rmap[x + i];
				gmap1[x] = gmap[x + i];
				bmap2[x] = bmap[x + i];
				pr_putcolormap(screen, 0, 20, rmap1, gmap1, bmap1);
				pr_getcolormap(screen, 0, 20, rmap2, gmap2, bmap2);
				map_chk(20);

			}
		}
		pr_putcolormap(screen, 0, 256, rmap1, gmap1, bmap1);
		pr_getcolormap(screen, 0, 256, rmap2, gmap2, bmap2);
		map_chk(256);
}

map_chk(it) 	/* verify colormap read matches colormap written */
char it;
{
	char z;

	for (z = 0; z < it; ++z) {
		if (rmap2[z] != rmap1[z] || simulate_error == RED_FAILED) {
                  errors++;
                  if (atn) send_msg_to_atn (ERROR, "Colormap error - red, loc = %d, exp = %d, actual = %d, pass %d, errors %d.", z, rmap1[z], rmap2[z], pass, errors);
                  throwup(-RED_FAILED, "ERROR: %s, Colormap error - red, loc = %d, exp = %d, actual = %d,%s errors %d.", TEST_NAME, z, rmap1[z], rmap2[z], print_pass()? tmp_msg:"", errors);
		}
			
		if (gmap2[z] != gmap1[z] || simulate_error == GREEN_FAILED) {
                  errors++;
                  if (atn) send_msg_to_atn (ERROR, "Colormap error - green, loc = %d, exp = %d, actual = %d, pass %d, errors %d.", z, gmap1[z], gmap2[z], pass, errors);
                  throwup(-GREEN_FAILED, "ERROR: %s, Colormap error - green, loc = %d, exp = %d, actual = %d,%s errors %d.", TEST_NAME, z, gmap1[z], gmap2[z], print_pass()? tmp_msg:"", errors);
		}

		if (bmap2[z] != bmap1[z] || simulate_error == BLUE_FAILED) {
                  errors++;
                  if (atn) send_msg_to_atn (ERROR, "Colormap error - blue, loc = %d, exp = %d, actual = %d, pass %d, errors %d.", z, bmap1[z], bmap2[z], pass, errors);
                  throwup(-BLUE_FAILED, "ERROR: %s, Colormap error - blue, loc = %d, exp = %d, actual = %d,%s errors %d.", TEST_NAME, z, bmap1[z], bmap2[z], print_pass()? tmp_msg:"", errors);
		}
	}
}

pix_chka(c,x,y,tx,ty,n)		/* verify locations in the frame buffer read as written */
int c,x,y,tx,ty,n;
{
	int a,b;

  for (; x < (tx - 50); x += n) {
     for (b = y; b < (ty - 50); b += n) {
        a = pr_get(screen, x, b);	
	if (c != a || simulate_error == FRAME_BUFFER_ERROR) {
           errors++;
           if (atn) send_msg_to_atn (ERROR, "Frame buffer failure, x pos = %d, y pos = %d, exp = 0x%x, actual = 0x%x, pass %d, errors %d.", x, b, c, a, pass, errors);
           throwup(-FRAME_BUFFER_ERROR, "ERROR: %s, Frame buffer failure, x pos = %d, y pos = %d, exp = 0x%x, actual = 0x%x,%s errors %d.", TEST_NAME, x, b, c, a, print_pass()? tmp_msg:"", errors);
	}
     }
  }
}

struct color *hsv_rgb(hx,s,v)	/* convert hue, saturation and value to red, green and blue */
float s, v;
int hx;
{
	float h, f, q, p, t;
	int i;
	struct color *col = &clr;

	h = hx;
	if (h > 359) {
		h = h - 360;
	}
	else if (h < 0) {
		h = 360 + h;
	}
	h = h / 60;
	i = h;
	f = h -i;
	p = v * (1 - s);
	q = v * (1 - (s * f));
	t = v * (1 - (s * (1 - f)));
	switch(i) {
	case 0:
		col->red = cnvrt(v);
		col->grn = cnvrt(t);
		col->blu = cnvrt(p);
		return(col);
	case 1:
		col->red = cnvrt(q);
		col->grn = cnvrt(v);
		col->blu = cnvrt(p);
		return(col);
	case 2:
		col->red = cnvrt(p);
		col->grn = cnvrt(v);
		col->blu = cnvrt(t);
		return(col);
	case 3:
		col->red = cnvrt(p);
		col->grn = cnvrt(q);
		col->blu = cnvrt(v);
		return(col);
	case 4:
		col->red = cnvrt(t);
		col->grn = cnvrt(p);
		col->blu = cnvrt(v);
		return(col);
	case 5:
		col->red = cnvrt(v);
		col->grn = cnvrt(p);
		col->blu = cnvrt(q);
		return(col);
	}
}

/*
		Note:   Not a standard "throwup" routine.
*/

throwup(where, fmt, a, b, c, d, e, f, g, h, i)
int     where;
char    *fmt;
u_long  a, b, c, d, e, f, g, h, i;
{
  char *attempt_log_msg = "Was attempting to log the following message:\n";
  extern char   *mktemp();
  int           clock;
  char          fmt_msg_buffer[200];
  char          *fmt_msg = fmt_msg_buffer;

  clock = time(0);
  sprintf(fmt_msg, fmt, a, b, c, d, e, f, g, h, i);

  if (!logfd){
     if (simulate_error == NO_OPEN_LOG) {
        strcpy(logfile, "not/valid/log");
     }   
     else {
        if (getenv("SD_LOG_DIRECTORY") && simulate_error != NO_SD_LOG_DIR) {
           strcpy(logfile, (getenv("SD_LOG_DIRECTORY")));
           strcat(logfile, "/");
           strcat(logfile, LOGFILE_NAME);
        }
        else {
           sprintf(msg, "No log file environmental variable.");
           if (atn) send_msg_to_atn(FATAL, msg);
           fprintf(stderr, "%s: %s\n", TEST_NAME, msg);
           fprintf(stderr, "%s: %s", TEST_NAME, attempt_log_msg);
           fprintf(stderr, "%s %s", fmt_msg, ctime(&clock));
           exit(NO_SD_LOG_DIR);

/*** not standard ***/
           if (clean_up) {
             /* Grabio has hidden semantic of reloading colormap */
             win_grabio(gfx_fd);
             /* Release io lock */
             win_releaseio(gfx_fd);
           }
/*** not standard ***/

        }
     }
     if ((logfd = open(mktemp(logfile),O_WRONLY|O_CREAT|O_APPEND ,0644)) <0){
        perror(perror_msg);
        sprintf(msg, "Couldn't open logfile '%s'.", logfile);
        if (atn) send_msg_to_atn(FATAL, msg);
        fprintf(stderr, "%s: %s\n", TEST_NAME, msg);
        fprintf(stderr, "%s: %s", TEST_NAME, attempt_log_msg);
        fprintf(stderr, "%s %s", fmt_msg, ctime(&clock));
   
/*** not standard ***/
        if (clean_up) {
          /* Grabio has hidden semantic of reloading colormap */
          win_grabio(gfx_fd);
          /* Release io lock */
          win_releaseio(gfx_fd);
        }
/*** not standard ***/

        exit(NO_OPEN_LOG);
     }
     else dup2(logfd, 2);               /* set logfile as stderr */
  }   
  fprintf(stderr, "%s %s", fmt_msg, ctime(&clock));
  printf("%s %s", fmt_msg, ctime(&clock));

  fflush(stderr);                          
  fsync(2);

/*** not standard ***/
  if (where < 0) {
     if (clean_up) {
       /* Grabio has hidden semantic of reloading colormap */
       win_grabio(gfx_fd);
       /* Release io lock */
       win_releaseio(gfx_fd);
     }   
     exit(-where);
  }
/*** not standard ***/

  failed = where;
}

print_pass()
{      
   if (atn) { 
      sprintf(tmp_msg, " pass %d,", pass);
      return TRUE;   
   }                 
   else return FALSE;
}      

finish()
{
   if (atn || verbose) {
      if (clean_up) {
        /* Grabio has hidden semantic of reloading colormap */
        win_grabio(gfx_fd);
        /* Release io lock */
        win_releaseio(gfx_fd);
      }
      sprintf(msg, "Stopped, pass %d, errors %d.", pass, errors);
      throwup(0, "%s: %s", TEST_NAME, msg);
      if (atn ) {
         send_msg_to_atn(INFO, msg);
         exit(0);
      }
   }
   exit(20);
}

#ifdef ATN_VERSION   
#include "atnrtns.c"    /* ATN routines */
#else    
send_msg_to_atn()
{             
printf("This is not the ATN version!\n");
}             
#endif 
