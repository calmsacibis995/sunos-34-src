
#ifndef lint
static	char sccsid[] = "@(#)hard_part.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "setup_runtime.h"

static	void	hard_partition_set();
static	caddr_t	hard_partition_get();
static	void	hard_partition_destroy();


Hard_partition
hard_partition_create(obj)
Setup_object	*obj;
{   
	Hard_partition_info	*hard_partition;

	/* remember, new uses calloc which sets everything to 0 */
	obj->data = (caddr_t) new(Hard_partition_info);
	obj->type = SETUP_HARD_PARTITION;
	obj->set_attr = hard_partition_set;
	obj->get_attr = hard_partition_get;
	obj->destroy  = hard_partition_destroy;

	hard_partition = (Hard_partition_info *) obj->data;
	hard_partition->mount_point = strdup("");
	
}
	


static
void
hard_partition_set(obj, avlist)
Setup_object	*obj;
Avlist		avlist;
{
	Hard_partition_info	*hard_partition;
	Setup_attribute		attr;
	int			n;
	int			ret;
	char			*chars;
	
	hard_partition = SETUP_GET_DATA(obj, Hard_partition_info);
	
	while(attr = (Setup_attribute) *avlist++) {
		switch (attr) {
		  case HARD_LETTER:
			/*
			 * Implicitly set the name as well.
			 */
			chars = (char *) *avlist++;
			hard_partition->letter = strdup(chars);
			if (hard_partition->disk) {
				sprintf(scratch_buf, "%s%s",
				    setup_get(hard_partition->disk, DISK_NAME),
				    hard_partition->letter);
				hard_partition->name = strdup(scratch_buf);
			} else {
				hard_partition->name = "None";
			}
			callback(obj, attr, FALSE);
			break;
			
		  case HARD_MOUNT_PT:
			chars = (char *) *avlist++;
			if (streq(hard_partition->mount_point, chars)) {
				break;
			}
                        if (is_reserved(obj) && (! privledged(obj))) {
                                runtime_message(SETUP_ERESHARDPART,
                                    hard_partition->name);
                                callback(obj, attr, TRUE);
			} else if (hard_partition->type != HARD_UNIX) {
				runtime_message(SETUP_EMNTPOINT,
				    hard_partition->name);
                                callback(obj, attr, TRUE);
                        } else if ((chars[0] != '/') && (! streq(chars, ""))) {
				runtime_message(SETUP_EFULLMNTPOINT,
				    hard_partition->name);
                                callback(obj, attr, TRUE);
			} else {
				hard_partition->mount_point = strdup(chars);
                                callback(obj, attr, FALSE);
                        }
			break;

		  case HARD_NUM_SOFT_PARTITION:
                        hard_partition->num_soft_parts = (int)*avlist++;
                        callback(obj, attr, FALSE);
                        break;
				  
		  case HARD_SOFT_PARTITION:
			n = (int) *avlist++;
			if (n == SETUP_APPEND) {
				n = hard_partition->num_soft_parts++;
			}
			hard_partition->soft_parts[n] = 
				(Soft_partition) *avlist++;
			callback(obj, attr, FALSE);
			break;
			
		  case HARD_SIZE:
			n = (int) *avlist++;
			if (n == hard_partition->size) {
				callback(obj, HARD_SIZE_STRING, FALSE);
				callback(obj, HARD_SIZE_STRING_LEFT, FALSE);
				break;
			}
			/*
			 * the front ends only do an update when the _STRING
			 * attribute changes, so use it to force updates 
			 */
			ret = set_hard_size(obj, hard_partition, n);
			if (ret) {			/* error somewhere */
				callback(obj, attr, ret);
			} else {
				callback(obj, HARD_SIZE_STRING, ret);
				callback(obj, HARD_SIZE_STRING_LEFT, ret);
			}
			break;

		  case HARD_MIN_SIZE:
			/*
			 * HARD_MIN_SIZE is cumulative, rather than absolute
			 */
			n = (int) *avlist++;
			hard_partition->min_size += n;
			callback(obj, attr, FALSE);
			break;
		  
		  case HARD_SIZE_STRING:
		  case HARD_SIZE_STRING_LEFT:
			chars = (char *) *avlist++;
			if (streq(chars, 
			    sectors_to_units_left(obj, hard_partition->size))) {
				callback(obj, HARD_SIZE_STRING, FALSE);
				callback(obj, HARD_SIZE_STRING_LEFT, FALSE);
				break;
			}
                        /*
                         * dont let the user change directly change the
                         * size of hard partitions used for ND
                         */
                        if (is_nd_partition(obj)) {
                                runtime_message(SETUP_ECHANGENDPART,
                                    hard_partition->name);
                                callback(obj, attr, TRUE);
                                break;
                        }
			n = units_to_sectors(obj, chars);
			ret = set_hard_size(obj, hard_partition, n);
			if (ret) {			/* error somewhere */
				callback(obj, attr, ret);
			} else {
				callback(obj, HARD_SIZE_STRING, ret);
				callback(obj, HARD_SIZE_STRING_LEFT, ret);
			}
			break;
			
		  case HARD_OFFSET:
			n = (int) *avlist++;
			if (n == hard_partition->offset) {
				callback(obj, HARD_OFFSET_STRING, FALSE);
				callback(obj, HARD_OFFSET_STRING_LEFT, FALSE);
				break;
			}
			/*
			 * the front ends only do an update when the _STRING
			 * attribute changes, so use it to force updates
			 */
			ret = set_hard_offset(obj, hard_partition, n);
			if (ret) {			/* error somewhere */
				callback(obj, attr, ret);
			} else {
				callback(obj, HARD_OFFSET_STRING, ret);
				callback(obj, HARD_OFFSET_STRING_LEFT, ret);
			}
			break;

		  case HARD_OFFSET_STRING:
		  case HARD_OFFSET_STRING_LEFT:
                        chars = (char *) *avlist++;
			if (streq(chars, 
			    sectors_to_units_left(obj, hard_partition->offset))){
				callback(obj, HARD_OFFSET_STRING, FALSE);
				callback(obj, HARD_OFFSET_STRING_LEFT, FALSE);
				break;
			}
                        n = units_to_sectors(obj, chars);
			ret = set_hard_offset(obj, hard_partition, n);
			if (ret) {			/* error somewhere */
				callback(obj, attr, ret);
			} else {
				callback(obj, HARD_OFFSET_STRING, ret);
				callback(obj, HARD_OFFSET_STRING_LEFT, ret);
			}
                        break;
			
		  case HARD_DISK:
			hard_partition->disk = (Disk) *avlist++;
			callback(obj, attr, FALSE);
			break;
			
		  case HARD_TYPE:
			if (is_reserved(obj) && (! privledged(obj))) {
				runtime_message(SETUP_ERESHARDPART, 
				    hard_partition->name);
				callback(obj, attr, TRUE);
				avlist++;
			} else {
				hard_partition->type = (Hard_type) *avlist++;
				callback(obj, attr, FALSE);
				callback(obj, HARD_WHAT_IT_IS, FALSE);
				if ((hard_partition->type == HARD_ND) 
				    || (hard_partition->type == HARD_FREE)) {
					hard_partition->min_size = 0;
					setup_set(obj, 
						HARD_SIZE, 0,
						0);
				}
			}
			break;

		  case HARD_RESERVED_TYPE:
			hard_partition->res_type = (Hard_reserved_type)*avlist++;
			if (hard_partition->res_type==HARD_RESERVED_ENTIREDISK) {
				hard_partition->offset = 0;
				callback(obj, HARD_OFFSET_STRING, FALSE);
				hard_partition->size = 
				 (int)setup_get(hard_partition->disk,DISK_SIZE);
				callback(obj, HARD_SIZE_STRING, FALSE);
			}
			break;

		  case HARD_PARAM_FREEHOG:
			hard_partition->free_hog = (int) *avlist++;
			callback(obj, attr, FALSE);
			/*
			 * return to the free count any sectors allocated
			 */
			if (hard_partition->size > 0) {
 				size_floating_disk(hard_partition, 0);
				hard_partition->size = 0;
			}
			float_disk(hard_partition->disk);
			callback(obj, HARD_SIZE_STRING, FALSE);
			callback(obj, HARD_SIZE_STRING_LEFT, FALSE);
			break;
			
		  default:
			avlist = attr_skip(attr, avlist);
			break;
			
		}
	}
}


