
#ifndef lint
static	char sccsid[] = "@(#)disk.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "setup_runtime.h"

static	void	disk_set();
static	caddr_t	disk_get();
static	void	disk_destroy();


Disk 
disk_create(obj)
Setup_object	*obj;
{   
	register int	i;
	
	obj->data = (caddr_t) new(Disk_info);
	obj->type = SETUP_DISK;
	obj->set_attr = disk_set;
	obj->get_attr = disk_get;
	obj->destroy  = disk_destroy;
	
	for (i = 0; i < DISK_NUM_HARD_PARTS; i++) {
		((Disk_info *)(obj->data))->hard_parts[i] = (Hard_partition)NULL;
	}
	((Disk_info *)(obj->data))->name = (char *) NULL;
}
	


static
void
disk_set(obj, avlist)
Setup_object	*obj;
Avlist		avlist;
{
	Disk_info	*disk;
	Setup_attribute	attr;
	int		hp_index, i;
	char		*name;
	
	disk = SETUP_GET_DATA(obj, Disk_info);
	
	while(attr = (Setup_attribute) *avlist++) {
		switch (attr) {
		case DISK_NAME:
			name = (char *) *avlist++;
			disk->name = (char *) strdup(name);
			break;
			
		case DISK_TYPE:
			disk->type = (Setup_setting) *avlist++;
			break;
			
		case DISK_NCYLS:
			disk->cyls = (int) *avlist++;
			break;
			
		case DISK_NHEADS:
			disk->heads = (int) *avlist++;
			break;
			
		case DISK_SEC_TRACK:
			disk->sectors_track = (int) *avlist++;
			break;
			
		case DISK_SIZE:
			disk->size = (int) *avlist++;
			callback(obj, DISK_SIZE_STRING_LEFT, FALSE);
			break;

		case DISK_FREE_SPACE:
			disk->free_space = (int) *avlist++;
			callback(obj, DISK_FREE_SPACE_STRING_LEFT, FALSE);
			break;
			
		case DISK_HARD_PARTITION:
			hp_index = (int) *avlist++;
			disk->hard_parts[hp_index] = (Hard_partition) *avlist++;
			break;

		case DISK_PARAM_FLOATING:
			i = (int) *avlist++;
			if (test_float_disk(obj, i)) {
				callback(obj, attr, TRUE);
			} else {
				disk->floating = i;
				callback(obj, attr, FALSE);
				if (disk->floating) {
					float_disk(obj);
				}
			}
			break;

		case DISK_PARAM_OVERLAPPING:
			i = (int) *avlist++;
			if (test_overlap_disk(obj, i)) {
				callback(obj, attr, TRUE);
			} else {
				disk->overlapping = i;
				callback(obj, attr, FALSE);
			}
			break;

		case DISK_PARAM_CYLROUNDING:
			disk->cyl_rounding = (int) *avlist++;
			break;
			
		case DISK_FREE_HOG_INDEX:
			/* hp_index of zero denotes no free hog */
			hp_index = (int) *avlist++;
			if (test_free_hog(obj, hp_index)) {
				callback(obj, attr, TRUE);
			} else {
				callback(obj, attr, FALSE);
			}
			break;

		default:
			avlist = attr_skip(attr, avlist);
			break;
		}
	}
}



static
caddr_t
disk_get(obj, attr, op1, op2)
Setup_object	*obj;
Setup_attribute	attr;
caddr_t		op1;
caddr_t		op2;
{
	Disk_info		*disk;   
	int			i;
	
	disk = SETUP_GET_DATA(obj, Disk_info);
	
	switch (attr) {

	case DISK_NAME:
		return ((caddr_t) disk->name);
		break;
	
	case DISK_TYPE:
		return ((caddr_t) disk->type);
		break;

	case DISK_NCYLS:
		return((caddr_t)disk->cyls);
		break;
			
	case DISK_NHEADS:
		return((caddr_t)disk->heads);
		break;
			
	case DISK_SEC_TRACK:
		return((caddr_t)disk->sectors_track);
		break;

	case DISK_SIZE:
		return ((caddr_t) disk->size);
		break;

	case DISK_SIZE_STRING_LEFT:
		return ((caddr_t) sectors_to_units_left(disk->hard_parts[0], 
				disk->size));
		break;

	case DISK_FREE_SPACE:
		return ((caddr_t) disk->free_space);
		break;

	case DISK_FREE_SPACE_STRING_LEFT:
		return ((caddr_t) sectors_to_units_left(disk->hard_parts[0],
				disk->free_space));
		break;
			
	case DISK_HARD_PARTITION:
		if ((int)op1 >= DISK_NUM_HARD_PARTS)
			return ((caddr_t) 0);
		else
			return((caddr_t) disk->hard_parts[(int)op1]);
		break;

	case DISK_FREE_HOG_INDEX:	/* index of op1 hp */
		return ((caddr_t) find_free_hog(obj));
	    
	case DISK_PARAM_FLOATING:
		return ((caddr_t) disk->floating);
		break;

	case DISK_PARAM_OVERLAPPING:
		return ((caddr_t) disk->overlapping);
		break;

	case DISK_PARAM_CYLROUNDING:
		return ((caddr_t) disk->cyl_rounding);
		break;
	
	default:
		runtime_error("Unknown disk attribute.");
		break;
	
	}
}


