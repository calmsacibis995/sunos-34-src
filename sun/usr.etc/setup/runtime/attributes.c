
#ifndef lint
static  char sccsid[] = "@(#)attributes.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "setup_runtime.h"

static void		generic_set();

static Opaque_info	*find_opaque();
static void		null_callback();

/* VARARGS */
Opaque
setup_create(create_func, arg1)
void		(*create_func)();
Setup_attribute	arg1;
{
	Setup_attribute	a[ATTR_STANDARD_SIZE];
	Setup_object	*object;
	
	object = new(Setup_object);
		
	object->callback = null_callback;

	(*create_func)(object);	
	
	generic_set(object, attr_make(a, ATTR_STANDARD_SIZE, &arg1));
	
	return (Opaque) object;
}

/* VARARGS */
void
setup_set(object, arg1)
Setup_object	*object;
Setup_attribute	arg1;
{
	Setup_attribute	a[ATTR_STANDARD_SIZE];

	generic_set(object, attr_make(a, ATTR_STANDARD_SIZE, &arg1)); 

}

/* VARARGS */
void
privledged_setup_set(object, arg1)
Setup_object	*object;
Setup_attribute	arg1;
{
	Setup_attribute	a[ATTR_STANDARD_SIZE];
	
	privledged_on(object);

	generic_set(object, attr_make(a, ATTR_STANDARD_SIZE, &arg1)); 

	privledged_off(object);
}



static void
generic_set(object, avlist)
Setup_object	*object;
Avlist		avlist;
{
	Setup_attribute	attr;
	Opaque_info	*info;
	Avlist		orig_avlist = avlist;
	int		n;
	int		count = 0;
	
	while (attr = (Setup_attribute) *avlist++) {
		switch (attr) {
		case SETUP_CALLBACK:
			object->callback = (Callback) *avlist++;
			break;
			
		case SETUP_OPAQUE:
			attr	= (Setup_attribute) *avlist++;
			info	= find_opaque(object->opaque_info, attr);
			info->data	= *avlist++;
			break;

		case SETUP_NOTCHANGEABLE:
			n = (int) *avlist++;
			if (n) {
				SET_ATTR_BIT(object->status, attr);
			} else {
				CLR_ATTR_BIT(object->status, attr);
			}
			break;
			
		default:
			avlist = attr_skip(attr, avlist);
			count++;
			break;
		}
	}
	
	if (count) {
		(*(object->set_attr))(object, orig_avlist);
	}

}


/* VARARGS */
caddr_t
setup_get(object, attr, op1, op2)
Setup_object	*object;
Setup_attribute	attr;
caddr_t		op1;
caddr_t		op2;
{
	Opaque_info	*info;
	
	switch (attr) {
	case SETUP_STATUS:
		return (caddr_t)TEST_ATTR_BIT(object->status, 
		    (Setup_attribute)op1);
		break;

	case SETUP_CALLBACK:
		return (caddr_t) object->callback;
		break;
		
	case SETUP_OPAQUE:
		info = find_opaque(object->opaque_info, (Setup_attribute) op1);
		return (caddr_t) info->data;
		break;

	case SETUP_NOTCHANGEABLE:
		return ((caddr_t)TEST_ATTR_BIT(object->status, attr));
		break;
			
	default:
		return (*(object->get_attr))(object, attr, op1, op2);
		break;

	}

}


/* VARARGS */
caddr_t
privledged_setup_get(object, attr, op1, op2)
Setup_object    *object;
Setup_attribute attr;
caddr_t         op1;
caddr_t         op2;
{
        Opaque_info     *info;
	caddr_t		value;
        
        switch (attr) {
        case SETUP_STATUS:
                return (caddr_t)TEST_ATTR_BIT(object->status,
                    (Setup_attribute)op1);
                break;
 
        case SETUP_CALLBACK:
                return (caddr_t) object->callback;
                break;
                
        case SETUP_OPAQUE:
                info = find_opaque(object->opaque_info, (Setup_attribute) op1);
                return (caddr_t) info->data;
                break;
                         
        default:
		privledged_on(object);
                value = (*(object->get_attr))(object, attr, op1, op2);
		privledged_off(object);
		return (value);
                break;
 
        }
 
}


static Opaque_info *
find_opaque(opaque_info, attr)
Opaque_info	opaque_info[OPAQUE_MAX];
Setup_attribute	attr;
{
	register int		i;
	register Opaque_info	*info;
	
	for (i = 0, info = opaque_info; i < OPAQUE_MAX; i++, info++)
		if (info->attr == attr)
			return info;
		else if (!info->attr) {
			info->attr = attr;
			return info;
		}

	runtime_error(
            "find_opaque(): Ran out of opaque data slots on attribute %u\n",
            (unsigned) attr);

	return NULL;
}


static void
null_callback()
{
}


void
setup_destroy(object, arg1)
Setup_object	*object;
Setup_attribute	arg1;
{   
    Setup_attribute	a[ATTR_STANDARD_SIZE];
    
    (*(object->destroy))(object, attr_make(a, ATTR_STANDARD_SIZE, &arg1)); 
    
}

