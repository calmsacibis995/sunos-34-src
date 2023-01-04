#ifndef lint
static	char sccsid[] = "@(#)software.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "setup_runtime.h"
 
static	void	oswg_set();
static	caddr_t	oswg_get();
static	void	oswg_destroy();


Oswg 
oswg_create(obj)
Setup_object	*obj;
{   
	register Oswg_info	*oswg;
	
	obj->data = (caddr_t) new(Oswg_info);
	obj->type = SETUP_OSWG;
	obj->set_attr = oswg_set;
	obj->get_attr = oswg_get;
	obj->destroy  = oswg_destroy;
	
	((Oswg_info *)(obj->data))->name = strdup("");
}
	

static
void
oswg_set(obj, avlist)
Setup_object	*obj;
Avlist		avlist;
{
	Oswg_info	*oswg;
	Setup_attribute	attr;
	char		*ptr;
	int		n;
	
	oswg = SETUP_GET_DATA(obj, Oswg_info);
	
	while(attr = (Setup_attribute) *avlist++) {
		switch (attr) {

		case OSWG_NAME:
			ptr = (char *) *avlist++;
		        oswg->name = strdup(ptr);
			break;

		case OSWG_USRSIZE:
			n = (int)*avlist++;
			oswg->usrsize[n] = (int)*avlist++;
			break;

		case OSWG_DIRECTORY:
			n = (int)*avlist++;
			ptr = *avlist++;
			oswg->dirs[n] = strdup(ptr);
			break;

		default:
			avlist = attr_skip(attr, avlist);
			break;
		
		}
	}
}


static
caddr_t
oswg_get(obj, attr, op1, op2)
Setup_object		*obj;
Setup_attribute		attr;
caddr_t			op1;
caddr_t			op2;
{
        Oswg_info       *oswg;
	Disk		disk;
	Controller	cont;
	Hard_partition	hp;
	
	oswg = SETUP_GET_DATA(obj, Oswg_info);
	
	switch (attr) {

	case OSWG_DESCRIPTION:
		cont = (Controller) setup_get(workstation, WS_CONTROLLER, 0);
		disk = (Disk) setup_get(cont, CONTROLLER_DISK, 0);
		hp = (Hard_partition) setup_get(disk, DISK_HARD_PARTITION, 0); 
		sprintf(scratch_buf, "%s (%s)",
		    oswg->name, sectors_to_units_left(hp, 
		    oswg->usrsize[(int)op1]));
		return ((caddr_t) scratch_buf);

	case OSWG_NAME:
		return ((caddr_t) oswg->name);
		 
	case OSWG_USRSIZE:
		return ((caddr_t) oswg->usrsize[(int)op1]);

	case OSWG_DIRECTORY:
		return ((caddr_t) oswg->dirs[(int)op1]);

	default:
		runtime_error("Unknown oswg attribute.");
		return;
	
	}
}


static
void
oswg_destroy(obj, avlist)
Setup_object	*obj;
Avlist		avlist;
{   
	Oswg_info		*oswg;
	
	oswg = SETUP_GET_DATA(obj, Oswg_info);
	
	free(oswg);
	free(obj);
}
