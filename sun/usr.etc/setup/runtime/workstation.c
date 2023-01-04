
#ifndef lint
static	char sccsid[] = "@(#)workstation.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "setup_runtime.h"

static	void	workstation_set();
static	caddr_t	workstation_get();
static	void	workstation_destroy();

static char	*config_string();


Workstation 
workstation_create(ws_obj)
Setup_object	*ws_obj;
{
	Workstation_info	*ws;

	ws_obj->data       = (caddr_t) new(Workstation_info);
        ws_obj->type            = SETUP_WORKSTATION;
        ws_obj->set_attr        = workstation_set;
        ws_obj->get_attr        = workstation_get;
        ws_obj->destroy         = workstation_destroy;
	ws 		   = (Workstation_info *) ws_obj->data;

	ws->info.name                   = strdup("");
        SET_ATTR_BIT(ws_obj->status, WS_NAME);
        ws->info.host_number            = strdup("");
        SET_ATTR_BIT(ws_obj->status, WS_HOST_NUMBER);
        ws->tape_server                 = strdup("");
        SET_ATTR_BIT(ws_obj->status, WS_TAPE_SERVER);
        ws->tape_server_inet      	= strdup("");
        SET_ATTR_BIT(ws_obj->status, WS_HOST_INTERNET_NUMBER);
	ws->type			= WORKSTATION_NONE;
        SET_ATTR_BIT(ws_obj->status, WS_TYPE);
        ws->domain                      = strdup("");
        SET_ATTR_BIT(ws_obj->status, WS_DOMAIN);
	ws->yp_master_name		= strdup("");
        SET_ATTR_BIT(ws_obj->status, WS_YPMASTER_NAME);
	ws->yp_master_internet		= strdup("");
        SET_ATTR_BIT(ws_obj->status, WS_YPMASTER_INTERNET);
	ws->info.cpu_arch = (is68020())? MC68020: MC68010;
	ws->mail_type = MAIL_CLIENT;
	ws->params.firsthost = MIN_HOST_NUMBER;	

}

