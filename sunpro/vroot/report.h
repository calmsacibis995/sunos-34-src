/*LINTLIBRARY*/
/*	@(#)report.h 1.2 87/03/17 SMI	*/

/*
 *	Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <stdio.h>

extern FILE	*get_report_file();
extern char	*get_target_being_reported_for();
extern void	report_dependency();

#define SUNPRO_DEPENDENCIES "SUNPRO_DEPENDENCIES"
