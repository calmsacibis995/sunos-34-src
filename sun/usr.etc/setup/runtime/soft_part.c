
#ifndef lint
static	char sccsid[] = "@(#)soft_part.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "setup_runtime.h"

static	void	soft_partition_set();
static	caddr_t	soft_partition_get();
static	void	soft_partition_destroy();
static  void    soft_remove_from_hard();


Hard_partition
soft_partition_create(obj)
Setup_object	*obj;
{   
	obj->data = (caddr_t) new(Hard_partition_info);
	obj->type = SETUP_SOFT_PARTITION;
	obj->set_attr = soft_partition_set;
	obj->get_attr = soft_partition_get;
	obj->destroy  = soft_partition_destroy;
	
	/* remember, new uses calloc which sets everything to 0 */
}
	


static
void
soft_partition_set(obj, avlist)
Setup_object	*obj;
Avlist		avlist;
{
	Soft_partition_info	*soft_partition;
	Setup_attribute		attr;
	int			n;
	char			*chars;
	
	soft_partition = SETUP_GET_DATA(obj, Soft_partition_info);
	
	while(attr = (Setup_attribute) *avlist++) {
		switch (attr) {
		  case SOFT_TYPE:
			soft_partition->type = (Soft_type) *avlist++;
			callback(obj, attr, FALSE);
			break;

		  case SOFT_SIZE:
			n = (int) *avlist++;
                        /* 
                         * the front ends only do an update when the _STRING
                         * attribute changes, so use it to force updates  
                         */ 
			soft_partition->size = n;
			callback(obj, SOFT_SIZE_STRING, FALSE);
			callback(obj, SOFT_SIZE_STRING_LEFT, FALSE);
			float_soft_partitions(obj);
			break;

		  case SOFT_SIZE_STRING:
		  case SOFT_SIZE_STRING_LEFT:
			chars = (char *) *avlist++;
			n = units_to_sectors(soft_partition->hp, chars);
			soft_partition->size = n;
			callback(obj, SOFT_SIZE_STRING, FALSE);
			callback(obj, SOFT_SIZE_STRING_LEFT, FALSE);
			float_soft_partitions(obj);
			break;
			
		  case SOFT_OFFSET:
			n = (int) *avlist++;
                        /* 
                         * the front ends only do an update when the _STRING
                         * attribute changes, so use it to force updates  
                         */ 
			soft_partition->offset = n;
			callback(obj, SOFT_OFFSET_STRING, FALSE);
			break;

		  case SOFT_OFFSET_STRING:
                        chars = (char *) *avlist++;
                        n = units_to_sectors(soft_partition->hp, chars);
			soft_partition->offset = n;
			callback(obj, attr, FALSE);
                        break;
			
		  case SOFT_HARD_PARTITION:
			soft_remove_from_hard(obj);
			soft_partition->hp = (Hard_partition) *avlist++;
			callback(obj, attr, FALSE);
			break;
			
		  case SOFT_CLIENT:
			soft_partition->client = (Client) *avlist++;
			callback(obj, attr, FALSE);
			break;

		  case SOFT_NDL:
			soft_partition->ndl = (int) *avlist++;
			callback(obj, attr, FALSE);
			break;
			
		  default:
			avlist = attr_skip(attr, avlist);
			break;
			
		}
	}
}


static
caddr_t
soft_partition_get(obj, attr, op1, op2)
Setup_object		*obj;
Setup_attribute		attr;
caddr_t			op1;
caddr_t			op2;
{
	Soft_partition_info	*soft_partition;   
	char			*letter;
	int			fudge;
	
	soft_partition = SETUP_GET_DATA(obj, Soft_partition_info);
	
	switch (attr) {
	case SOFT_TYPE:
		return ((caddr_t) soft_partition->type);

	case SOFT_HARD_PARTITION:
		return ((caddr_t) soft_partition->hp);
	
	case SOFT_SIZE:
		return ((caddr_t) soft_partition->size);

	case SOFT_SIZE_STRING:
		return ((caddr_t) sectors_to_units(soft_partition->hp,
					soft_partition->size));

	case SOFT_SIZE_STRING_LEFT:
		return ((caddr_t) sectors_to_units_left(soft_partition->hp,
					soft_partition->size));
	
	case SOFT_OFFSET:
		letter = setup_get(soft_partition->hp, HARD_LETTER);
		if (streq(letter, "c")) {
			fudge = (int) setup_get(soft_partition->hp,HARD_OFFSET);
			return((caddr_t) soft_partition->offset + fudge);
		}
		return ((caddr_t) soft_partition->offset);

	case SOFT_OFFSET_STRING:
		return ((caddr_t) sectors_to_units(soft_partition->hp,
					soft_partition->offset));

	case SOFT_CLIENT:
		return ((caddr_t) soft_partition->client);

	case SOFT_CLIENT_NAME:
		return (setup_get(soft_partition->client, CLIENT_NAME));

	case SOFT_NDL:
		return ((caddr_t) soft_partition->ndl);

	default:
		runtime_error("Unknown Soft_partition attribute, %x.", attr);
		break;
	
	}
}


static
void
soft_partition_destroy(obj, avlist)
Setup_object	*obj;
Avlist		avlist;
{   
	Soft_partition_info	*soft_partition;   
	
	soft_partition = SETUP_GET_DATA(obj, Soft_partition_info);
	soft_remove_from_hard(obj);
	free(soft_partition);
	free(obj);
}

/*
 * Remove a soft partition from a hard partition.
 * This is needed when a soft partition is being destroyed or
 * when it is being moved to another hard partition.
 */
static
void
soft_remove_from_hard(obj)
Setup_object    *obj;
{
        Soft_partition_info     *soft_partition;
        Soft_partition          sp;
        Hard_partition          hp;
        int                     hp_size;
        int                     n;

        soft_partition = SETUP_GET_DATA(obj, Soft_partition_info);
        hp = soft_partition->hp;
        if (hp == NULL) {
                return;
        }
        hp_size = (int)setup_get(hp, HARD_SIZE);
        hp_size -= soft_partition->size;
        setup_set(hp,
                HARD_MIN_SIZE, -(soft_partition->size),
                HARD_SIZE, hp_size,
                0);

        SETUP_FOREACH_OBJECT(hp, HARD_SOFT_PARTITION, n, sp) {
                if (sp == (Soft_partition)obj) {
                        break;
                }
        } SETUP_END_FOREACH
        delete_array_entry(hp, HARD_NUM_SOFT_PARTITION, HARD_SOFT_PARTITION, n);
	float_soft_partitions(obj);
}