static void
workstation_set(obj, avlist)
Setup_object		*obj;
caddr_t			*avlist;
{
	Workstation_info	*ws;
	Setup_attribute		attr;
	int			n;
	int			max_host_number;
	int			config_index;
	char			*ptr;
	char			*host_number;
	char			mnt_pt[32];
	char			number[16];
	Workstation_type	type;
	Map_t			new_map;
	Hard_partition		hp;

	ws = SETUP_GET_DATA(obj, Workstation_info);
	
	while(attr = (Setup_attribute) *avlist++) {
		switch (attr) {

		case WS_DOMAIN:
			ptr = (char *)*avlist++;
                        if (legal_name(ptr)) {
				callback(obj, attr, TRUE);
                        } else {
                                ws->domain = strdup(ptr);
				callback(obj, attr, FALSE);
                        }
			break;

		case WS_NETWORK:
			ptr = (char *)*avlist++;
                        if ((ws->class = legal_network(ptr)) == BAD_NETWORK) {
				callback(obj, attr, TRUE);
                        } else {
                                ws->network = strdup(ptr);
				callback(obj, attr, FALSE);
                        }
			break;

		case WS_NAME:
			ptr = (char *)*avlist++;
                        if (legal_client_name(ptr, ws->info.name)) {
				callback(obj, attr, TRUE);
                        } else {
                                ws->info.name = strdup(ptr);
				callback(obj, attr, FALSE);
				if (ws->type == WORKSTATION_SERVER) {
					sprintf(mnt_pt, "/usr/%s", ptr);
					privledged_setup_set(ws->homedir_hard, 
					    HARD_MOUNT_PT, mnt_pt, 
					    0);
				}
                        }
			break;

		case WS_HOST_NUMBER:
			/*
			 * set host number to the null string so that
			 * legal_host_number doesnt find a dup 
			 */
			ws->info.host_number = strdup("");
			ptr = (char *)*avlist++;
                        if ((host_number = legal_host_number(ptr)) == NULL) {
				callback(obj, attr, TRUE);
                        } else {
                                ws->info.host_number = host_number;
				callback(obj, attr, FALSE);
                        }
			break;

		case WS_MODEL:
			ws->info.model = (int)*avlist++;
			callback(obj, attr, FALSE);
			break;

		case WS_ARCH:
			ws->info.cpu_arch = (Arch_type)*avlist++;
			callback(obj, attr, FALSE);
			break;

		case WS_ETHERTYPE:
			n = (int)*avlist++;
			if (n == 0) {		/* NONE */
				if (ws->num_clients) {
					runtime_message(SETUP_ENEEDETHERNET);
					callback(obj, attr, TRUE);
				} else {
					ws->ethertype = n;
					callback(obj, attr, FALSE);
				}
			} else {
				ws->ethertype = n;
				callback(obj, attr, FALSE);
				if ((ws->params.autohost) &&
				    (streq("", ws->info.host_number))) {
					sprintf(number, "%d", 
					    ws->params.firsthost);
					ws->info.host_number = strdup(number);
					callback(obj, WS_HOST_NUMBER, FALSE);
				}
			}
			break;

		case WS_ARCH_SERVED:
			new_map = (Map_t)*avlist++;
                        if (legal_arch_served(new_map)) {
                                callback(obj, attr, TRUE);
                        } else {
				ws->arch_served_map = new_map;
                                callback(obj, attr, FALSE);
                        }
			break;

		case WS_NUM_ARCH_SERVED:
			ws->num_arch_served = (int)*avlist++;
			callback(obj, attr, FALSE);
			break;

                case WS_ARCH_SERVED_ARRAY:
                        n = (int)*avlist++;
                        if (n == SETUP_APPEND) {
                                n = ws->num_arch_served++;
                        }
                        ws->arch_served_array[n] = (Arch_served) *avlist++;
			callback(obj, attr, FALSE);
                        break;

		case WS_SERVED_NDHARD:
			ws->nd_hard = (Hard_partition)*avlist++;
			callback(obj, attr, FALSE);
			break;

		case WS_SERVED_HOMEDIRHARD:
			ws->homedir_hard = (Hard_partition)*avlist++;
			callback(obj, attr, FALSE);
			break;

		case WS_SERVED_ROOTHARD:
			ws->root_hard = (Hard_partition)*avlist++;
			callback(obj, attr, FALSE);
			break;

		case WS_TYPE:
			type = (Workstation_type) *avlist++;
			switch(ws->type) {

			case WORKSTATION_NONE:
				ws->type = type;
				if (ws->type == WORKSTATION_STANDALONE) {
					allocate_standalone_arch();
				} else if (ws->type == WORKSTATION_SERVER) {
					ws->arch_served_map |= 
					    (1 << ord(ws->info.cpu_arch));
					add_arch_served(ord(ws->info.cpu_arch));
					callback(obj, WS_ARCH_SERVED, FALSE);
				}
				callback(obj, attr, FALSE);
				break;

			case WORKSTATION_STANDALONE:
				ws->type = type;
				if (ws->type == WORKSTATION_NONE) {
					deallocate_standalone_arch(ws);
				} else if (ws->type == WORKSTATION_SERVER) {
					deallocate_standalone_arch(ws);
					ws->arch_served_map |= 
					    (1 << ord(ws->info.cpu_arch));
					add_arch_served(ord(ws->info.cpu_arch));
					callback(obj, WS_ARCH_SERVED, FALSE);
				}
				callback(obj, attr, FALSE);
				break;

			case WORKSTATION_SERVER:
				if (ws->num_clients && 
				    (type != WORKSTATION_SERVER)) {
					runtime_message(SETUP_ESERVERWCLNT);
					callback(obj, attr, TRUE);
					break;
				}
				ws->type = type;
				if (ws->type == WORKSTATION_NONE) {
					deallocate_servers_archs(ws);
				} else if (ws->type == WORKSTATION_STANDALONE) {
					deallocate_servers_archs(ws);
					allocate_standalone_arch();
				}
				callback(obj, attr, FALSE);
				break;

			}

			break;

		case WS_TAPE_TYPE:
			ws->tape_type = (int)*avlist++;
			callback(obj, attr, FALSE);
			break;

		case WS_TAPE_LOC:
			ws->tape_loc = (Tape_loc)*avlist++;
			callback(obj, attr, FALSE);
			break;

		case WS_TAPE_SERVER:
			ptr = (char *)*avlist++;
                        if (legal_client_name(ptr, ws->tape_server)) {
				callback(obj, attr, TRUE);
                        } else {
                                ws->tape_server = strdup(ptr);
				callback(obj, attr, FALSE);
                        }
			break;

		case WS_HOST_INTERNET_NUMBER:
			ptr = (char *)*avlist++;
			ws->tape_server_inet = (char *) strdup(ptr);
			callback(obj, attr, FALSE);
			break;

		case WS_CONTROLLER:
			n = (int)*avlist++;
			if (n == SETUP_APPEND) {
				n = ws->num_controllers++;
			}
			ws->controllers[n] = (Controller) *avlist++;
			callback(obj, attr, FALSE);
			break;

	       case SETUP_CHOICE_STRING:
			config_index = (int) *avlist++;
			n = (int) *avlist++;
			ptr = (char *)*avlist++;
			if (ptr) {
				ws->configs[config_index][n] = strdup(ptr);
			} else {
				ws->configs[config_index][n] = NULL;
			}
			break;
			
 		case WS_CLIENT:
			n = (int) *avlist++;
			if (n == SETUP_APPEND) {
			    n = ws->num_clients++;
			    if (n == WS_MAX_CLIENTS) {
			       runtime_error( "Maximum number of clients is %d.",
				    WS_MAX_CLIENTS);
				n = WS_MAX_CLIENTS - 1;
			    }
			}
			ws->clients[n] = (Client) *avlist++;
			callback(obj, attr, FALSE);
			break;

 		case WS_CARD:
			n = (int) *avlist++;
			if (n == SETUP_APPEND) {
			    n = ws->num_cards++;
			    if (n == WS_MAX_CARDS) {
			       runtime_error( "Maximum number of cards is %d.",
				    WS_MAX_CARDS);
				n = WS_MAX_CARDS - 1;
			    }
			}
			ws->cards[n] = (Card) *avlist++;
			callback(obj, attr, FALSE);
			break;

                case WS_OSWG:
                        n = (int) *avlist++;
                        if (n == SETUP_APPEND) {
                            n = ws->num_oswgs++;
                            if (n == WS_MAX_OSWGS) {
                            	runtime_error(
				    "Maximum # of optional s/w items is %d.", 
	                             WS_MAX_OSWGS);
                                n = WS_MAX_OSWGS - 1;
                            }   
                        }   
                        ws->oswgs[n] = (Oswg) *avlist++;
			callback(obj, attr, FALSE);
                        break;

		case WS_DEFAULT_CARD:
			ws->default_card = (Card) *avlist++;
			callback(obj, attr, FALSE);
			break;

		case PARAM_DISK_DISPLAY_UNITS:
			if ((Disk_display_units)*avlist != 
			    ws->params.disk_display) {
				ws->params.disk_display = 
				    (Disk_display_units)*avlist;
				display_units_changed();
			}
			avlist++;
			callback(obj, attr, FALSE);
			break;

		case PARAM_AUTOHOST:
			ws->params.autohost = (int) *avlist++;
			callback(obj, attr, FALSE);
			break;

		case PARAM_FIRSTHOST:
			n = (int) *avlist++;
			if (n == ws->params.firsthost) {
				callback(obj, attr, FALSE);
			}
			max_host_number = (1 << ord(ws->class)) - 2;
                        if ((n < MIN_HOST_NUMBER) || (n > max_host_number)) { 
				callback(obj, attr, TRUE);
                        } else {
				ws->params.firsthost = n;
				callback(obj, attr, FALSE);
                        }
			break;

		case PARAM_FIRSTHOST_STRING_LEFT:
			ptr = *avlist++;
			n = atoi(ptr);
			if (n == ws->params.firsthost) {
				callback(obj, attr, FALSE);
			}
			max_host_number = (1 << ord(ws->class)) - 2;
                        if ((n < MIN_HOST_NUMBER) || (n > max_host_number)) { 
				callback(obj, attr, TRUE);
                        } else {
				ws->params.firsthost = n;
				callback(obj, attr, FALSE);
                        }
			break;

		case PARAM_DEFAULT_OSWG:
			ws->params.default_oswg = (Map_t) *avlist++;
			callback(obj, attr, FALSE);
			break;
		
		case SETUP_MESSAGE_PROC:
			ws->message = (Voidfunc) *avlist++;
			break;
		
		case SETUP_CONFIRM_PROC:
			ws->confirm = (Boolfunc) *avlist++;
			break;

		case SETUP_CONTINUE_PROC:
			ws->contfunc = (Voidfunc) *avlist++;
			break;   

		case SETUP_UPGRADE:
			ws->upgrade = (int) *avlist++;
			break;   

		case WS_YPTYPE:
			ws->yp_type = (Yp_type) *avlist++;
			callback(obj, attr, FALSE);
			break;

		case WS_YPMASTER_NAME:
			ptr = (char *)*avlist++;
                        if (legal_client_name(ptr, ws->yp_master_name)) {
				callback(obj, attr, TRUE);
                        } else {
				ws->yp_master_name = strdup(ptr);
				callback(obj, attr, FALSE);
                        }
			break;

		case WS_YPMASTER_INTERNET:
			ptr = (char *)*avlist++;
			ws->yp_master_internet = strdup(ptr);
			callback(obj, attr, FALSE);
			break;

		case WS_PRESERVED:
			ws->preserved = (int) *avlist++;
			callback(obj, attr, FALSE);
			break;

		case WS_MAILTYPE:
			ws->mail_type = (Mail_type) *avlist++;
			callback(obj, attr, FALSE);
			break;

		case WS_MOVE_HARD_PART:
			hp = (Hard_partition) *avlist++;
			n = (int) *avlist++;
			/*
			 * callback on HARD_TYPE to force the error message
			 * to get displayed
			 */
			if (move_hp(hp, n)) {
				callback(hp, HARD_TYPE, TRUE);
			}
			break;

		case WS_CONSOLE_FD:
			ws->console_fd = (int) *avlist++;
			break;
			
		default:
			avlist = attr_skip(attr, avlist);
			break;

		}
	}
}


