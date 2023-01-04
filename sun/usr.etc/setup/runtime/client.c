#ifndef lint
static	char sccsid[] = "@(#)client.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */


#include "setup_runtime.h"

/* slots zero and one in the client's
 * soft_parts[] array are used for the 
 * root and swap partitions.
 */
#define	ROOT_INDEX	0
#define	SWAP_INDEX	1


static	void	client_set();
static	caddr_t	client_get();
static	void	client_destroy();
static	void	client_change_part_index();

Client 
client_create(obj)
Setup_object	*obj;
{   
	register Client_info	*client;
	
	obj->data = (caddr_t) new(Client_info);
	obj->type = SETUP_CLIENT;
	obj->set_attr = client_set;
	obj->get_attr = client_get;
	obj->destroy  = client_destroy;
	
	client = SETUP_GET_DATA(obj, Client_info);

	client->name		= strdup("");
        SET_ATTR_BIT(obj->status, CLIENT_NAME);
	client->ether_addr	= strdup("");
        SET_ATTR_BIT(obj->status, CLIENT_E_ADDR);
	client->host_number	= strdup("");
        SET_ATTR_BIT(obj->status, CLIENT_HOST_NUMBER);
	client->model		= 0;
	client->cpu_arch	= MC68010;
	client->three_com_interface = FALSE;

        SET_ATTR_BIT(obj->status, CLIENT_ROOT_SIZE_STRING_LEFT);
        SET_ATTR_BIT(obj->status, CLIENT_SWAP_SIZE_STRING_LEFT);

	client->soft_parts[ROOT_INDEX] = 
	    setup_create(SOFT_PARTITION, 
	                 SOFT_CLIENT, obj,
                         SOFT_TYPE, SOFT_ROOT,
			 SOFT_HARD_PARTITION, 
			     setup_get(workstation, WS_SERVED_NDHARD),
			 0);
	setup_set(setup_get(workstation, WS_SERVED_NDHARD), 
			HARD_SOFT_PARTITION, SETUP_APPEND, 
			client->soft_parts[ROOT_INDEX], 0);


	client->soft_parts[SWAP_INDEX] = 
	    setup_create(SOFT_PARTITION, 
	                 SOFT_CLIENT, obj,
                         SOFT_TYPE, SOFT_SWAP,
			 SOFT_HARD_PARTITION, 
			     setup_get(workstation, WS_SERVED_NDHARD),
			 0);
	setup_set(setup_get(workstation, WS_SERVED_NDHARD), 
			HARD_SOFT_PARTITION, SETUP_APPEND, 
			client->soft_parts[SWAP_INDEX], 0);

	
	/* apply the default config card */
	card_apply((Client) obj, (Card) setup_get(workstation, WS_DEFAULT_CARD));

	/*
	 * if autohost number is turned on then assign a host number
	 */
	if ((int)setup_get(workstation, PARAM_AUTOHOST)) {
		setup_set(obj,
			CLIENT_HOST_NUMBER, "",
                	0);
	}

}
	