/*
 * Set the size of a hard partition.
 * Common code for HARD_SIZE and HARD_SIZE_STRING.
 */
static
set_hard_size(obj, hard_partition, n)
Setup_object		*obj;
Hard_partition_info	*hard_partition;
int			n;
{
	int	size;

	if (setup_get(hard_partition->disk, DISK_PARAM_CYLROUNDING)) {
		n = round_to_cyls(hard_partition->disk, n);
	}
	if (hard_partition->free_hog) {
		size = disk_free_space(hard_partition->disk);
	} else {
		size = hard_partition->size;
	}
	if (size == n) {
		return(FALSE);
	}
	if (check_changeable(obj)) {
		return(TRUE);
	}
	if (check_minimum(obj, n)) {
		return(TRUE);
	}
	if (check_hard_partition(hard_partition, n, 0)) {
		return(TRUE);
	}
	if (is_floating(hard_partition->disk)) {
		if (size_floating_disk(hard_partition, n)) {
			return(TRUE);
		} else {
			hard_partition->size = n;
			float_disk(hard_partition->disk);
			return(FALSE);
		}
	} else {
		hard_partition->size = n;
		return(FALSE);
	}
}

/*
 * Set the offset of a hard partition.
 * Common code for HARD_OFFSET and HARD_OFFSET_STRING.
 */