/* VARARGS */
static caddr_t 
workstation_get(obj, attr, op1, op2)
Setup_object		*obj;
Setup_attribute		attr;
caddr_t			op1;
caddr_t			op2;
{
	Workstation_info	*ws;
	Controller		controller;
	Disk			disk;
	Hard_partition		hp;
    	register int		i, cont_index, disk_index, hp_index, nd_index;
	Hard_type		type;
	char			buffer[32];

	ws = SETUP_GET_DATA(obj, Workstation_info);
	
	switch (attr) {
	case WS_DOMAIN:
		return ((caddr_t) ws->domain);
		break;

	case WS_NETWORK:
		return ((caddr_t) ws->network);
		break;

	case WS_NETWORK_CLASS:
		return ((caddr_t) ws->class);
		break;

	case WS_NAME:
		return((caddr_t)ws->info.name);
		break;

	case WS_HOST_NUMBER:
		return((caddr_t)ws->info.host_number);
		break;

	case WS_MODEL:
		return ((caddr_t)ws->info.model);
		break;

	case WS_ARCH:
		return ((caddr_t)ws->info.cpu_arch);
		break;

	case WS_ETHERTYPE:
		return ((caddr_t)ws->ethertype);
		break;

	case WS_ARCH_SERVED:
		return ((caddr_t) ws->arch_served_map);
		break;

	case WS_NUM_ARCH_SERVED:
		return ((caddr_t) ws->num_arch_served);
		break;

	case WS_ARCH_SERVED_ARRAY:
		if ((int)op1 >= ws->num_arch_served) {
			return ((caddr_t) 0);
		} else {
			return ((caddr_t) ws->arch_served_array[(int)op1]);
		}
		break;

	case WS_SERVED_NDHARD:
		return ((caddr_t)ws->nd_hard);
		break;

	case WS_SERVED_HOMEDIRHARD:
		return ((caddr_t)ws->homedir_hard);
		break;

	case WS_SERVED_ROOTHARD:
		return ((caddr_t)ws->root_hard);
		break;

	case WS_TYPE:
		return ((caddr_t)ws->type);
		break;

	case WS_TAPE_TYPE:
		return ((caddr_t)ws->tape_type);
		break;

	case WS_TAPE_LOC:
		return ((caddr_t)ws->tape_loc);
		break;

	case WS_TAPE_SERVER:
		return ((caddr_t)ws->tape_server);
		break;

	case WS_HOST_INTERNET_NUMBER:
		return ((caddr_t)ws->tape_server_inet);
		break;

	case WS_NUM_CONTROLLERS:
		return ((caddr_t) ws->num_controllers);
		break;

	case WS_CONTROLLER:
		if ((int)op1 >= ws->num_controllers) {
			return ((caddr_t) 0);
		} else {
			return ((caddr_t) ws->controllers[(int)op1]);
		}
		break;

       case WS_DISK_INDEX:	/* index of op1 in array of disks */
		i = 0;
		SETUP_FOREACH_OBJECT(workstation, WS_CONTROLLER, 
	            cont_index, controller)
			SETUP_FOREACH_OBJECT(controller, CONTROLLER_DISK, 
			    disk_index, disk)
				if (disk == op1)
				    return ((caddr_t) i);
				i++;
			SETUP_END_FOREACH
		SETUP_END_FOREACH
		return ((caddr_t) 0);
		break;

       case SETUP_CHOICE_STRING:
		return ((caddr_t) config_string(ws->configs, (int)op1, 
		    (int)op2));
		break;

	case WS_CLIENT:
		if ((int)op1 >= ws->num_clients) {
		    return ((caddr_t) 0);
		} else {
		    return ((caddr_t) ws->clients[(int)op1]);
		}
		break;

	case WS_CARD:
		if ((int)op1 >= ws->num_cards) {
		    return ((caddr_t) 0);
		} else {
		    return ((caddr_t) ws->cards[(int)op1]);
		}
		break;

	case WS_DEFAULT_CARD:
	    return ((caddr_t) ws->default_card);
	    break;

	case WS_OSWG:
		if ((int)op1 >= ws->num_oswgs) {
		    return ((caddr_t) 0);
		} else {
		    return ((caddr_t) ws->oswgs[(int)op1]);
		}
		break;
		
	 case WS_CLIENT_NAME:
		for (i = 0; i < ws->num_clients; i++) {
		    if (strcmp((char *)op1, (char *) setup_get(ws->clients[i], 
		        CLIENT_NAME)) == 0)
			       return((caddr_t)ws->clients[i]);
		}
		return((caddr_t)NULL);
		break;

	 case WS_CARD_NAME:
	 case WS_CARD_NAME_TO_DEFAULT:
		for (i = 0; i < ws->num_cards; i++) {
			if (streq((char *)op1, 
			    (char *)setup_get(ws->cards[i], CLIENT_NAME))) {
			       return((caddr_t)ws->cards[i]);
			}
		}
		/*
		 * if we were trying to change the default card and we
		 * couldnt find it then force an error message by calling
		 * back using WS_NAME
		 */
		if (attr == WS_CARD_NAME_TO_DEFAULT) { 
			runtime_message(SETUP_ENOCARD, (char *)op1);
			callback(obj, WS_NAME, TRUE);
		}
		return((caddr_t)NULL);
		break;

	case WS_ND_PARTITION:
		nd_index = (int) op1;
		i = 0;
		SETUP_FOREACH_OBJECT(workstation, WS_CONTROLLER, 
	            cont_index, controller) {
			SETUP_FOREACH_OBJECT(controller, CONTROLLER_DISK, 
			    disk_index, disk) {
				SETUP_FOREACH_OBJECT(disk, DISK_HARD_PARTITION,
				    hp_index, hp) {
					type = (Hard_type)privledged_setup_get(
					    hp, HARD_TYPE);
					if (type == HARD_ND)  {
						if (i++ == nd_index) {
							return ((caddr_t) hp);
						}
					}
				} SETUP_END_FOREACH
			} SETUP_END_FOREACH
		} SETUP_END_FOREACH
		return ((caddr_t) 0);
		break;

	case PARAM_DISK_DISPLAY_UNITS:
		return((caddr_t)ws->params.disk_display);
		break;

	case PARAM_AUTOHOST:
		return((caddr_t)ws->params.autohost);
		break;

	case PARAM_FIRSTHOST:
		return((caddr_t)ws->params.firsthost);
		break;

	case PARAM_FIRSTHOST_STRING_LEFT:
                sprintf(buffer, "%d", ws->params.firsthost);
		return((caddr_t) buffer);
		break;

	case PARAM_DEFAULT_OSWG:
		return((caddr_t)ws->params.default_oswg);
		break;
		
	case SETUP_MESSAGE_PROC:
		return((caddr_t)ws->message);
		break;
		
	case SETUP_CONFIRM_PROC:
		return((caddr_t)ws->confirm);
		break;
		
	case SETUP_CONTINUE_PROC:
		return((caddr_t)ws->contfunc);
		break;

	case SETUP_UPGRADE:
		return((caddr_t)ws->upgrade);
		break;

	case WS_YPTYPE:
		return((caddr_t)ws->yp_type);
		break;

	case WS_YPMASTER_NAME:
		return((caddr_t)ws->yp_master_name);
		break; 

	case WS_YPMASTER_INTERNET:   
		return((caddr_t)ws->yp_master_internet);
		break; 

	case WS_PRESERVED:   
		return((caddr_t)ws->preserved);
		break; 

	case WS_MAILTYPE:   
		return((caddr_t)ws->mail_type);
		break; 

	case WS_CONSOLE_FD:
		return((caddr_t)ws->console_fd);
		break;

/*
	case WS_GET_CONSOLE_MESSAGES:
		get_console_message();
		return;
		break;
*/
                        
	default:
                runtime_error("Unknown workstation attribute.");
                break;

	}
}


