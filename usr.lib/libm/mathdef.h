#ifndef lint 
static  char mathdefid[] = "@(#)mathdef.h 1.1 86/09/25 SMI"; 
#endif
 

extern double fabs(), floor(), ceil(), fmod(), ldexp(), frexp();
extern double sqrt(), hypot(), atof();
extern double sin(), cos(), tan(), asin(), acos(), atan(), atan2();
extern double exp(), log(), log10(), pow();
extern double sinh(), cosh(), tanh();
extern double gamma();
extern double j0(), j1(), jn(), y0(), y1(), yn();
extern int d_integral() ; int d_powexp() ; double d_intfrac() ;

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

int dclass() ;
int dminus() ;
double dinf() ;
double dnan() ;
double dquietnan() ;
double dtrigarg() ;