static
set_hard_offset(obj, hard_partition, n)
Setup_object		*obj;
Hard_partition_info	*hard_partition;
int			n;
{
	n = round_to_cyls(hard_partition->disk, n);
	if (hard_partition->offset == n) {
		return(FALSE);
	}
	if (check_changeable(obj)) {
		return(TRUE);
	}
	if (is_floating(hard_partition->disk) &&
	    (! privledged(obj))) {
		runtime_message(SETUP_EFLOATING);
		return(TRUE);
	}
	if (check_hard_partition(hard_partition, n, 1)) {
		return(TRUE);
	} else {
		hard_partition->offset = n;
		return(FALSE);
	}
}

static
caddr_t
hard_partition_get(obj, attr, op1, op2)
Setup_object		*obj;
Setup_attribute		attr;
caddr_t			op1;
caddr_t			op2;
{
	Disk			disk;
	Hard_partition_info	*hard_partition;   
	Hard_partition		hp;
	int			nd_index;
	int			size;
	
	hard_partition = SETUP_GET_DATA(obj, Hard_partition_info);
	disk = hard_partition->disk;
	
	switch (attr) {
	case HARD_NAME:
		return((caddr_t) hard_partition->name);

	case HARD_LETTER:
		return ((caddr_t) hard_partition->letter);

	case HARD_MOUNT_PT:
		return ((caddr_t) hard_partition->mount_point);

	case HARD_NUM_SOFT_PARTITION:
                return ((caddr_t) hard_partition->num_soft_parts);
                break;

	case HARD_SOFT_PARTITION:
		return ((caddr_t) hard_partition->soft_parts[(int) op1]);
	
	case HARD_SIZE:
		if (hard_partition->free_hog) {
			size = disk_free_space(disk);
			return ((caddr_t) size);
		} else {
			return ((caddr_t) hard_partition->size);
		}

	case HARD_SIZE_STRING:
		if (hard_partition->free_hog) {
			size = disk_free_space(disk);
			return ((caddr_t) sectors_to_units(obj, size));
		} else {
			return ((caddr_t) 
			    sectors_to_units(obj, hard_partition->size));
		}

	case HARD_SIZE_STRING_LEFT:
		if (hard_partition->free_hog) {
			size = disk_free_space(disk);
			return ((caddr_t) sectors_to_units_left(obj, size));
		} else {
			return ((caddr_t) 
			    sectors_to_units_left(obj, hard_partition->size));
		}

	case HARD_SIZE_REAL:
		if (streq(hard_partition->letter, "c")) {
			return(setup_get(disk, DISK_SIZE));
		}
		if (hard_partition->free_hog) {
			size = disk_free_space(disk);
			return((caddr_t) size);
		}
		return ((caddr_t) hard_partition->size);
	
	case HARD_OFFSET:
		return ((caddr_t) hard_partition->offset);

	case HARD_OFFSET_CYL:
		return ((caddr_t) offset_to_cyls(obj));

	case HARD_OFFSET_STRING:
		return ((caddr_t) sectors_to_units(obj,hard_partition->offset));

	case HARD_OFFSET_STRING_LEFT:
		return ((caddr_t) sectors_to_units_left(obj,
			hard_partition->offset));

	case HARD_OFFSET_CYL_REAL:
		if (streq(hard_partition->letter, "c")) {
			return(0);
		}
		return ((caddr_t) offset_to_cyls(obj));

	case HARD_DISK:
		return ((caddr_t) disk);

	case HARD_DISK_NAME:
		return (setup_get(disk, DISK_NAME));

	case HARD_TYPE:
		return ((caddr_t) hard_partition->type);

	case HARD_ND_INDEX:
		SETUP_FOREACH_OBJECT(workstation, WS_ND_PARTITION, nd_index, hp)
			if (hp == (Hard_partition) obj)
				return ((caddr_t) nd_index);
		SETUP_END_FOREACH
		return ((caddr_t) 0);

	case HARD_RESERVED_TYPE:
		return ((caddr_t) hard_partition->res_type);
		break;

	case HARD_PARAM_FREEHOG:
		return ((caddr_t) hard_partition->free_hog);
		break;

	case HARD_MOVEABLE:
		return ((caddr_t) is_moveable(obj));
		break;
		
	case HARD_WHAT_IT_IS:
		if (is_reserved(obj)) {
			return((caddr_t)
			 hard_reserved_info[ord(hard_partition->res_type)].name);
		} else {
			return((caddr_t)
			    setup_get(workstation, SETUP_CHOICE_STRING, 
			    CONFIG_HARD_PARTITION_TYPE, 
			    ord(hard_partition->type)));
		}
		break;

	case HARD_TEST_FIT:
		return ((caddr_t) test_hard_size(obj,hard_partition,(int)op1));
		break;

	case HARD_MIN_SIZE:
		return ((caddr_t) hard_partition->min_size);
		break;

	case HARD_MIN_SIZE_IN_UNITS:
		return ((caddr_t) 
		    atoi(sectors_to_units(obj, hard_partition->min_size)));
		break;

	case HARD_MAX_SIZE_IN_UNITS:
		if (! (int)setup_get(disk, DISK_PARAM_OVERLAPPING)) {
			size = disk_free_space(disk);
			return((caddr_t) 
			    (atoi(sectors_to_units(obj, hard_partition->size)) + 
			     atoi(sectors_to_units(obj, size))));
		} else {
			size = (int) setup_get(disk, DISK_SIZE);
			return((caddr_t) atoi(sectors_to_units(obj, size)));
		}
		break;
		
	case HARD_INDEX:
		SETUP_FOREACH_OBJECT(disk, DISK_HARD_PARTITION, nd_index, hp)
		    if (hp == (Hard_partition) obj)
			return ((caddr_t) nd_index);
		SETUP_END_FOREACH
		return ((caddr_t) 0);

	default:
		runtime_error("Unknown Hard_partition attribute, %x.", attr);
		break;
	
	}
}