static void
workstation_destroy(obj, avlist)
Setup_object		*obj;
caddr_t			*avlist;
{   
    Workstation_info	*ws;
    Setup_attribute	attr;
    register	int	i;
    register	char	*client_name, *card_name;
    
    ws = SETUP_GET_DATA(obj, Workstation_info);
    
    while(attr = (Setup_attribute) *avlist++) {
	switch (attr) {
	
	    /* 
	     * Destroy a client by name not by a Client handle because
	     * the attribute WS_CLIENT is of type INT_PAIR and to destroy
	     * a client only requires one argument.
	     */
	  case WS_CLIENT_NAME:
	    client_name = (char *) *avlist++;
	    for (i = 0; i < ws->num_clients; i++) {
		if (strcmp((char *) client_name, 
			   (char *) setup_get(ws->clients[i], CLIENT_NAME)) == 0) {
		    setup_destroy(ws->clients[i], 0);
		    ws->num_clients--;
		    for (; i < ws->num_clients; i++) {
			ws->clients[i] = ws->clients[i + 1];
		    }
		    break;
		}
	    }
	    break;
	    
	  case WS_CARD_NAME:
	    card_name = (Card) *avlist++;
	    for (i = 0; i < ws->num_cards; i++) {
		if (strcmp((char *) card_name, 
			   (char *) setup_get(ws->cards[i], CLIENT_NAME)) == 0) {
		    if (ws->default_card == ws->cards[i])
			ws->default_card = (Card) 0;

		    setup_destroy(ws->cards[i], 0);
		    ws->num_cards--;
		    for (; i < ws->num_cards; i++) {
			ws->cards[i] = ws->cards[i + 1];
		    }
		    break;
		}
	    }
	    break;
	    
	  default:
	    avlist = attr_skip(attr, avlist);
	    break;
	}
    }
}


