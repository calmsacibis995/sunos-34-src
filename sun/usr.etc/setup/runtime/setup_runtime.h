/*	@(#)setup_runtime.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sys/types.h>
#include "setup_attr.h"
#include "setup_message_table.h"

#define	FALSE	0
#define	TRUE	1

#define new(type)	((type *) calloc(sizeof(type), 1))
#define strdup(src)     strcpy(malloc((unsigned) strlen(src) + 1), src)
#define streq(a, b)	(strcmp(a, b) == 0)


/* assume 8 bits per byte, so byte for nth
 * element is n/8, bit within that byte is
 * defined by the loworder three bits of n.
 */
#define WORD(n)         (n >> 5)        /* word for element n */
#define BIT(n)          (n & 0x1F)      /* bit in word for element n */
 
/* Add a choice by or-ing in the correct bit.
 * Remove a choice by and-ing out the correct bit.
 */
#define SET_BIT(set, n)  ((set)[WORD(n)] |= (1 << BIT(n)))
#define CLR_BIT(set, n)  ((set)[WORD(n)] &= ~(1 << BIT(n)))

/* See if the nth bit is on */
#define TEST_BIT(set, n)  (((set)[WORD(n)] >> BIT(n)) & 01)

#define SET_ATTR_BIT(set, n) SET_BIT(set, ATTR_ORDINAL(n))
#define CLR_ATTR_BIT(set, n) CLR_BIT(set, ATTR_ORDINAL(n))
#define TEST_ATTR_BIT(set, n) TEST_BIT(set, ATTR_ORDINAL(n))


#define	HARD_MAX_SOFT_PARTS	40
#define	CLIENT_MAX_SOFT_PARTS	2
#define	WS_MAX_CONTROLLERS	4
#define	WS_MAX_CLIENTS		20
#define	WS_MAX_CARDS		32
#define	WS_MAX_OSWGS		32
#define	DISK_NUM_HARD_PARTS	8
#define CONFIG_ITEMS    	32
#define MAX_ARCHS    		4
#define OPAQUE_MAX              32
#define MIN_HOST_NUMBER         1      /* XXX should be read from setup.config */


#define SETUP_GET_DATA(object, type)	((type *)((object)->data))

/* distinguished values for configuration
 * cards.  The CARD_TO_CLIENT_* values are the
 * amount to subtract to map to a corresponding
 * client-usable value.
 */
#define	CARD_UNSPECIFIED_CPU	0
#define	CARD_TO_CLIENT_CPU	1

#define	CARD_UNSPECIFIED_ND	0
#define	CARD_FIRST_FIT_ND	1
#define	CARD_TO_CLIENT_ND	2


enum	Boolean	{ false, true };
typedef	enum	Boolean	Boolean;
typedef	unsigned	Map_t;

typedef void 		(*Callback)();
typedef void 		(*Voidfunc)();
typedef Boolean		(*Boolfunc)();
typedef unsigned 	Set_bits[8];


typedef enum {
	SETUP_WORKSTATION,
	SETUP_CONTROLLER,
	SETUP_DISK, 
	SETUP_HARD_PARTITION, 
	SETUP_SOFT_PARTITION,  
	SETUP_CLIENT,
	SETUP_OSWG,
	SETUP_ARCH_SERVED,
	SETUP_CARD,
	SETUP_LAST_TYPE
} Setup_type;

typedef enum {
	HARD_RESERVED_NONE,
	HARD_RESERVED_ENTIREDISK,
	HARD_RESERVED_ROOT,
	HARD_RESERVED_SWAP,
	HARD_RESERVED_ND,
	HARD_RESERVED_STANDALONE_USR,
	HARD_RESERVED_SERVERHOMEDIRS,
	HARD_RESERVED_PUB010,
	HARD_RESERVED_PUB020,
	HARD_RESERVED_USR010,
	HARD_RESERVED_USR020,
	HARD_RESERVED_LAST
} Hard_reserved_type;

struct {
	char		name[64];
	char		partition;
	char    	mount_point[64];
	Hard_type	type;
	int		minimum_size;
} hard_reserved_info[HARD_RESERVED_LAST];


typedef enum {
	SOFT_FREE,
	SOFT_ROOT,
	SOFT_SWAP,
	SOFT_OTHER,
	SOFT_LAST_TYPE
} Soft_type;


typedef enum {
	MC68010,
	MC68020,
	ARCH_TYPE_LAST
} Arch_type;


typedef enum {
	BAD_NETWORK = FALSE,
	CLASS_A_NETWORK = 24,
	CLASS_B_NETWORK = 16,
	CLASS_C_NETWORK = 8
} Network_class;

Network_class	legal_network();


typedef enum {
	TAPE_LOCAL,
	TAPE_REMOTE,
} Tape_loc;


typedef enum {
	MAIL_SERVER,
	MAIL_CLIENT,
} Mail_type;


typedef char    *Config_array[ord(CONFIG_LAST)][CONFIG_ITEMS];

typedef struct {
        Setup_attribute         attr;
        Opaque                  data;
} Opaque_info;


/*
 * all disk sizes are stored in sectors, so these become multipliers
 * for conversion
 */
typedef enum {
        MBYTES, 
        KBYTES, 
	CYLS,
        SECTORS
} Disk_display_units;


typedef struct {
	Disk_display_units	disk_display;
	int			autohost;
	int			firsthost;
	Map_t			default_oswg;
} Parameters;