static
void
hard_partition_destroy()
{   
}


/*
 * type: 0 = size
 *       1 = offset
 */
check_hard_partition(hard_partition, newvalue, type)
Hard_partition_info     *hard_partition;
int			newvalue;
int			type;
{
	Disk		disk;
	Hard_partition	hp;
	int		n;
	int		free_space;
	int		newoffset;
	int		newsize;
	int		size;
	int		offset;
	int		disksize;
	Hard_partition_info     *current_hard_partition;

	disk = hard_partition->disk;

	if (newvalue < 0) {
		runtime_message(SETUP_EHARDPARTNEG);
		return(TRUE);
		
	}

	/*
	 * since only the middle can change offsets if we are floating
	 * assume any offset change is legal
	 */
	if ((int)setup_get(disk, DISK_PARAM_FLOATING)) {
		if (type == 0) {			/* size */
			newsize = newvalue - hard_partition->size;
			free_space = (int)setup_get(disk, DISK_FREE_SPACE);
			if (newsize > free_space) {
				hp = (Hard_partition)setup_get(disk,
				    DISK_HARD_PARTITION, 0);
				runtime_message(SETUP_EHARDPARTTOBIG, 
				    hard_partition->letter, 
		    		    sectors_to_units_left(hp, 
					hard_partition->size + free_space));
				return(TRUE);
			}
		}
		return(FALSE);
	}

	if (type) {					/* new offset */
		newoffset = newvalue;
		newsize = hard_partition->size;
	} else {					/* new size */
		newoffset = hard_partition->offset;
		newsize = newvalue;
	}

	disksize = (int)setup_get(disk, DISK_SIZE);
	if ((newoffset + newsize) > disksize) {
		hp = setup_get(disk, DISK_HARD_PARTITION, 0);
		runtime_message(SETUP_EHARDPARTTOBIG, hard_partition->letter, 
		    sectors_to_units_left(hp, disksize - newoffset));
		return(TRUE);
	}

	if (newsize == 0) {
		return(FALSE);
	}

	if (! (int)setup_get(disk, DISK_PARAM_OVERLAPPING)) {
		SETUP_FOREACH_OBJECT(disk, DISK_HARD_PARTITION, n, hp)

			if (is_entiredisk(hp)) {
				continue;
			}

			current_hard_partition = 
		          SETUP_GET_DATA((Setup_object *)hp,Hard_partition_info);
			if (hard_partition == current_hard_partition) {
				continue;
			}

			offset = (int)setup_get(hp, HARD_OFFSET);
			size = (int)setup_get(hp, HARD_SIZE);
			if (size == 0) {
				continue;
			}

			if (isoverlapped(newoffset, (newoffset + newsize),
			    offset, (offset + size))) {
				runtime_message(SETUP_EHARDPARTOVERLAP);
				return(TRUE);
			}

		SETUP_END_FOREACH
	} else {
		return(FALSE);
	}

	return(FALSE);

}