allocate_standalone_arch()
{
	Controller		cont;
	Disk			disk;
	int			i;
	Hard_partition		hp;
	Arch_served		arch;
	Arch_type		ws_type;
	Hard_reserved_type	init_type;
	char			*arch_name;

        cont = setup_get(workstation, WS_CONTROLLER, 0);
        disk = setup_get(cont, CONTROLLER_DISK, 0);

	zero_unused_partitions(disk);

	ws_type = (Arch_type) setup_get(workstation, WS_ARCH);
	init_type = HARD_RESERVED_STANDALONE_USR;

	hp = hard_partition_initialization(init_type, disk);
	setup_set(hp, HARD_PARAM_FREEHOG, TRUE, 0);
	float_disk(disk);

	arch_name = setup_get(workstation, SETUP_CHOICE_STRING,
		CONFIG_CPU, ws_type);
	arch = (Arch_served)setup_create(ARCH_SERVED,
		ARCH_SERVED_NAME, arch_name,
		ARCH_SERVED_TYPE, ws_type,
		ARCH_SERVED_USRHARD, hp,
		0);

	setup_set(workstation, WS_ARCH_SERVED_ARRAY,
			SETUP_APPEND,  arch,
			0);
}


deallocate_standalone_arch(ws)
Workstation_info	*ws;
{
	Arch_served	arch;
	Hard_partition	hp;

	arch = ws->arch_served_array[0];
	hard_partition_free((Hard_partition)setup_get(arch,ARCH_SERVED_USRHARD));
	setup_destroy(arch, 0);

	ws->num_arch_served = 0;
	ws->arch_served_map = 0;
}


