
#ifndef lint
static	char sccsid[] = "@(#)arch_served.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "setup_runtime.h"
 

static	void	arch_served_set();
static	caddr_t	arch_served_get();
static	void	arch_served_destroy();
static	Oswg	oswg_lookup();


Arch_served 
arch_served_create(obj)
Setup_object	*obj;
{   
	register Arch_served_info	*arch_served;
	
	obj->data = (caddr_t) new(Arch_served_info);
	obj->type = SETUP_ARCH_SERVED;
	obj->set_attr = arch_served_set;
	obj->get_attr = arch_served_get;
	obj->destroy  = arch_served_destroy;
	
	((Arch_served_info *)(obj->data))->name = strdup("");
}
	

static
void
arch_served_set(obj, avlist)
Setup_object	*obj;
Avlist		avlist;
{
	Arch_served_info	*arch_served;
	Setup_attribute		attr;
	char			*ptr;
	int			n;
	Map_t			new_map;
	
	arch_served = SETUP_GET_DATA(obj, Arch_served_info);
	
	while(attr = (Setup_attribute) *avlist++) {
		switch (attr) {

		case ARCH_SERVED_NAME:
			ptr = (char *) *avlist++;
		        arch_served->name = strdup(ptr);
			break;

		case ARCH_SERVED_TYPE:
		        arch_served->type = (Arch_type)*avlist++;
			break;
			
		case ARCH_SERVED_PUBHARD:
			arch_served->pub_hard = (Hard_partition)*avlist++;
			break;

		case ARCH_SERVED_USRHARD:
			arch_served->usr_hard = (Hard_partition)*avlist++;
			break;

		case ARCH_OSWG:
			new_map = (Map_t)*avlist++;
			/*
			 * for individual OSWGs, only callback if choosing
			 * it failed
			 */
			if (adjust_oswg(obj, new_map, arch_served)) {
				callback(obj, ARCH_OSWG, TRUE);
			}
			break;

		case ARCH_OSWG_CLEAR:
			new_map = 0;
			if (adjust_oswg(obj, new_map, arch_served)) {
				callback(obj, ARCH_OSWG, TRUE);
			} else {
				callback(obj, ARCH_OSWG, FALSE);
			}
			break;

		case ARCH_OSWG_ALL:
			new_map = 0xffffffff;
			if (adjust_oswg(obj, new_map, arch_served)) {
				callback(obj, ARCH_OSWG, TRUE);
			} else {
				callback(obj, ARCH_OSWG, FALSE);
			}
			break;

		case ARCH_OSWG_DEFAULT:
			new_map = (Map_t)setup_get(workstation, 
			    PARAM_DEFAULT_OSWG);
			if (adjust_oswg(obj, new_map, arch_served)) {
				callback(obj, ARCH_OSWG, TRUE);
			} else {
				callback(obj, ARCH_OSWG, FALSE);
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
arch_served_get(obj, attr, op1, op2)
Setup_object		*obj;
Setup_attribute		attr;
caddr_t			op1;
caddr_t			op2;
{
	Arch_served_info	*arch_served;   
	
	arch_served = SETUP_GET_DATA(obj, Arch_served_info);
	
	switch (attr) {

	case ARCH_SERVED_NAME:
		return ((caddr_t) arch_served->name);
		break;

	case ARCH_SERVED_TYPE:
		return ((caddr_t) arch_served->type);
		break;
	 
	case ARCH_SERVED_PUBHARD:
		return ((caddr_t) arch_served->pub_hard);
		break;

	case ARCH_SERVED_USRHARD:
		return ((caddr_t) arch_served->usr_hard);
		break;

	case ARCH_OSWG:
	case ARCH_OSWG_CLEAR:
	case ARCH_OSWG_ALL:
	case ARCH_OSWG_DEFAULT:
		return ((caddr_t) arch_served->oswg_map);
		break;

	case ARCH_OSWG_SELECTED:
		return ((caddr_t) oswg_lookup(obj, (int)op1));
		break;

	default:
		runtime_error("Unknown arch_served attribute.");
		break;
	
	}
}


static
void
arch_served_destroy(obj, avlist)
Setup_object	*obj;
Avlist		avlist;
{   
	Arch_served_info		*arch_served;
	
	arch_served = SETUP_GET_DATA(obj, Arch_served_info);
	
	free(arch_served);
	free(obj);
}


legal_arch_served(arch_bit_map)
Map_t	arch_bit_map;
{
	Arch_type	ws_arch;
	Arch_type	arch_type;
	Arch_served	arch;
	Arch_served	arch_found;
	int		i, n;
	Hard_partition	hp;

	ws_arch = (Arch_type)setup_get(workstation, WS_ARCH);

	if (not_in_map(arch_bit_map, ord(ws_arch))) {
		runtime_message(SETUP_ENOSERVERFORWS);
		return(TRUE);
	}

	for (i = 0; i < ord(ARCH_TYPE_LAST); i++) {

		if (i == ord(ws_arch)) {
			continue;
		}

		arch_found = NULL;
		SETUP_FOREACH_OBJECT(workstation, WS_ARCH_SERVED_ARRAY, n, arch){
			arch_type = (Arch_type)setup_get(arch, ARCH_SERVED_TYPE);
			if (i == ord(arch_type)) {
				arch_found = arch;
				break;
			}
		} SETUP_END_FOREACH

		if (is_in_map(arch_bit_map, i)) {
			if (arch_found == NULL) {
				add_arch_served(i);
			}
		} else {
			if (arch_found != NULL) {
                                if (remove_arch_served(n)) {
					return(TRUE);
				}
                        }
		}

	}

	return (FALSE);
}


add_arch_served(new_arch_type)
int	new_arch_type;
{
	Controller	cont;
	Disk		disk;
	Hard_partition	hp;
	char		*arch_name;
	char		*ws_name;
	char		mount_point[32];
	Arch_served	arch;
	int		i;

	
	arch_name = (char *)setup_get(workstation, SETUP_CHOICE_STRING, 
			CONFIG_CPU, new_arch_type),
	arch = (Arch_served)setup_create(ARCH_SERVED,
		ARCH_SERVED_NAME, arch_name,
		ARCH_SERVED_TYPE, (Arch_type)new_arch_type, 
		0);
	setup_set(workstation, WS_ARCH_SERVED_ARRAY,
                    SETUP_APPEND,  arch,
                    0);

	cont = setup_get(workstation, WS_CONTROLLER, 0);
	disk = setup_get(cont, CONTROLLER_DISK, 0);

	zero_unused_partitions(disk);

	if(setup_get(workstation, WS_SERVED_NDHARD) == 0) {
		hp = hard_partition_initialization(HARD_RESERVED_ND, disk);
		setup_set(workstation, WS_SERVED_NDHARD, hp, 0);
	}

	if(setup_get(workstation, WS_SERVED_HOMEDIRHARD) == 0) {
		hp = hard_partition_initialization(HARD_RESERVED_SERVERHOMEDIRS,
		    disk);
		setup_set(hp, HARD_PARAM_FREEHOG, TRUE, 0); 
		setup_set(workstation, WS_SERVED_HOMEDIRHARD, hp, 0);
		ws_name = (char *)setup_get(workstation, WS_NAME);
		sprintf(mount_point, "/usr/%s", ws_name);
		privledged_setup_set(hp, HARD_MOUNT_PT, mount_point, 0);
	}

	if (new_arch_type == ord(MC68010)) {
		hp = hard_partition_initialization(HARD_RESERVED_PUB010, disk);
		setup_set(arch, ARCH_SERVED_PUBHARD, hp, 0);
		hp = hard_partition_initialization(HARD_RESERVED_USR010, disk);
		setup_set(arch, ARCH_SERVED_USRHARD, hp, 0);
	} else if (new_arch_type == ord(MC68020)) {
		hp = hard_partition_initialization(HARD_RESERVED_PUB020, disk);
		setup_set(arch, ARCH_SERVED_PUBHARD, hp, 0);
		hp = hard_partition_initialization(HARD_RESERVED_USR020, disk);
		setup_set(arch, ARCH_SERVED_USRHARD, hp, 0);
	}

}


remove_arch_served(array_index)
int	array_index;
{
	Arch_served	arch;
	Arch_type	arch_type;
	Client		client;
	Hard_partition	hp;
	int		i;


	arch = (Arch_served)setup_get(workstation, 
	    WS_ARCH_SERVED_ARRAY, array_index);
	arch_type = (Arch_type)setup_get(arch, ARCH_SERVED_TYPE);

	SETUP_FOREACH_OBJECT(workstation, WS_CLIENT, i, client) {
		if (arch_type == (Arch_type)setup_get(client, CLIENT_ARCH)) {
			runtime_message(SETUP_EARCHHASCLIENTS,
			    (char *) setup_get(arch, ARCH_SERVED_NAME));
			return(TRUE);
		}
	} SETUP_END_FOREACH

	hard_partition_free((Hard_partition)setup_get(arch,ARCH_SERVED_USRHARD));
	hard_partition_free((Hard_partition)setup_get(arch,ARCH_SERVED_PUBHARD));

	setup_destroy(arch, 0);

	delete_array_entry(workstation, 
	    WS_NUM_ARCH_SERVED, WS_ARCH_SERVED_ARRAY, array_index);
	
	return(FALSE);

}


clear_server_hp()
{
	Hard_partition	hp;

	hp = setup_get(workstation, WS_SERVED_NDHARD);
	hard_partition_free(hp);
	setup_set(workstation, WS_SERVED_NDHARD, 0, 0);

	hp = setup_get(workstation, WS_SERVED_HOMEDIRHARD);
	hard_partition_free(hp);
	setup_set(workstation, WS_SERVED_HOMEDIRHARD, 0, 0);
}


adjust_oswg(arch, new_map, arch_served_info)
Arch_served		arch;
Map_t			new_map;
Arch_served_info        *arch_served_info;
{
	Map_t		old_map;
	int		i;
	int		size;
	Oswg		oswg;
	int		usrsize;
	Hard_partition	get_root_hp(), hp;
	unsigned	mask;
	Arch_type	arch_type;
	int		freehog;
	int		minsize;

	old_map = (int) setup_get(arch, ARCH_OSWG);
	arch_type = (Arch_type)setup_get(arch, ARCH_SERVED_TYPE);
	hp = (Hard_partition)setup_get(arch, ARCH_SERVED_USRHARD);
	freehog = (int)setup_get(hp, HARD_PARAM_FREEHOG);

	SETUP_FOREACH_OBJECT(workstation, WS_OSWG, i, oswg) {

		usrsize = (int)setup_get(oswg, OSWG_USRSIZE, ord(arch_type));
		size = (int)setup_get(hp, HARD_SIZE);
		minsize = (int)setup_get(hp, HARD_MIN_SIZE);

		/*
		 * adding an optional software category
		 */
		if (is_in_map(new_map, i) && not_in_map(old_map, i)) {
			if (freehog) {
				if ((minsize + usrsize) <= size) {
					setup_set(hp,
						HARD_MIN_SIZE, usrsize, 
						0);
				/*
				 * for some reason (probably out of space)
				 * the new size didnt take so assign the bit
				 * mask of the oswg's that we have finished
				 */
				} else {
					mask = (~0 << i);
					arch_served_info->oswg_map = 
					    (old_map & mask) | (new_map & ~mask);
					runtime_message(SETUP_EOSWGDONTFIT, 
					    (char *) setup_get(hp, HARD_NAME),
					    (char *) setup_get(oswg, OSWG_NAME));
					return(TRUE);
				}
			} else {
				if ((int)setup_get(hp, HARD_TEST_FIT, usrsize)) {
					setup_set(hp,
						HARD_SIZE, (size + usrsize), 
						HARD_MIN_SIZE, usrsize, 
						0);
				/*
				 * for some reason (probably out of space)
				 * the new size didnt take so assign the bit
				 * mask of the oswg's that we have finished
				 */
				} else {
					mask = (~0 << i);
					arch_served_info->oswg_map = 
					    (old_map & mask) | (new_map & ~mask);
					runtime_message(SETUP_EOSWGDONTFIT, 
					    (char *) setup_get(hp, HARD_NAME),
					    (char *) setup_get(oswg, OSWG_NAME));
					return(TRUE);
				}
			}
		/*
		 * removing an optional software category
		 */
		} else if (not_in_map(new_map, i) && is_in_map(old_map, i)) {
			if (freehog) {
				setup_set(hp,
				    HARD_MIN_SIZE, -(usrsize),
				    0);
			} else {
				setup_set(hp,
				    HARD_MIN_SIZE, -(usrsize),
				    HARD_SIZE, (size - usrsize), 
				    0);
			}
		}

	} SETUP_END_FOREACH

	arch_served_info->oswg_map = new_map;
	return(FALSE);
}


Hard_partition
get_root_hp()
{
	Controller	cont;
	Disk		disk;
	Hard_partition	hp;

	cont = setup_get(workstation, WS_CONTROLLER, 0);
        disk = setup_get(cont, CONTROLLER_DISK, 0); 
        hp = setup_get(disk, DISK_HARD_PARTITION, 0);
	return(hp);
}


static
Oswg
oswg_lookup(obj, ix)
Setup_object	*obj;
int		ix;
{
	Arch_served_info	*arch_served;   
	Map_t			map;
	int			i;
	int			found;
	
	arch_served = SETUP_GET_DATA(obj, Arch_served_info);

	map = arch_served->oswg_map;
	found = -1;
	for (i = 0; i < WS_MAX_OSWGS; i++) {
		if (is_in_map(map, i)) {
			found++;
			if (found == ix) {
				return((Oswg)setup_get(workstation,WS_OSWG,i));
			}
		}
	}
	return(NULL);
}