size_floating_disk(hp_info, new_size)
Hard_partition_info	*hp_info;
int			new_size;
{
	int	free_space;

	free_space = (int)setup_get(hp_info->disk, DISK_FREE_SPACE);
	free_space -= (new_size - hp_info->size);

	if (free_space < 0) {
		runtime_message(SETUP_ENOFREESPACE, 
		    (char *)setup_get(hp_info->disk, DISK_NAME),
		    hp_info->letter);
		return(TRUE);
	} else {
		setup_set(hp_info->disk, DISK_FREE_SPACE, free_space, 0);
		return(FALSE);
	}
}


Hard_partition
hard_partition_initialization(type, disk)
Hard_reserved_type	type;
Disk			disk;
{
	Hard_partition		hp;
	Hard_partition_info	*hard_partition;

	hp = (Hard_partition)setup_get(disk, 
	    DISK_HARD_PARTITION, (hard_reserved_info[ord(type)].partition-'a'));

	/*
	 * if we are changing the entire disk then force it directly
	 * otherwise we will add the entire disk size to the free space
	 */
	if (is_entiredisk(hp)) {

		hard_partition = SETUP_GET_DATA((Setup_object *)hp, 
		    Hard_partition_info);
		hard_partition->size = 0;
		setup_set(hp, 
			HARD_RESERVED_TYPE, HARD_RESERVED_NONE, 
			HARD_TYPE, hard_reserved_info[ord(type)].type, 
			HARD_MIN_SIZE, 
			    hard_reserved_info[ord(type)].minimum_size, 
			0);

	} else {

		setup_set(hp, 
			HARD_RESERVED_TYPE, HARD_RESERVED_NONE, 
			HARD_TYPE, hard_reserved_info[ord(type)].type, 
			0);

		if ((type == HARD_RESERVED_ROOT)||(type == HARD_RESERVED_SWAP)) {
			setup_set(hp, 
				HARD_SIZE, (int)setup_get(hp, HARD_SIZE),
				HARD_MIN_SIZE, (int)setup_get(hp, HARD_SIZE),
				0);
		} else {
			setup_set(hp, 
				HARD_SIZE, 
				    hard_reserved_info[ord(type)].minimum_size,
				HARD_MIN_SIZE, 
				    hard_reserved_info[ord(type)].minimum_size,
				0);
		}

		if (hard_reserved_info[ord(type)].type == HARD_UNIX) {
			setup_set(hp,
				HARD_MOUNT_PT,
				    hard_reserved_info[ord(type)].mount_point, 
				0);
		}

	}

	setup_set(hp, 
		HARD_RESERVED_TYPE, type, 
		0);

	return(hp);

}


hard_partition_free(hp)
Hard_partition	hp;
{
	char		*letter;

	if (((Hard_type)setup_get(hp, HARD_TYPE)) == HARD_UNIX) {
		privledged_setup_set(hp, 
			HARD_MOUNT_PT, "",
			0); 
	}

	/*
	 * need to be privledged because users cannot set the size on
	 * the freehog, and we need to set the size to 0 before changing
	 * freehog otherwise the free counter gets changed twice
	 */
	privledged_setup_set(hp, 
		HARD_RESERVED_TYPE, HARD_RESERVED_NONE, 
		HARD_MIN_SIZE, -((int)setup_get(hp, HARD_MIN_SIZE)),
		HARD_SIZE, 0,
		HARD_PARAM_FREEHOG, FALSE, 
		HARD_TYPE, HARD_FREE,
		0); 

	letter = (char *)setup_get(hp, HARD_LETTER);
	if (strcmp(letter, "c") == 0) {
		setup_set(hp, 
			HARD_RESERVED_TYPE, HARD_RESERVED_ENTIREDISK,
			0);
	}

}