static
void
disk_destroy()
{   
}


find_free_hog(disk)
Disk		disk;
{
	int		i;
	Hard_partition	hp;

	/* 
	 * Index of zero denotes no free hog
	 */
	SETUP_FOREACH_OBJECT(disk, DISK_HARD_PARTITION, i, hp) {
		if ((int)setup_get(hp, HARD_PARAM_FREEHOG)) {
			return (i + 1);
		}
	} SETUP_END_FOREACH
	return (0);
}


release_free_hog(hp)
Hard_partition	hp;
{
	int	min_size;

	setup_set(hp, 
		HARD_PARAM_FREEHOG, FALSE,
		0);

	min_size = (int)setup_get(hp, HARD_MIN_SIZE);
	setup_set(hp,
		HARD_SIZE, min_size,
		0);
}


test_float_disk(disk, new_value)
Disk	disk;
int	new_value;
{
	int		old_value;
	int		i;
	Hard_partition	hp;

	old_value = (int) setup_get(disk, DISK_PARAM_FLOATING);
	if (new_value == old_value) {
		return(FALSE);
	}

	if (new_value) {	/* turning on floating */
		if ((int)setup_get(disk, DISK_PARAM_OVERLAPPING)) {
			runtime_message(SETUP_ENOFLOATWOVER, 
			    (char *)setup_get(disk, DISK_NAME));
			return(TRUE);
		}
		return(init_floating_disk(disk));
	} else {		/* turning off floating */
		SETUP_FOREACH_OBJECT(disk, DISK_HARD_PARTITION, i, hp) {
			if ((int)setup_get(hp, HARD_PARAM_FREEHOG)) {
				runtime_message(SETUP_EFREEHOGONFLT,
				    (char *)setup_get(disk, DISK_NAME),
				    (char *)setup_get(hp, HARD_NAME));
				return(TRUE);
			}
			if (is_reserved(hp) && (! is_entiredisk(hp))) {
				runtime_message(SETUP_ERESERVEONFLT,
				    (char *)setup_get(disk, DISK_NAME),
				    (char *)setup_get(hp, HARD_NAME));
				return(TRUE);
			}
		} SETUP_END_FOREACH
		return(FALSE);
	}
}


test_overlap_disk(disk, new_value)
Disk	disk;
int	new_value;
{
	int		old_value;
	int		i;
	Hard_partition	hp;

	old_value = (int) setup_get(disk, DISK_PARAM_OVERLAPPING);
	if (new_value == old_value) {
		return(FALSE);
	}

	if (new_value) {	/* turning on overlapping */
		if ((int)setup_get(disk, DISK_PARAM_FLOATING)) {
			runtime_message(SETUP_ENOOVERWFLOAT, 
			    (char *)setup_get(disk, DISK_NAME));
			return(TRUE);
		}
	}
	return(FALSE);
}

 
test_free_hog(disk, hp_index)
Disk	disk;
int	hp_index;
{
	int		i;
	Hard_partition	hp;

	i = find_free_hog(disk);

	/*
	 * setting from NONE to NONE
	 */
	if ((hp_index == 0) && (i == 0)) {
		return(FALSE);
	}

	hp_index--;
	i--;

	if ((int)setup_get(disk, DISK_PARAM_FLOATING)) {
		if (hp_index >= 0) {
			hp = (Hard_partition)setup_get(disk, 
			    DISK_HARD_PARTITION, hp_index);
			if (is_reserved(hp)) {
				if (! is_moveable(hp)) {
					runtime_message(SETUP_ECANOTHOGRES,
					    (char *)setup_get(hp, HARD_NAME));
					return(TRUE);
				}
			}
			if (is_nd_partition(hp)) {
				runtime_message(SETUP_ENDPARTFREEHOG,
				    (char *)setup_get(hp, HARD_NAME));
				return(TRUE);
			}
		}
		/*
		 * free existing free_hog
		 */
		if (i >= 0) {
			hp = (Hard_partition)setup_get(disk, 
			    DISK_HARD_PARTITION, i);
			release_free_hog(hp);
		}
		/*
		 * if not NONE allocate new free_hog
		 */
		if (hp_index >= 0) {
			hp = (Hard_partition)setup_get(disk, 
			    DISK_HARD_PARTITION, hp_index);
			setup_set(hp,
				HARD_PARAM_FREEHOG, TRUE,
				0);
		}
		return(FALSE);
	} else {
		runtime_message(SETUP_ENOHOGWOFLOAT, 
		    (char *)setup_get(disk, DISK_NAME));
		return(TRUE);
	}
}