deallocate_servers_archs(ws)
Workstation_info	*ws;
{
	int	i;

	/*
	 * iterate over the arch's in reverse order because 
	 * remove_arch_served() deletes arch's from the arch
	 * array and compacts it
	 */
	for (i = ws->num_arch_served; i > 0; i--) {
		remove_arch_served(i - 1);
	}
	ws->num_arch_served = 0;
	ws->arch_served_map = 0;
	clear_server_hp();
}


/* return the print string for
 * a particular element of a config group.
 * The CARD config info is special cased to
 * deal with the UNSPECIFIED and NO options.
 */
static char *
config_string(configs, group, element)
Config_array	configs;
int		group, element;
{
    Hard_partition	hp;

    switch (group) {
	case CONFIG_CARD_CPU:
	    switch (element) {
		case CARD_UNSPECIFIED_CPU:
		    return "Don't Apply";

		default:
		    group = (int) CONFIG_CPU;
		    element -= CARD_TO_CLIENT_CPU;
		    return configs[group][element];
		    break;
	    }
	    break;
	
	case CONFIG_CARD_ND:
	    switch (element) {
		case CARD_UNSPECIFIED_ND:
		    return "Don't Apply";

		case CARD_FIRST_FIT_ND:
		    return "First Fit";

		default:
		    element -= CARD_TO_CLIENT_ND;
		    hp = setup_get(workstation, WS_ND_PARTITION, element);
		    return hp ? (char *) setup_get(hp, HARD_NAME) : NULL;
	    }
   
	default:
	    return configs[group][element];
    }
}