static
void
client_set(obj, avlist)
Setup_object	*obj;
Avlist		avlist;
{
	Client_info	*client;
	Setup_attribute	attr;
	char		*ptr;
	char		*host_number;
	int		nd_index;
	Hard_partition	hp;
	Soft_partition	soft;
	Map_t		arch_served;
	Arch_type	arch;
	int		n;
	
	client = SETUP_GET_DATA(obj, Client_info);
	
	while(attr = (Setup_attribute) *avlist++) {
		switch (attr) {

		case CLIENT_NAME:
			ptr = (char *) *avlist++;
			if (legal_client_name(ptr, client->name)) {
				callback(obj, attr, TRUE);
			} else {
				client->name = strdup(ptr);
				callback(obj, attr, FALSE);
			}
			break;

		case CLIENT_E_ADDR:
			ptr = (char *) *avlist++;
			if (legal_ethernet_addr(ptr, client->ether_addr)) {
				callback(obj, attr, TRUE);
			} else {
				client->ether_addr = strdup(ptr);
				callback(obj, attr, FALSE);
			}
			break;

		case CLIENT_HOST_NUMBER:
                        /* 
                         * set host number to the null string so that
                         * legal_host_number doesnt find a dup 
                         */ 
			client->host_number = strdup("");
			ptr = (char *) *avlist++;
                        if ((host_number = legal_host_number(ptr)) == NULL) {
				callback(obj, attr, TRUE);
                        } else {
				client->host_number = host_number;
				callback(obj, attr, FALSE);
                        }
			break;

		case CLIENT_MODEL:
                        client->model = (int)*avlist++;
			callback(obj, attr, FALSE);
			break;

		case CLIENT_ARCH:
                        arch = (Arch_type)*avlist++;
			arch_served = (Map_t)setup_get(workstation,
			    WS_ARCH_SERVED);
			if (is_in_map(arch_served, ord(arch))) {
				client->cpu_arch = arch;
				callback(obj, attr, FALSE);
			} else {
				runtime_message(SETUP_EARCHNOTSERVED, 
	    			    (char *)setup_get(workstation, 
				    SETUP_CHOICE_STRING, CONFIG_CPU, ord(arch)));
				callback(obj, attr, TRUE);
			}
			break;

		case CLIENT_ROOT_SIZE:
			n = (int)*avlist++;
			if (resize_soft_part(client->soft_parts[ROOT_INDEX],&n)){
				callback(obj, attr, TRUE);
			} else {
				setup_set(client->soft_parts[ROOT_INDEX],
					  SOFT_SIZE, n,
					  0);
				callback(obj, CLIENT_ROOT_SIZE_STRING, FALSE);
				callback(obj,CLIENT_ROOT_SIZE_STRING_LEFT,FALSE);
			}
			break;

		case CLIENT_ROOT_SIZE_STRING:
		case CLIENT_ROOT_SIZE_STRING_LEFT:
			ptr = (char *)*avlist++;
			hp = setup_get(client->soft_parts[ROOT_INDEX], 
				SOFT_HARD_PARTITION);
			n = units_to_sectors(hp, ptr);
			if (resize_soft_part(client->soft_parts[ROOT_INDEX],&n)){
				callback(obj, attr, TRUE);
			} else {
				setup_set(client->soft_parts[ROOT_INDEX],
					  SOFT_SIZE, 	n,
					  0);
				callback(obj, CLIENT_ROOT_SIZE_STRING, FALSE);
				callback(obj,CLIENT_ROOT_SIZE_STRING_LEFT,FALSE);
			}
			break;

		case CLIENT_ROOT_PARTITION_INDEX:
			client_change_part_index(obj, attr, (int) *avlist++, 
			     ROOT_INDEX);
			callback(obj, CLIENT_ROOT_SIZE_STRING, FALSE);
			callback(obj,CLIENT_ROOT_SIZE_STRING_LEFT,FALSE);
			break;

		case CLIENT_SWAP_SIZE:
			n = (int)*avlist++;
			if (resize_soft_part(client->soft_parts[SWAP_INDEX],&n)){
				callback(obj, attr, TRUE);
			} else {
				setup_set(client->soft_parts[SWAP_INDEX],
					  SOFT_SIZE, n,
					  0);
				callback(obj, CLIENT_SWAP_SIZE_STRING, FALSE);
				callback(obj,CLIENT_SWAP_SIZE_STRING_LEFT,FALSE);
			}
			break;

		case CLIENT_SWAP_SIZE_STRING:
		case CLIENT_SWAP_SIZE_STRING_LEFT:
			ptr = (char *)*avlist++;
			hp = setup_get(client->soft_parts[SWAP_INDEX], 
				SOFT_HARD_PARTITION);
			n = units_to_sectors(hp, ptr);
			if (resize_soft_part(client->soft_parts[SWAP_INDEX],&n)){
				callback(obj, attr, TRUE);
			} else {
				setup_set(client->soft_parts[SWAP_INDEX],
					  SOFT_SIZE, 	n,
					  0);
				callback(obj, CLIENT_SWAP_SIZE_STRING, FALSE);
				callback(obj,CLIENT_SWAP_SIZE_STRING_LEFT,FALSE);
			}
			break;

		case CLIENT_SWAP_PARTITION_INDEX:
			client_change_part_index(obj, attr, (int) *avlist++, 
			     SWAP_INDEX);
			callback(obj, CLIENT_SWAP_SIZE_STRING, FALSE);
			callback(obj,CLIENT_SWAP_SIZE_STRING_LEFT,FALSE);
			break;
			
		case CLIENT_3COM_INTERFACE:
		        client->three_com_interface = (int) *avlist++;
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
client_get(obj, attr, op1, op2)
Setup_object		*obj;
Setup_attribute		attr;
caddr_t			op1;
caddr_t			op2;
{
	Client_info		*client;   
	Hard_partition		hp;
	
	client = SETUP_GET_DATA(obj, Client_info);
	
	switch (attr) {

	case CLIENT_NAME:
		return ((caddr_t) client->name);
		break;
 
	case CLIENT_E_ADDR:
		return ((caddr_t)client->ether_addr);
		break;

	case CLIENT_HOST_NUMBER:
		return ((caddr_t)client->host_number);
		break;

	case CLIENT_MODEL:
		return ((caddr_t)client->model);
		break;

	case CLIENT_ARCH:
		return ((caddr_t)client->cpu_arch);
		break;

	case CLIENT_ROOT_SIZE:
		return ((caddr_t)setup_get(client->soft_parts[ROOT_INDEX],
		    SOFT_SIZE));
		break;

	case CLIENT_ROOT_SIZE_STRING:
		return ((caddr_t)setup_get(client->soft_parts[ROOT_INDEX],
		    SOFT_SIZE_STRING));
		break;

	case CLIENT_ROOT_SIZE_STRING_LEFT:
		return ((caddr_t)setup_get(client->soft_parts[ROOT_INDEX],
		    SOFT_SIZE_STRING_LEFT));
		break;

	case CLIENT_ROOT_PARTITION:
		return ((caddr_t)client->soft_parts[ROOT_INDEX]);
		break;

	case CLIENT_ROOT_PARTITION_INDEX:
		hp = (Hard_partition) setup_get(client->soft_parts[ROOT_INDEX],
		    SOFT_HARD_PARTITION);
		return ((caddr_t) setup_get(hp, HARD_ND_INDEX));
		break;

	case CLIENT_SWAP_SIZE:
		return ((caddr_t)setup_get(client->soft_parts[SWAP_INDEX],
		    SOFT_SIZE));
		break;

	case CLIENT_SWAP_SIZE_STRING:
		return ((caddr_t)setup_get(client->soft_parts[SWAP_INDEX],
		    SOFT_SIZE_STRING));
		break;

	case CLIENT_SWAP_SIZE_STRING_LEFT:
		return ((caddr_t)setup_get(client->soft_parts[SWAP_INDEX],
		    SOFT_SIZE_STRING_LEFT));
		break;

	case CLIENT_SWAP_PARTITION:
		return ((caddr_t)client->soft_parts[SWAP_INDEX]);
		break;

	case CLIENT_SWAP_PARTITION_INDEX:
		hp = (Hard_partition) setup_get(client->soft_parts[SWAP_INDEX],
		    SOFT_HARD_PARTITION);
		return ((caddr_t) setup_get(hp, HARD_ND_INDEX));
		break;
		
	case CLIENT_3COM_INTERFACE:
		return((caddr_t) client->three_com_interface);
		break;
		
		
	default:
		runtime_error("Unknown client attribute.");
		break;
	
	}
}


static
void
client_destroy(obj, avlist)
Setup_object	*obj;
Avlist		avlist;
{   
	Client_info		*client;
	
	client = SETUP_GET_DATA(obj, Client_info);

	setup_destroy(client->soft_parts[ROOT_INDEX], 0);
	setup_destroy(client->soft_parts[SWAP_INDEX], 0);
	
	free(client);
	free(obj);
}

/*
 * Change the hard partition that a client's root or swap
 * partition lives on.
 */
static
void
client_change_part_index(obj, attr, nd_index, part_index)
Setup_object	*obj;
Setup_attribute attr;
int		nd_index;
int		part_index;
{
	Client_info	*client;
	Disk		disk;
	Hard_partition	hard;
	Soft_partition	soft;
	int		size;
	int		hp_size;

	client = SETUP_GET_DATA(obj, Client_info);
	hard = setup_get(workstation, WS_ND_PARTITION, nd_index);
	if (!hard) {
		callback(obj, attr, TRUE);
	} else {
		disk = (Disk) setup_get(hard, HARD_DISK);
		soft = client->soft_parts[part_index];
		size = (int) setup_get(soft, SOFT_SIZE);
		size = round_to_cyls(disk, size);

		if ((int)setup_get(hard, HARD_TEST_FIT, size)) {
			hp_size = (int)setup_get(hard, HARD_SIZE);
			hp_size += size;
			setup_set(hard, 
				HARD_MIN_SIZE, size,
				HARD_SIZE, hp_size, 
				0);
			/*
			 * Must add the soft partition to the hard list
			 * before changing the soft parition's size because
			 * when we set SOFT_SIZE the hp's soft partitions
			 * are re-floated
			 */
			setup_set(hard, 
			    HARD_SOFT_PARTITION, SETUP_APPEND, soft,
			    0);
			/*
			 * setting SOFT_HARD_PARTITION causes soft to be 
			 * deleted from the previous hp where it was, and 
			 * setting SOFT_SIZE forces re-floating of the soft
			 * partitions
			 */
			setup_set(soft,
			  SOFT_HARD_PARTITION,	hard,
			  SOFT_SIZE,		size,
			  0);
			callback(obj, attr, FALSE);
		} else {
			runtime_message(SETUP_ECANOTMOVEND,
			    (char *)setup_get(hard, HARD_NAME),
			    (char *)setup_get(hard, HARD_NAME));
			callback(obj, attr, TRUE);
		}
	}
}

resize_soft_part(sp, newsize)
Soft_partition	sp;
int		*newsize;
{
	Disk		disk;
	Hard_partition	hp;
	int		delta;
	int		hp_size;

	hp = (Hard_partition)setup_get(sp, SOFT_HARD_PARTITION);
	disk = (Disk)setup_get(hp, HARD_DISK);

	if (setup_get(disk, DISK_PARAM_CYLROUNDING)) {
		*newsize = round_to_cyls(disk, *newsize);
	}

	delta = (*newsize - (int)setup_get(sp, SOFT_SIZE));
	if ((int)setup_get(hp, HARD_TEST_FIT, delta)) {
		hp_size = (int)setup_get(hp, HARD_SIZE);
		hp_size += delta;
		setup_set(hp, 
			HARD_MIN_SIZE, delta,
			HARD_SIZE, hp_size, 
			0);
		return(FALSE);
	} else {
		runtime_message(SETUP_ECHANGEND);
		return(TRUE);
	}

}
