#ifndef lint
static char sccsid[] = "@(#)segdraw.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "coretypes.h"
#include "corevars.h"

static short sdstr[64];
/****************************************************************************/
_core_segdraw(asegptr,anindex,rastererase)
register segstruc *asegptr;
int anindex, rastererase;
{			/* paint the segment on the viewsurface */
    viewsurf *sptr;			/* ptr to current viewsurface   */
    register int (*surfdd)();
    register ddargtype *ddptr;		/* ptr to ddstruct		*/
    ddargtype ddstruct;
    int offset;				/* byte offset in PDF */
    short sdopcode, n, i;
    char *sdstring;
    ipt_type isp, iup, ipath;		/* direction vectors for characters */
    ipt_type p0, p1, ip1, ip2, ip3, ip4;/* NDC point variables */
    rast_type raster;			/* bitmap structure */
    short *ptr;
    int size, mk, vcount;
    int extents[4];
    porttype pdfvwport;			/* viewport used for output clipping */
    primattr pdfcurrent;
    float f;

    sptr = asegptr->vsurfptr[anindex];
    if (!(asegptr->segats.visbilty))
       return(0);
    _core_critflag++;
    ddptr = &ddstruct;
    ddstruct.instance = sptr->vsurf.instance;
    surfdd = sptr->vsurf.dd;
    sdstring = (char*)sdstr;	

    offset = asegptr->pdfptr;
    _core_pdfseek( offset, 0, &ptr);
    vcount = 0;				/* lock every n opcodes */
    ddstruct.opcode = LOCK;
    (*surfdd)(ddptr);
    while (TRUE) {			/* read PDF output to DD */
       _core_pdfread(SHORT,&sdopcode);
       if (vcount++ > 15){
		vcount = 0;
		ddstruct.opcode = UNLOCK;
		(*surfdd)(ddptr);
		ddstruct.opcode = LOCK;
		(*surfdd)(ddptr);
		}

       switch (sdopcode) {
	  case PDFMOVE:
	     _core_pdfread(FLOAT*3, &ip1);
	     /* perform an image transformation before sending to DD */
   	     if(! asegptr->idenflag) {
	        if(asegptr->type == XLATE2) {	/* PERFORM A TRANSLATE ONLY. */
		    ip1.x += asegptr->imxform[3][0];
		    ip1.y += asegptr->imxform[3][1];
		    ip1.z += asegptr->imxform[3][2];
	        }
                else if(asegptr->type == XFORM2){/* ROTATE,SCALE, TRANSLATE */
		   p1 = ip1;
		   _core_imxfrm3( asegptr, &p1, &ip1);
	        }
	     }
	     ddptr->opcode = MOVE;
	     ddptr->int1 = ip1.x;
	     ddptr->int2 = ip1.y;
	     (*surfdd)(ddptr);
	    break;
	  case PDFLINE:
	     _core_pdfread(FLOAT*3, &ip2);
	     /* perform an image transformation before sending to DD */
   	     if(! asegptr->idenflag) {
	        if(asegptr->type == XLATE2) {	/* PERFORM A TRANSLATE ONLY. */
		       ip2.x += asegptr->imxform[3][0];
		       ip2.y += asegptr->imxform[3][1];
		       ip2.z += asegptr->imxform[3][2];
		} else if(asegptr->type == XFORM2){/* ROTATE,SCALE, TRANSLATE */
		   p1 = ip2;
		   _core_imxfrm3( asegptr, &p1, &ip2);
	           }
	     }
	     p0 = ip1;
	     p1 = ip2;
	     if(_core_outpclip) {
		if ( !_core_oclpvec2(&p0,&p1,&pdfvwport)) {
	            ip1 = ip2;
		    break;			/* if not visible */
		    }
		else if (p0.x != ip1.x || p0.y != ip1.y) {
		    ddptr->opcode = MOVE;
		    ddptr->int1 = p0.x;
		    ddptr->int2 = p0.y;
		    (*surfdd)(ddptr);
		    }
		}
	     ddptr->opcode = LINE;
	     ddptr->int1 = p1.x;
	     ddptr->int2 = p1.y;
	     (*surfdd)(ddptr);
	     ip1 = ip2;
	     break;
	  case PDFTEXT:
	     _core_pdfread(SHORT,&n); i = (n & 1) ? n+1 : n;
	     _core_pdfread(i,sdstr);
	     *(sdstring + n) = '\0';
	     if ((pdfcurrent.chqualty == STRING) && (sptr->txhardwr)) {
		ddptr->opcode = TEXT;
		ddptr->ptr1 = sdstring;
		ddptr->ptr2 = (char*)(&pdfvwport);
		ddptr->int1 = pdfcurrent.font;
		ddptr->int2 = 0;		/* string position from cp */
		ddptr->logical = TRUE;
		(*surfdd)(ddptr);
		}
	     else {		/* use vector font text */
		ddptr->opcode = VTEXT;
		ddptr->ptr1 = sdstring;
		ddptr->ptr2 = (char*)(&pdfvwport);
		(*surfdd)(ddptr);
		}
	     break;
	  case PDFMARKER:
	     _core_pdfread(FLOAT,&mk);
	     pdfcurrent.marker = mk;
	     *sdstring = mk;  *(sdstring+1) = '\0';
	     if (_core_outpclip) {
		 if ( !_core_oclippt2( ip1.x, ip1.y, &pdfvwport)) break;
		 }
	     ddptr->opcode = MARKER;
	     ddptr->ptr1 = sdstring;
	     ddptr->ptr2 = (char*)(&pdfvwport);
	     (*surfdd)(ddptr);
	     break;
	  case PDFBITMAP:
	     _core_pdfread(FLOAT,&raster.width);
	     _core_pdfread(FLOAT,&raster.height);
	     _core_pdfread(FLOAT,&raster.depth);
	     if (raster.depth == 1)
	         size = ((raster.width+15)>>4)*raster.height*2;
	     else if (raster.depth == 8)
		 size = raster.width*raster.height;
	     else size = raster.width*raster.height*3;
	     _core_pdfseek(0, 1, &raster.bits);
		 ip2.x = pdfvwport.xmin; ip2.y = pdfvwport.ymin;
		 ip4.x = pdfvwport.xmax; ip4.y = pdfvwport.ymax;
		 ddptr->opcode = RASPUT;
		 ddptr->logical = TRUE;
		 ddptr->ptr1 = (char*)(&raster);
		 ddptr->ptr2 = (char*)(&ip2);	/* lower left viewport */
		 ddptr->ptr3 = (char*)(&ip4);	/* upper right viewport */
		 (*surfdd)(ddptr);
	     _core_pdfskip( size); 
	     break;
	  case PDFPOL2: 			/* plot 2-D polygon */
	    _core_pdfread( SHORT, &n);
	    for (i=0; i<n; i++) {
	       _core_pdfread( FLOAT*4, &_core_vtxlist[i]);
	       }
	    /*----- image transform the polygon ------------*/
	    if(! asegptr->idenflag) {
	        if(asegptr->type == XLATE2) {		/* translate only */
		    for (i=0; i<n; i++) {
			_core_vtxlist[i].x += asegptr->imxform[3][0];
			_core_vtxlist[i].y += asegptr->imxform[3][1];
			}
		} else if(asegptr->type == XFORM2)
		    for (i=0; i<n; i++) {		/* full transform */
		        ip2.x = _core_vtxlist[i].x;
		        ip2.y = _core_vtxlist[i].y;
		        ip2.z = _core_vtxlist[i].z;
		        ip2.w = _core_vtxlist[i].w;
		        _core_imxfrm2( asegptr, &ip2, &_core_vtxlist[i]);
			}
	        }
	    if (_core_outpclip) {
	       _core_vtxcount = n;
	       for (i=0; i<n; i++) {
						/* clip to view port  */
		    _core_oclpvtx2( &_core_vtxlist[i], &pdfvwport);
		    }
	       _core_oclpend2();
	       if (_core_vtxcount > n) {
	          ddptr->opcode = POLYGN2;	/* output polygon to DD */
	          ddptr->int1 = _core_vtxcount - n;
	          ddptr->ptr1 = (char*)(&_core_vtxlist[n]);
	          ddptr->ptr2 = (char*)(&_core_ddvtxlist[n]);
	          (*surfdd)(ddptr);
		  }
	       }
	    else {
	       ddptr->opcode = POLYGN2;		/* output polygon to DD */
	       ddptr->int1 = n;
	       ddptr->ptr1 = (char*)(&_core_vtxlist[0]);
	       ddptr->ptr2 = (char*)(&_core_ddvtxlist[0]);
	       (*surfdd)(ddptr);
	       }
	    break;
	  case PDFPOL3: 			/* plot 3-D polygon */
	    _core_pdfread( SHORT, &n);
	    for (i=0; i<n; i++) {
	       _core_pdfread( FLOAT*8, &_core_vtxlist[i]);
	       }
	    /*----- image transform the polygon ------------*/
	    if(! asegptr->idenflag) {
	        if(asegptr->type == XLATE2) {		/* translate only */
		    for (i=0; i<n; i++) {
			_core_vtxlist[i].x += asegptr->imxform[3][0];
			_core_vtxlist[i].y += asegptr->imxform[3][1];
			_core_vtxlist[i].z += asegptr->imxform[3][2];
		        }
		    }
		else if(asegptr->type == XFORM2)
		    for (i=0; i<n; i++) {		/* full transform */
		       ip2.x = _core_vtxlist[i].x;
		       ip2.y = _core_vtxlist[i].y;
		       ip2.z = _core_vtxlist[i].z;
		       ip2.w = _core_vtxlist[i].w;
		       _core_imxfrm3( asegptr, &ip2, &_core_vtxlist[i]);
		       ip2.x = _core_vtxlist[i].dx;
		       ip2.y = _core_vtxlist[i].dy;
		       ip2.z = _core_vtxlist[i].dz;
		       ip2.w = _core_vtxlist[i].dw;
		       _core_imszpt3( asegptr, &ip2, &_core_vtxlist[i].dx);
		       }
	        }
	    if (_core_outpclip) {
	       _core_vtxcount = n;
	       for (i=0; i<n; i++) {
						/* clip to view port  */
		    _core_oclpvtx2( &_core_vtxlist[i], &pdfvwport);
		    }
	       _core_oclpend2();
	       if (_core_vtxcount > n) {
	          ddptr->opcode = POLYGN3;	/* output polygon to DD */
	          ddptr->int1 = _core_vtxcount - n;
	          ddptr->int3 = sptr->hiddenon;
	          ddptr->ptr1 = (char*)(&_core_vtxlist[n]);
	          ddptr->ptr2 = (char*)(&_core_ddvtxlist[n]);
	          (*surfdd)(ddptr);
		  }
	       }
	    else {
	       ddptr->opcode = POLYGN3;		/* output polygon to DD */
	       ddptr->int1 = n;
	       ddptr->int3 = sptr->hiddenon;
	       ddptr->ptr1 = (char*)(&_core_vtxlist[0]);
	       ddptr->ptr2 = (char*)(&_core_ddvtxlist[0]);
	       (*surfdd)(ddptr);
	       }
	    break;
	  case PDFLCOLOR:
	    _core_pdfread(FLOAT, &(pdfcurrent.lineindx));
	    ddptr->opcode = SETLCOL;
	    ddptr->int1 = pdfcurrent.lineindx;
    	    ddptr->int2 = pdfcurrent.rasterop;
	    ddptr->int3 = rastererase;
	    (*surfdd)(ddptr);
	    break;
	  case PDFFCOLOR:
	    _core_pdfread(FLOAT, &(pdfcurrent.fillindx));
	    ddptr->opcode = SETFCOL;
	    ddptr->int1 = pdfcurrent.fillindx;
    	    ddptr->int2 = pdfcurrent.rasterop;
	    ddptr->int3 = rastererase;
	    (*surfdd)(ddptr);
	    break;
	  case PDFTCOLOR:
	    _core_pdfread(FLOAT, &(pdfcurrent.textindx));
	    ddptr->opcode = SETTCOL;
	    ddptr->int1 = pdfcurrent.textindx;
    	    ddptr->int2 = pdfcurrent.rasterop;
	    ddptr->int3 = rastererase;
	    (*surfdd)(ddptr);
	    break;
	  case PDFLINESTYLE:
	    _core_pdfread(FLOAT,&(pdfcurrent.linestyl));
	    if (sptr->lshardwr) {
	       ddptr->opcode = SETSTYL;
	       ddptr->int1 = pdfcurrent.linestyl;
	       (*surfdd)(ddptr);
	       }
	    break;
	  case PDFPISTYLE:
	    _core_pdfread(FLOAT,&(pdfcurrent.polyintstyl));
	    break;
	  case PDFPESTYLE:
	    _core_pdfread(FLOAT,&(pdfcurrent.polyedgstyl));
	    break;
	  case PDFLINEWIDTH:
	    _core_pdfread(FLOAT,&(pdfcurrent.linwidth));
	    if (sptr->lwhardwr) {
	       ddptr->opcode = SETWIDTH;
	       f = (float)((_core_ndcspace[0]<_core_ndcspace[1])?
	       _core_ndcspace[0] : _core_ndcspace[1]);
	       ddstruct.int1 = pdfcurrent.linwidth * f / 100.;
	       (*surfdd)(ddptr);
	       }
	    break;
	  case PDFFONT:
	    _core_pdfread(FLOAT,&(pdfcurrent.font));
	    ddptr->opcode = SETFONT;
	    ddptr->int1 = pdfcurrent.font;
	    (*surfdd)(ddptr);
	    break;
	  case PDFPEN:
	    _core_pdfread(FLOAT,&(pdfcurrent.pen));
	    ddptr->opcode = SETPEN;
	    ddptr->int1 = pdfcurrent.pen;
	    (*surfdd)(ddptr);
	    break;
	  case PDFSPACE:
	    _core_pdfread(FLOAT*3, &pdfcurrent.chrspace);
	    				/* image transform spacing vector */
   	    if( !asegptr->idenflag) {
	    	_core_imszpt3( asegptr, &pdfcurrent.chrspace, &isp);
	        ddptr->ptr1 = (char*)(&isp);
		}
	    else
	        ddptr->ptr1 = (char*)(&pdfcurrent.chrspace);
	    ddptr->opcode = SETSPACE;
	    (*surfdd)(ddptr);
	    break;
	  case PDFPATH:
	    _core_pdfread(FLOAT*3, &pdfcurrent.chrpath);
	    				/* image transform path vector */
   	    if( !asegptr->idenflag) {
	    	_core_imszpt3( asegptr, &pdfcurrent.chrpath, &ipath);
	        ddptr->ptr1 = (char*)(&ipath);
		}
	    else
	        ddptr->ptr1 = (char*)(&pdfcurrent.chrpath);
	    ddptr->opcode = SETPATH;
	    (*surfdd)(ddptr);
	    break;
	  case PDFUP:
	    _core_pdfread(FLOAT*3, &pdfcurrent.chrup.x);
					/* image transform up vector */
   	    if( !asegptr->idenflag) {
	    	_core_imszpt3( asegptr, &pdfcurrent.chrup, &iup);
	        ddptr->ptr1 = (char*)(&iup);
		}
	    else
	        ddptr->ptr1 = (char*)(&pdfcurrent.chrup);
	    ddptr->opcode = SETUP;
	    (*surfdd)(ddptr);
	    break;
	  case PDFCHARQUALITY:
	    _core_pdfread(FLOAT,&(pdfcurrent.chqualty));
	    break;
	  case PDFCHARJUST:
	    _core_pdfread(FLOAT,&(pdfcurrent.chjust));
	    break;
	  case PDFROP:
	    _core_pdfread(FLOAT,&(pdfcurrent.rasterop));
	    if (_core_xorflag) pdfcurrent.rasterop = XORROP;
	    break;
	  case PDFPICKID:
	    _core_pdfread(FLOAT, &(pdfcurrent.pickid));
	    _core_pdfread(SHORT, &n);
	    _core_pdfskip(FLOAT*n*3);
	    break;
	  case PDFVWPORT:		/* get viewport from PDF */
	    _core_pdfread( FLOAT*6, &pdfvwport);
	    break;
	  case PDFENDSEGMENT:
	    goto done;
	  default: break;
	  }
    }
    done:

    ddstruct.opcode = UNLOCK;
    (*surfdd)(ddptr);
    if (_core_openseg) {
	_core_cpchang = TRUE;
	_core_linatt = TRUE;
	_core_polatt = TRUE;
	_core_texatt = TRUE;
	_core_rasatt = TRUE;
	}
    if (--_core_critflag == 0 && _core_updatewin && _core_sighandle)
	(*_core_sighandle)();
    return(0);
}