/*
 * zero out any partitions that remain unused 
 */
zero_unused_partitions(disk)
Disk	disk;
{
	int		i;
	Hard_partition	hp;

	/* 
	 * zero the size first so that we dont have an overlapping
	 * partition when we set the offset to 0
	 */
	SETUP_FOREACH_OBJECT(disk, DISK_HARD_PARTITION, i, hp) {
                if (! is_reserved(hp)) {
			if ((int)setup_get(hp, HARD_PARAM_FREEHOG)) {
				setup_set(hp,
					HARD_PARAM_FREEHOG, FALSE,
					0);
			}
                        setup_set(hp, 
			    HARD_SIZE, 0, 
			    0);
			/*
			 * we need to set the offset as privledged because
			 * the disk may be floating
			 */
                        privledged_setup_set(hp, 
			    HARD_OFFSET, 0, 
			    0);
                }
        } SETUP_END_FOREACH

	setup_set(disk, 
		DISK_PARAM_FLOATING, TRUE, 
		0);

}


check_minimum(hp, size)
Hard_partition	hp;
{
	int	min_size;

	min_size = (int)setup_get(hp, HARD_MIN_SIZE);
	if (size < min_size) {
		runtime_message(SETUP_ENORESMIN,
		    (char *)setup_get(hp, HARD_NAME),
		    sectors_to_units_left(hp, min_size));
		return(TRUE);
	}
	return(FALSE);
}


check_changeable(hp)
Hard_partition	hp;
{
	Hard_reserved_type	type;

	if (privledged(hp)) {
		return(FALSE);
	}

	if (is_entiredisk(hp)) {
		runtime_message(SETUP_ENTIREDISK);
		return(TRUE);
	}

	type = (Hard_reserved_type)setup_get(hp, HARD_RESERVED_TYPE);
	if (type == HARD_RESERVED_ROOT) {
		runtime_message(SETUP_EROOTCHANGED);
		return(TRUE);
	}

	if ((int)setup_get(hp, HARD_PARAM_FREEHOG)) {
		runtime_message(SETUP_EFREEHOG, 
		    (char *)setup_get(hp, HARD_LETTER));
		return(TRUE);
	}

	return(FALSE);
}


/*
 * non-modifying test to see if a hard partition's size can be
 * be changed + delta sectors
 */
static
test_hard_size(obj, hp_info, delta)
Setup_object            *obj;
Hard_partition_info     *hp_info;
int                     delta;
{
	int	free_space;
	int	new_size;

        if (check_changeable(obj)) {
                return(FALSE);
        }

        if (setup_get(hp_info->disk, DISK_PARAM_CYLROUNDING)) {
                delta = round_to_cyls(hp_info->disk, delta);
        }

	new_size = delta + hp_info->size;
        if (check_hard_partition(hp_info, new_size, 0)) {
                return(FALSE);
        }

        if (is_floating(hp_info->disk)) {
		free_space = (int)setup_get(hp_info->disk, DISK_FREE_SPACE);
		free_space -= delta;
		if (free_space < 0) {
			return(FALSE);
                }
        }

	return(TRUE);
}


is_moveable(hp)
Hard_partition	hp;
{
	Hard_reserved_type	type;

	if (! is_reserved(hp)) {
		return(FALSE);
	}
	if (is_entiredisk(hp)) {
		return(FALSE);
	}
	type = (Hard_reserved_type)setup_get(hp, HARD_RESERVED_TYPE);
	if ((type == HARD_RESERVED_ROOT) || (type == HARD_RESERVED_SWAP)
	    || (type == HARD_RESERVED_ND)) {
		return(FALSE);
	}
	return(TRUE);
}


/*
 * return the free space available on a disk, if cylinder rounding is
 * turned on round down to the cylinder boundary
 */
disk_free_space(disk)
Disk	disk;
{
	int	free_space;

	free_space = (int)setup_get(disk, DISK_FREE_SPACE);
        if (setup_get(disk, DISK_PARAM_CYLROUNDING)) {
                return(round_down_to_cyls(disk, free_space));
        }
	return(free_space);
}
