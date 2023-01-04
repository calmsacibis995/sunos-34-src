/*      @(#)libmdefs.h 1.1 86/09/25 SMI      */
 
/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <math.h>

#define zero 0 
#define subnormal 1 
#define normal 2 
#define inf 3 
#define qnan 4 
#define snan 5 
#define nansqrt 1 
#define nanzero 21 
#define nantrig 33
#define naninvtrig 34
#define nanlog  36 
#define nanpower 37

int d_integral() ; 
int d_powexp() ; 
double d_intfrac() ;
int dclass() ;
int dminus() ;
double dinf() ;
double dnan() ;
double dquietnan() ;
double dtrigarg() ;
double FFscaled_( /* double *x, int *n */ ) ; 

/*	Following glue routines help implement System V Interface for -fsoft */

extern double _vexpd(), _vlogd(), _vlog10d(), _vpowd(), _vsqrtd() ;
extern double _vlength2d(), _vsinhd(), _vcoshd() ;
extern double _vsind(), _vcosd(), _vtand();
extern double _vasind(), _vacosd(), _vatand(), _vatan2d() ;
