/*	@(#)colorshader.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */
/*
 *	This file contains the include stuff for 3D polygon shading
 */
typedef struct {    /* Shading parameters, all fixed pt with 15 bit fraction */
   int lx, ly, lz;		/* light direction vector */
   int ex, ey, ez;		/* eye direction vector */
   int hx, hy, hz;		/* hilite max direction vector */
   int ambient;			/* amount of ambient light 0.0...255. */
   int diffuseamt;		/* coef of diffuse light 0.0...1.0 */
   int specularamt;		/* coef of specular reflection 0.0...1.0 */
   int floodamt;		/* coef of flood light from viewer 0.0...1.0 */
   int bump;			/* width of specular bump 0 or 1.0 */
   int transparent;		/* coef of transparency, 0 is opaque */
   int hue;			/* 0,1,2,3 = grey range 0..255,1..63,64..127..*/
   int frenel;			/* coef for Torrance-Sparrow hilites */
   int shadestyle;		/* 0,1 = interpolate int,normals  */
   } shading_parameter_structure;

shading_parameter_structure _core_shading_parameters;

#define zbvisible( zb, zc, z)		((z)<(zb) && (z)>(zc))