move_hp(old_hp, hp_index)
Hard_partition	old_hp;
int		hp_index;
{
	Hard_partition		find_hp_from_index(), new_hp;
	Disk			old_disk;
	Disk			new_disk;
	Hard_reserved_type      old_res_type;
	Hard_type		old_type;
	char			*old_mount_point;
	int			old_min_size;
	int			new_min_size;
	int			old_size;
	int			new_size;
	int			freehog;
	Hard_partition_info	*old_hp_info;
	
	old_disk = (Disk)setup_get(old_hp, HARD_DISK);
	new_hp = find_hp_from_index(hp_index);
	new_disk = (Disk)setup_get(new_hp, HARD_DISK);

	old_type = (Hard_type)setup_get(new_hp, HARD_TYPE);
	if (old_type != HARD_FREE) {
		runtime_message(SETUP_EMOVENOTFREE,
			(char *)setup_get(old_hp, HARD_NAME),
			(char *)setup_get(new_hp, HARD_NAME),
			(char *)setup_get(new_hp, HARD_NAME));
		return(TRUE);
	}

	freehog = (int)setup_get(old_hp, HARD_PARAM_FREEHOG);
	if (freehog) {
		if (! ((int)setup_get(new_disk, DISK_PARAM_FLOATING))) {
		runtime_message(SETUP_EMOVENOTFLOAT,
			(char *)setup_get(old_hp, HARD_NAME),
			(char *)setup_get(new_hp, HARD_NAME),
			(char *)setup_get(new_hp, HARD_NAME));
			return(TRUE);
		}
		/*
		 * set the free hog of the source disk to NONE if it
		 * different from the destination disk, otherwise
		 * we will move the free hog below
		 */
		if (old_disk != new_disk) {
			setup_set(old_disk,
				DISK_FREE_HOG_INDEX, 0,
				0);
		}
	}

	old_res_type = (Hard_reserved_type)setup_get(old_hp, HARD_RESERVED_TYPE);
	old_type = (Hard_type)setup_get(old_hp, HARD_TYPE);
	old_size = (int)setup_get(old_hp, HARD_SIZE);
	new_size = (int)setup_get(new_hp, HARD_SIZE);
	old_min_size = (int)setup_get(old_hp, HARD_MIN_SIZE);
	new_min_size = (int)setup_get(new_hp, HARD_MIN_SIZE);
	old_mount_point = (char *)setup_get(old_hp, HARD_MOUNT_PT);

	if (! ((int)setup_get(new_hp, HARD_TEST_FIT, (old_size - new_size)))) {
		runtime_message(SETUP_EMOVENOSPACE,
			(char *)setup_get(old_hp, HARD_NAME),
			(char *)setup_get(new_hp, HARD_NAME),
			(char *)setup_get(new_hp, HARD_NAME));
		return(TRUE);
	}

	privledged_setup_set(new_hp,
		HARD_RESERVED_TYPE, old_res_type,
		HARD_TYPE, old_type,
		HARD_MOUNT_PT, old_mount_point,
		HARD_SIZE, old_size,
		HARD_MIN_SIZE, (old_min_size - new_min_size),
		0);

	move_hp_ptrs(old_res_type, new_hp);

	if ((int)setup_get(old_hp, HARD_NUM_SOFT_PARTITION)) {
		copy_soft_partitions(old_hp, new_hp);
	}

	privledged_setup_set(old_hp,
		HARD_RESERVED_TYPE, HARD_RESERVED_NONE,
		HARD_TYPE, HARD_FREE,
		HARD_SIZE, 0,
		0);


	if (freehog) {
		/*
		 * set the free hog of the destination disk to new partition
		 * (DISK_FREE_HOG_INDEX starts at 0 for NONE, 1 - 9 for hp's)
		 */
		setup_set(new_disk,
			DISK_FREE_HOG_INDEX, 
			    ((int)setup_get(new_hp, HARD_INDEX) + 1),
			0);
	}

	return(FALSE);

}


