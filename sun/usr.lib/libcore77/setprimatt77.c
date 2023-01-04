#ifndef lint
static char sccsid[] = "@(#)setprimatt77.c 1.1 86/09/25 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

int define_color_indices();
int set_charjust();
int set_charpath_2();
int set_charpath_3();
int set_charprecision();
int set_charsize();
int set_charspace();
int set_charup_2();
int set_charup_3();
int set_drag();
int set_fill_index();
int set_font();
int set_line_index();
int set_linestyle();
int set_linewidth();
int set_marker_symbol();
int set_pen();
int set_pick_id();
int set_polygon_edge_style();
int set_polygon_interior_style();
int set_primitive_attributes();
int set_rasterop();
int set_text_index();

#define DEVNAMESIZE 20

struct vwsurf	{
		char screenname[DEVNAMESIZE];
		char windowname[DEVNAMESIZE];
		int windowfd;
		int (*dd)();
		int instance;
		int cmapsize;
		char cmapname[DEVNAMESIZE];
		int flags;
		char **ptr;
		};

int defcolorindices_(surf, i1, i2, red, grn, blu)
struct vwsurf *surf;
int *i1, *i2;
float red[], grn[], blu[];
	{
	return(define_color_indices(surf, *i1, *i2, red, grn, blu));
	}

int setcharjust_(chjust)
int *chjust;
	{
	return(set_charjust(*chjust));
	}

int setcharpath2_(dx, dy)
float *dx, *dy;
	{
	return(set_charpath_2(*dx, *dy));
	}

int setcharpath3_(dx, dy, dz)
float *dx, *dy, *dz;
	{
	return(set_charpath_3(*dx, *dy, *dz));
	}

int setcharprecision(chqualty)
int *chqualty;
	{
	return(set_charprecision(*chqualty));
	}

int setcharsize_(chwidth, cheight)
float *chwidth, *cheight;
	{
	return(set_charsize(*chwidth, *cheight));
	}

int setcharspace_(space)
float *space;
	{
	return(set_charspace(*space));
	}

int setcharup2_(dx, dy)
float *dx, *dy;
	{
	return(set_charup_2(*dx, *dy));
	}

int setcharup3_(dx, dy, dz)
float *dx, *dy, *dz;
	{
	return(set_charup_3(*dx, *dy, *dz));
	}

int setdrag_(drag)
int *drag;
	{
	return(set_drag(*drag));
	}

int setfillindex_(color)
int *color;
	{
	return(set_fill_index(*color));
	}

int setfont_(font)
int *font;
	{
	return(set_font(*font));
	}

int setlineindex_(color)
int *color;
	{
	return(set_line_index(*color));
	}

int setlinestyle_(linestyl)
int *linestyl;
	{
	return(set_linestyle(*linestyl));
	}

int setlinewidth_(linwidth)
float *linwidth;
	{
	return(set_linewidth(*linwidth));
	}

int setmarkersymbol_(mark)
int *mark;
	{
	return(set_marker_symbol(*mark));
	}

int setpen_(pen)
int *pen;
	{
	return(set_pen(*pen));
	}

int setpickid_(pickid)
int *pickid;
	{
	return(set_pick_id(*pickid));
	}

int setpolyedgestyle(polyedgstyl)
int *polyedgstyl;
	{
	return(set_polygon_edge_style(*polyedgstyl));
	}

int setpolyintrstyle(polyintstyl)
int *polyintstyl;
	{
	return(set_polygon_interior_style(*polyintstyl));
	}

typedef struct { float x,y,z,w;} pt_type;
typedef struct { float width, height; }aspect;

typedef struct {		/* primitive attribute list structure */
   int lineindx;
   int fillindx;
   int textindx;
   int linestyl;
   int polyintstyl;
   int polyedgstyl;
   float linwidth;
   int pen;
   int font;
   aspect charsize;
   pt_type chrup, chrpath, chrspace;
   int chjust;
   int chqualty;
   int marker;
   int pickid;
   int rasterop;
   } primattr;

int setprimattribs_(defprim)
primattr *defprim;
	{
	return(set_primitive_attributes(defprim));
	}

int setrasterop_(flag)
int *flag;
	{
	return(set_rasterop(*flag));
	}

int settextindex_(color)
int *color;
	{
	return(set_text_index(*color));
	}

