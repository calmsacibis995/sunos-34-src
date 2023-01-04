
#ifndef lint
static	char sccsid[] = "@(#)card.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "setup_runtime.h"

#define	not_changeable(obj) (TEST_ATTR_BIT(obj->status, SETUP_NOTCHANGEABLE))

static	void	card_set();
static	caddr_t	card_get();
static	void	card_destroy();

static int	card_apply_to();

Card 
card_create(obj)
Setup_object	*obj;
{   
	register		int	i;
	register	Card_info	*card;
	
	obj->data = (caddr_t) new(Card_info);
	obj->type = SETUP_CARD;
	obj->set_attr = card_set;
	obj->get_attr = card_get;
	obj->destroy  = card_destroy;
	
	card = SETUP_GET_DATA(obj, Card_info);
	card->name		= (char *)strdup("");
}


static
void
card_set(obj, avlist)
Setup_object	*obj;
Avlist		avlist;
{
	Card_info	*card;
	Setup_attribute	attr;
	char		*ptr;
	char		*host_number;
	int		nd_index;
	Hard_partition	hp;
	Soft_partition	soft;
	int		n;
	
	card = SETUP_GET_DATA(obj, Card_info);

	while(attr = (Setup_attribute) *avlist++) {

		switch (attr) {

		case CARD_APPLY_TO:
			ptr = (char *) *avlist++;
			if (card_apply_to(obj, ptr)) {
				callback(obj, attr, TRUE);
			} else {
				/* this will clear the string */
				callback(obj, attr, FALSE);
			}
			break;

		case CLIENT_NAME:
			ptr = (char *) *avlist++;
			if (not_changeable(obj)) {
				if (! (streq(ptr, card->name))) {
					runtime_message(SETUP_ENOTCHANGEABLE);
					callback(obj, attr, TRUE);
				}
				break;
			
			}
			if (legal_card_name(ptr, card->name)) {
				callback(obj, attr, TRUE);
			} else {
				card->name = (char *)strdup(ptr);
				callback(obj, attr, FALSE);
			}
			break;

		case CLIENT_ARCH:
			if (not_changeable(obj)) {
				runtime_message(SETUP_ENOTCHANGEABLE);
				callback(obj, attr, TRUE);
				break;
			}
                        card->cpu_arch = (int) *avlist++;
			callback(obj, attr, FALSE);
			break;

		case CLIENT_ROOT_SIZE:
			n = (int)*avlist++;
			if (not_changeable(obj)) {
				if (n != card->root_size) {
					runtime_message(SETUP_ENOTCHANGEABLE);
					callback(obj, attr, TRUE);
				}
				break;
			}
			/* check the size here */
			card->root_size = n;
			callback(obj, attr, FALSE);
			break;

		case CLIENT_ROOT_SIZE_STRING:
		case CLIENT_ROOT_SIZE_STRING_LEFT:
			/* XXX
			 * I am not sure this is correct, but we now need 
			 * to pass a hp to sectors_to_units, so this will 
			 * work for now -- jdf
			 */
			if ((hp = card->root_nd) == NULL) {
				hp = setup_get(workstation, WS_SERVED_NDHARD);
			}
			ptr = (char *)*avlist++;
			if (not_changeable(obj)) {
				/*
				 * convert to unit strings otherwise we have
				 * roundoff error problems between units
				 */
				if (! (streq(ptr, 
				    sectors_to_units_left(hp,card->root_size)))){
					runtime_message(SETUP_ENOTCHANGEABLE);
					callback(obj, attr, TRUE);
				}
				break;
			}
			card->root_size = units_to_sectors(hp, ptr);
			callback(obj, attr, FALSE);
			break;

		case CLIENT_ROOT_PARTITION_INDEX:
			if (not_changeable(obj)) {
				runtime_message(SETUP_ENOTCHANGEABLE);
				callback(obj, attr, TRUE);
				break;
			}
			nd_index = (int) *avlist++;
			switch (nd_index) {
			    case CARD_FIRST_FIT_ND:
			    case CARD_UNSPECIFIED_ND:
				card->root_nd_index = nd_index;
				card->root_nd = 0;
				callback(obj, attr, FALSE);
				break;

			    default:
				hp = setup_get(workstation, 
				    WS_ND_PARTITION, nd_index - CARD_TO_CLIENT_ND);
				if (!hp) {
				    callback(obj, 
					CLIENT_ROOT_PARTITION_INDEX, TRUE);
				} else {
				    card->root_nd_index = nd_index;
				    card->root_nd = hp;
				}
				callback(obj, attr, FALSE);
				break;
			}
			break;

		case CLIENT_SWAP_SIZE:
			n = (int)*avlist++;
			if (not_changeable(obj)) {
				if (n != card->swap_size) {
					runtime_message(SETUP_ENOTCHANGEABLE);
					callback(obj, attr, TRUE);
				}
				break;
			}
			/* check the size here */
			card->swap_size = n;
			callback(obj, attr, FALSE);
			break;

		case CLIENT_SWAP_SIZE_STRING:
		case CLIENT_SWAP_SIZE_STRING_LEFT:
			/* XXX
			 * I am not sure this is correct, but we now need 
			 * to pass a hp to sectors_to_units, so this will 
			 * work for now -- jdf
			 */
			if ((hp = card->swap_nd) == NULL) {
				hp = setup_get(workstation, WS_SERVED_NDHARD);
			}
			ptr = (char *)*avlist++;
			if (not_changeable(obj)) {
				/*
				 * convert to unit strings otherwise we have
				 * roundoff error problems between units
				 */
				if (! (streq(ptr, 
				    sectors_to_units_left(hp,card->swap_size)))){
					runtime_message(SETUP_ENOTCHANGEABLE);
					callback(obj, attr, TRUE);
				}
				break;
			}
			card->swap_size = units_to_sectors(hp, ptr);
			callback(obj, attr, FALSE);
			break;

		case CLIENT_SWAP_PARTITION_INDEX:
			if (not_changeable(obj)) {
				runtime_message(SETUP_ENOTCHANGEABLE);
				callback(obj, attr, TRUE);
				break;
			}
			nd_index = (int) *avlist++;
			switch (nd_index) {
			    case CARD_FIRST_FIT_ND:
			    case CARD_UNSPECIFIED_ND:
				card->swap_nd_index = nd_index;
				card->swap_nd = 0;
				callback(obj, attr, FALSE);
				break;

			    default:
				hp = setup_get(workstation, 
				    WS_ND_PARTITION, nd_index - CARD_TO_CLIENT_ND);
				if (!hp) {
				    callback(obj, 
					CLIENT_SWAP_PARTITION_INDEX, TRUE);
				} else {
				    card->swap_nd_index = nd_index;
				    card->swap_nd = hp;
				}
				callback(obj, attr, FALSE);
				break;
			}
			break;
			
		case CLIENT_3COM_INTERFACE:
			if (not_changeable(obj)) {
				runtime_message(SETUP_ENOTCHANGEABLE);
				callback(obj, attr, TRUE);
				break;
			}
			card->three_com_interface = (int) *avlist++;
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
card_get(obj, attr, op1, op2)
Setup_object		*obj;
Setup_attribute		attr;
caddr_t			op1;
caddr_t			op2;
{
	Card_info	*card;   
	Hard_partition	hp;
	
	card = SETUP_GET_DATA(obj, Card_info);
	
	switch (attr) {

	case CLIENT_NAME:
		return ((caddr_t) card->name);
		break;
 
	case CLIENT_ARCH:
		return ((caddr_t)card->cpu_arch);
		break;

	case CLIENT_ROOT_SIZE:
		return ((caddr_t)card->root_size);
		break;

	case CLIENT_ROOT_SIZE_STRING:
		/* XXX
		 * I am not sure this is correct, but we now need to pass
		 * a hp to sectors_to_units, so this will work for now -- jdf
		 */
		if ((hp = card->root_nd) == NULL) {
			hp = setup_get(workstation, WS_SERVED_NDHARD);
		}
		return ((caddr_t) (card->root_size < 0) ?
		    "" : sectors_to_units(hp, card->root_size));
		break;

	case CLIENT_ROOT_SIZE_STRING_LEFT:
		if ((hp = card->root_nd) == NULL) {
			hp = setup_get(workstation, WS_SERVED_NDHARD);
		}
		return ((caddr_t) (card->root_size < 0) ?
		    "" : sectors_to_units_left(hp, card->root_size));
		break;

	case CLIENT_ROOT_PARTITION:
		return ((caddr_t)card->root_nd);
		break;

	case CLIENT_ROOT_PARTITION_INDEX:
		return ((caddr_t) 
		    check_nd_index(card->root_nd_index, card->root_nd));
		break;

	case CLIENT_SWAP_SIZE:
		return ((caddr_t)card->swap_size);
		break;

	case CLIENT_SWAP_SIZE_STRING:
		if ((hp = card->swap_nd) == NULL) {
			hp = setup_get(workstation, WS_SERVED_NDHARD);
		}
		return ((caddr_t) (card->swap_size < 0) ?
		    "" : sectors_to_units(hp, card->swap_size));
		break;

	case CLIENT_SWAP_SIZE_STRING_LEFT:
		if ((hp = card->swap_nd) == NULL) {
			hp = setup_get(workstation, WS_SERVED_NDHARD);
		}
		return ((caddr_t) (card->swap_size < 0) ?
		    "" : sectors_to_units_left(hp, card->swap_size));
		break;

	case CLIENT_SWAP_PARTITION:
		return ((caddr_t)card->swap_nd);
		break;

	case CLIENT_SWAP_PARTITION_INDEX:
		return ((caddr_t) 
		    check_nd_index(card->swap_nd_index, card->swap_nd));
		break;

	case CARD_APPLY_TO:
		return((caddr_t) "");
		break;
		
	case CLIENT_3COM_INTERFACE:
		return((caddr_t) card->three_com_interface);
		break;
		
	default:
		runtime_error("Unknown card attribute.");
		break;
	
	}
}


static
void
card_destroy(obj, avlist)
Setup_object	*obj;
Avlist		avlist;
{   
	Card_info		*card;
	
	card = SETUP_GET_DATA(obj, Card_info);
	
	free(card);
	free(obj);
}


static int
check_nd_index(nd_index, hp)
int		nd_index;
Hard_partition	hp;
{
    Hard_type	hp_type;

    switch (nd_index) {
        case CARD_UNSPECIFIED_ND:
	case CARD_FIRST_FIT_ND:
	   return nd_index;

	default:
	  hp_type = (Hard_type) setup_get(hp, HARD_TYPE);
	  return ((hp_type != HARD_ND) ? CARD_UNSPECIFIED_ND : nd_index);
    }
}


void
card_apply(client, card)
Client	client;
Card	card;
{
    int	cpu_type, nd_index, nd_size;

    if (!card)
	return;

    cpu_type = (int) setup_get(card, CLIENT_ARCH);
    if (cpu_type != CARD_UNSPECIFIED_CPU) {
	setup_set(client, CLIENT_ARCH, cpu_type - CARD_TO_CLIENT_CPU, 0);
    }

    nd_size = (int) setup_get(card, CLIENT_ROOT_SIZE);
    if (nd_size >= 0) {
	nd_index = (int) setup_get(card, CLIENT_ROOT_PARTITION_INDEX);
	if (assign_nd(client, nd_index, nd_size, CLIENT_ROOT_PARTITION_INDEX)) {
		setup_set(client, CLIENT_ROOT_SIZE, nd_size, 0);
	} else {
                /*
                 * callback on the card CLIENT_NAME to force the error message
		 * to be displayed, we cannot be sure that a callback is
		 * yet registered for the client
                 */
		callback(card, CLIENT_NAME, TRUE);
	}
    }

    nd_size = (int) setup_get(card, CLIENT_SWAP_SIZE);
    if (nd_size >= 0) {
        nd_index = (int) setup_get(card, CLIENT_SWAP_PARTITION_INDEX);
        if (assign_nd(client, nd_index, nd_size, CLIENT_SWAP_PARTITION_INDEX)) {
		setup_set(client, CLIENT_SWAP_SIZE, nd_size, 0);
	} else {
                /*
                 * callback on the card CLIENT_NAME to force the error message
		 * to be displayed, we cannot be sure that a callback is
		 * yet registered for the client
                 */
		callback(card, CLIENT_NAME, TRUE);
	}
    }
}


static
card_apply_to(card, client_list)
Card		card;
register char	*client_list;
{
    char		name[64];
    register char	*namep;
    Client		client;

    while (*client_list) {
	/* skip leading blanks */
	while (*client_list == ' ')
	    *client_list++;

	if (!*client_list)
	    break;

	/* scan off the client name */
	namep = name;
	while (*client_list && *client_list != ',' && *client_list != ' ')
	    *namep++ = *client_list++;
	*namep = '\0';
	while (*client_list == ' ')
	    *client_list++;
	if (*client_list == ',')
	    *client_list++;

	/* lookup the client */
	client = setup_get(workstation, WS_CLIENT_NAME, name);
	if (!client) {
	    runtime_message(SETUP_ENOCLIENT, 
		name, (char *)setup_get(card, CLIENT_NAME));
	    return(TRUE);
	}

	/* apply the card */
        card_apply(client, card);
    }
    return(FALSE);
}


assign_nd(client, nd_index, size, nd_type)
Client		client;
int		nd_index;
int		size;
Setup_attribute	nd_type;
{

	Hard_partition	hp;
	int		n;

    switch (nd_index) {
        case CARD_UNSPECIFIED_ND:
	    return(FALSE);

	case CARD_FIRST_FIT_ND:
	    SETUP_FOREACH_OBJECT(workstation, WS_ND_PARTITION, n, hp) {
		if ((int)setup_get(hp, HARD_TEST_FIT, size)) {
		   setup_set(client, nd_type, n, 0);
		   return(TRUE);
		}
	    } SETUP_END_FOREACH
	    /*
	     * if we failed to fit, set the ND partition to the first
	     * partition 
	     */
	    setup_set(client, nd_type, 0, 0);
	    runtime_message(SETUP_ENOSPACEFORND);
	    return(FALSE);
	
	default:
	   nd_index -= CARD_TO_CLIENT_ND;
           setup_set(client, nd_type, nd_index, 0);
	   return(TRUE);
    }
}
