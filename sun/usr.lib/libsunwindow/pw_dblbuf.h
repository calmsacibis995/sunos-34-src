
/*      @(#)pw_dblbuf.h 1.3 87/01/09 SMI      */

#ifndef	KERNEL
#include <sys/types.h>
#include <sunwindow/attr.h>
#else	KERNEL
#include "../h/types.h"
#include "../sunwindow/attr.h"
#endif	KERNEL

typedef int		Pw_attribute_value;

#define	PIXWIN_ATTR(type, ordinal)	ATTR(ATTR_PKG_PIXWIN, type, ordinal)

typedef enum {

	/* integer attributes */
	PW_DBL_AVAIL		= PIXWIN_ATTR(ATTR_BOOLEAN, 20),/* DBL Available      */
	PW_DBL_DISPLAY		= PIXWIN_ATTR(ATTR_INT, 21), 	/* Display control bit*/
	PW_DBL_WRITE	 	= PIXWIN_ATTR(ATTR_INT, 22),	/* Write control bits */
	PW_DBL_READ		= PIXWIN_ATTR(ATTR_INT, 23),	/* Read Control bits  */
} Pw_dbl_attribute;


#ifndef	KERNEL
extern int pw_dbl_get();
extern int pw_dbl_set();
#endif	KERNEL

#define PW_DBL_ERROR	-1	/* Error while trying to set the dbl control bits */
#define PW_DBL_FORE	2	/* Set a control bit to the foreground */
#define PW_DBL_BACK	3	/* Set a control bit to the background */
#define PW_DBL_BOTH	4	/* Set a control bit to both backgr and the foregr */


