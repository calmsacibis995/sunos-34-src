/*	@(#)object.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */


#ifndef	OBJECT_INCLUDED

#define OBJECT_INCLUDED 1

typedef struct {
    void	(*set)();
    caddr_t	(*get)();
    void	(*destroy)();
    void	(*display)();
    int		(*process_input)();
    caddr_t	data;
} Object;


#endif OBJECT_INCLUDED
