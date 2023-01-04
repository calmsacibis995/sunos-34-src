
#ifndef lint
static	char sccsid[] = "@(#)controller.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "setup_runtime.h"


static	void	controller_set();
static	caddr_t	controller_get();
static	void	controller_destroy();


Controller 
controller_create(object)
Setup_object	*object;
{
	Controller_info		*controller;

	object->data		= (caddr_t) new(Controller_info);
	object->type		= SETUP_CONTROLLER;
	object->set_attr	= controller_set;
	object->get_attr	= controller_get;
	object->destroy		= controller_destroy;
	
	controller 		= SETUP_GET_DATA(object, Controller_info);
	controller->name	= "";
	controller->num_disks	= 0;
}


static void
controller_set(object, avlist)
Setup_object	*object;
Avlist		 avlist;
{
	Controller_info		*controller;
	Setup_attribute		attr;
	char			*ptr;
	int			n;

	controller = SETUP_GET_DATA(object, Controller_info);
	
	while(attr = (Setup_attribute) *avlist++) {
		switch (attr) {
		case CONTROLLER_NAME:
			ptr = (char *) *avlist++;
			controller->name = (char *) strdup(ptr);
			break;

		case CONTROLLER_TYPE:
		    	controller->type = (Setup_setting) *avlist++;
		    	break;

		case CONTROLLER_DISK:
			n = (int) *avlist++;
			if (n == SETUP_APPEND) {
				n = controller->num_disks++;
			}
			controller->disks[n] = (Disk) *avlist++;
			break;

		default:
			avlist = attr_skip(attr, avlist);
			break;

		}
	}
}


static caddr_t
controller_get(object, attr, op1, op2)
Setup_object		*object;
Setup_attribute		attr;
caddr_t			op1;
caddr_t			op2;
{
	Controller_info		*controller;
	
	controller = SETUP_GET_DATA(object, Controller_info);
	
	switch (attr) {
	case CONTROLLER_NAME:
		return ((caddr_t) controller->name);

	case CONTROLLER_TYPE:
		return ((caddr_t) controller->type);

	case CONTROLLER_NUM_DISKS:
		return ((caddr_t) controller->num_disks);

	case CONTROLLER_DISK:
		if ((int)op1 >= controller->num_disks)
			return ((caddr_t) 0);
		else
			return ((caddr_t) controller->disks[(int)op1]);

	default:
                runtime_error("Unknown controller attribute.");
                break;

	}
}

static void
controller_destroy(object)
Setup_object	*object;
{
}