typedef struct {
	Setup_type	type;
	void		(*set_attr)();
	caddr_t		(*get_attr)();
	void		(*destroy)();
	void		(*callback)();
	Opaque_info     opaque_info[OPAQUE_MAX];
	caddr_t		data;
	Set_bits	status;
} Setup_object;


typedef struct {
    char		*name;
    int			cpu_arch;
    int			root_nd_index;
    Hard_partition	root_nd;
    int			root_size;
    int			swap_nd_index;
    Hard_partition	swap_nd;
    int			swap_size;
    int			three_com_interface;
} Card_info;


typedef struct {
    char		*name;
    char		*ether_addr;
    char		*host_number;
    int			model;
    Arch_type		cpu_arch;
    int			three_com_interface;
    int			num_soft_parts;
    Soft_partition	soft_parts[CLIENT_MAX_SOFT_PARTS];
} Client_info;


typedef struct {
    Soft_type		type;
    int			size;
    int			offset;
    Hard_partition	hp;
    Client		client;
    int			ndl;
} Soft_partition_info;



typedef struct {
	Disk			disk;
	int			size;
	int			offset;
	char			*mount_point;
	Hard_type		type;
	char			*name;
	char			*letter;
	int			num_soft_parts;
	Soft_partition		soft_parts[HARD_MAX_SOFT_PARTS];
	Hard_reserved_type	res_type;
	int			free_hog;
	int			min_size;
} Hard_partition_info;
	
	
typedef struct {
	char		*name;
        Setup_setting	type;
	int		cyls;
	int		heads;
	int		sectors_track;
	int		size;
	int		free_space;
    	Hard_partition	hard_parts[DISK_NUM_HARD_PARTS];
	int		floating;
	int		overlapping;
	int		cyl_rounding;
} Disk_info;

typedef struct {
	char			*name;
        Setup_setting           type;
	int			num_disks;
	Disk			disks[SETUP_MAX_DISKS_PER_CONTROLLER];
} Controller_info;

typedef struct {
	char	*name;
	int	usrsize[ARCH_TYPE_LAST];
	char	*dirs[ord(WORKSTATION_LAST)];
} Oswg_info;

typedef struct {
	Arch_type	type;
	char		*name;
	Hard_partition	pub_hard;
	Hard_partition	usr_hard;
	Map_t 		oswg_map;
} Arch_served_info;

typedef struct  {
	Client_info		info;
	int			ethertype;
	char			*domain;
	char			*network;
	Network_class		class;
	Workstation_type	type;
	int			tape_type;
	Tape_loc		tape_loc;
	char			*tape_server;
	char			*tape_server_inet;
	Map_t 			arch_served_map;
	int			num_controllers;
	Controller		controllers[WS_MAX_CONTROLLERS];
	int			num_clients;
	Client			clients[WS_MAX_CLIENTS];
        Config_array            configs;
	Parameters		params;
	int			num_arch_served;
	Arch_served		arch_served_array[ord(ARCH_TYPE_LAST)];
	Hard_partition		nd_hard;
	Hard_partition		homedir_hard;
	Hard_partition		root_hard;
	int			num_oswgs;
	Oswg			oswgs[WS_MAX_OSWGS];
	Voidfunc		message;
	Boolfunc		confirm;
	Voidfunc		contfunc;
	int			upgrade;			
	Yp_type			yp_type;
	char			*yp_master_name;
	char			*yp_master_internet;
	Card			default_card;
	int			num_cards;
	Card			cards[WS_MAX_CARDS];
	Mail_type		mail_type;
	int			preserved;
	int			console_fd;
} Workstation_info;


char		*get_device_abbrev();
char		*legal_host_number();
char		*legal_tapehost_number();
char		*sectors_to_units();
char		*sectors_to_units_left();
int		units_to_sectors();
int		round_to_cyls();
char		*malloc();
char		*calloc();
char		*strcpy();
char		*rindex();
Hard_partition	hard_partition_initialization();

char    	setup_msgbuf[256];
char		scratch_buf[256];

Workstation     workstation;

#define privledged(obj) \
	(TEST_ATTR_BIT(((Setup_object *)obj)->status, SETUP_PRIVLEDGED))
#define privledged_on(obj) \
	(SET_ATTR_BIT(((Setup_object *)obj)->status, SETUP_PRIVLEDGED))
#define privledged_off(obj) \
	(CLR_ATTR_BIT(((Setup_object *)obj)->status, SETUP_PRIVLEDGED))


int	Mflag;			/* running from the miniroot */


#define isoverlapped(x1, y1, x2, y2) \
	((x1 <= x2 && y1 >= x2) || (x2 <= x1 && y2 >= x1))

#define is_floating(disk) ((int)setup_get(disk, DISK_PARAM_FLOATING))

#define is_reserved(hp) \
	((Hard_reserved_type)setup_get(hp, HARD_RESERVED_TYPE) \
	!= HARD_RESERVED_NONE)

#define is_entiredisk(hp) \
	((Hard_reserved_type)setup_get(hp, HARD_RESERVED_TYPE) \
	 == HARD_RESERVED_ENTIREDISK)

#define is_nd_partition(hp) ((Hard_type)setup_get(hp, HARD_TYPE) == HARD_ND)


#define is_in_map(map, i) ((map & (1 << i)) == (1 << i))
#define not_in_map(map, i) ((map & (1 << i)) == 0)