Hard_partition
find_hp_from_index(index)
{
        Controller      	cont;
        Disk            	disk;
        Hard_partition  	hp;
        int             	i, j, k;
	int			count = 0;

        SETUP_FOREACH_OBJECT(workstation, WS_CONTROLLER, i, cont) {
                SETUP_FOREACH_OBJECT(cont, CONTROLLER_DISK, j, disk) {
                        SETUP_FOREACH_OBJECT(disk, DISK_HARD_PARTITION, k, hp) {
                                if (index == count) {
                                        return(hp);
                                }
                                count++;
                        } SETUP_END_FOREACH
                } SETUP_END_FOREACH
        } SETUP_END_FOREACH

	return(NULL);
}


copy_soft_partitions(from_hp, to_hp)
Hard_partition	from_hp;
Hard_partition	to_hp;
{
	int		i;
	Soft_partition	sp;

	SETUP_FOREACH_OBJECT(from_hp, HARD_SOFT_PARTITION, i, sp) {
		setup_set(to_hp, HARD_SOFT_PARTITION, sp, 0);
		setup_set(sp, SOFT_HARD_PARTITION, to_hp, 0);
	} SETUP_END_FOREACH

}


move_hp_ptrs(res_type, hp)
Hard_reserved_type      res_type;
Hard_partition		hp;
{
	Arch_served     	arch;
	int			i;
	Hard_partition		nhp;
	Hard_reserved_type      type;
	Setup_attribute         attr;

	switch (res_type) {

	case HARD_RESERVED_STANDALONE_USR:
		arch = (Arch_served)setup_get(workstation, 
		    WS_ARCH_SERVED_ARRAY, 0);
		setup_set(arch,
			ARCH_SERVED_USRHARD, hp,
			0);
		return;

	case HARD_RESERVED_SERVERHOMEDIRS:
		setup_set(workstation,
			WS_SERVED_HOMEDIRHARD, hp,
			0);
		return;

	case HARD_RESERVED_PUB010:
	case HARD_RESERVED_PUB020:
		attr = ARCH_SERVED_PUBHARD;
		break;

	case HARD_RESERVED_USR010:
	case HARD_RESERVED_USR020:
		attr = ARCH_SERVED_USRHARD;
		break;

	}

	SETUP_FOREACH_OBJECT(workstation, WS_ARCH_SERVED_ARRAY, i, arch) {
		nhp = (Hard_partition) setup_get(arch, attr);
		type = (Hard_reserved_type) setup_get(nhp, HARD_RESERVED_TYPE);
		if (type == res_type) {
			setup_set(arch,
				attr, hp,
				0);
			return;
		}
	} SETUP_END_FOREACH
	runtime_error("Evan says: put in an error message.");

}
