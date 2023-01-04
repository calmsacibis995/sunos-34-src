#ifndef lint
static char sccsid[] = "@(#)inqprimatt77.c 1.1 86/09/25 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

int inquire_charjust();
int inquire_charpath_2();
int inquire_charpath_3();
int inquire_charprecision();
int inquire_charsize();
int inquire_charspace();
int inquire_charup_2();
int inquire_charup_3();
int inquire_color_indices();
int inquire_fill_index();
int inquire_font();
int inquire_line_index();
int inquire_linestyle();
int inquire_linewidth();
int inquire_marker_symbol();
int inquire_pen();
int inquire_pick_id();
int inquire_polygon_edge_style();
int inquire_polygon_interior_style();
int inquire_primitive_attributes();
int inquire_rasterop();
int inquire_text_index();

int inqcharjust_(chjust)
int *chjust;
	{
	return(inquire_charjust(chjust));
	}

int inqcharpath2_(dx, dy)
float *dx, *dy;
	{
	return(inquire_charpath_2(dx, dy));
	}

int inqcharpath3_(dx, dy, dz)
float *dx, *dy, *dz;
	{
	return(inquire_charpath_3(dx, dy, dz));
	}

int inqcharprecision(chqualty)
int *chqualty;
	{
	return(inquire_charprecision(chqualty));
	}

int inqcharsize_(chwidth, cheight)
float *chwidth, *cheight;
	{
	return(inquire_charsize(chwidth, cheight));
	}

int inqcharspace_(space)
float *space;
	{
	return(inquire_charspace(space));
	}

int inqcharup2_(dx, dy)
float *dx, *dy;
	{
	return(inquire_charup_2(dx, dy));
	}

int inqcharup3_(dx, dy, dz)
float *dx, *dy, *dz;
	{
	return(inquire_charup_3(dx, dy, dz));
	}

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

int inqcolorindices_(surf, i1, i2, red, grn, blu)
struct vwsurf *surf;
int *i1, *i2;
float red[], grn[], blu[];
	{
	return(inquire_color_indices(surf, *i1, *i2, red, grn, blu));
	}

int inqfillindex_(color)
int *color;
	{
	return(inquire_fill_index(color));
	}

int inqfont_(font)
int *font;
	{
	return(inquire_font(font));
	}

int inqlineindex_(color)
int *color;
	{
	return(inquire_line_index(color));
	}

int inqlinestyle_(linestyl)
int *linestyl;
	{
	return(inquire_linestyle(linestyl));
	}

int inqlinewidth_(linwidth)
float *linwidth;
	{
	return(inquire_linewidth(linwidth));
	}

int inqmarkersymbol_(mark)
int *mark;
	{
	return(inquire_marker_symbol(mark));
	}

int inqpen_(pen)
int *pen;
	{
	return(inquire_pen(pen));
	}

int inqpickid_(pickid)
int *pickid;
	{
	return(inquire_pick_id(pickid));
	}

int inqpolyedgestyle(polyedgstyl)
int *polyedgstyl;
	{
	return(inquire_polygon_edge_style(polyedgstyl));
	}

int inqpolyintrstyle(polyintstyl)
int *polyintstyl;
	{
	return(inquire_polygon_interior_style(polyintstyl));
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

int inqprimattribs_(defprim)
primattr *defprim;
	{
	return(inquire_primitive_attributes(defprim));
	}

int inqrasterop_(rasterop)
int *rasterop;
	{
	return(inquire_rasterop(rasterop));
	}

int inqtextindex_(color)
int *color;
	{
	return(inquire_text_index(color));
	}
