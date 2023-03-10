
/*	@(#)setup_attr.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <sunwindow/attr.h>

#ifndef Opaque_DEFINED
typedef char	*Opaque;
#define       Opaque_DEFINED
#endif
typedef int	(*Callout_function)();

typedef Opaque	Workstation;
typedef Opaque	Controller;
typedef Opaque	Disk;
typedef Opaque	Hard_partition;
typedef Opaque	Soft_partition;
typedef Opaque	Client;
typedef Opaque	Card;
typedef Opaque	Arch_served;
typedef Opaque	Oswg;

 
#define SETUP_ATTR(type, ordinal)     ATTR(ATTR_PKG_UNUSED_FIRST, type, ordinal)
#define ord(x)          ((int) x)
 

typedef enum {

	SETUP_CALLBACK		= SETUP_ATTR(ATTR_FUNCTION_PTR, 1),	
	SETUP_MESSAGE_PROC	= SETUP_ATTR(ATTR_FUNCTION_PTR, 2),
	SETUP_CONFIRM_PROC	= SETUP_ATTR(ATTR_FUNCTION_PTR, 3),	
	SETUP_CONTINUE_PROC	= SETUP_ATTR(ATTR_FUNCTION_PTR, 4),
	SETUP_OPAQUE		= SETUP_ATTR(ATTR_INT_PAIR, 5),
	SETUP_STATUS		= SETUP_ATTR(ATTR_BOOLEAN, 6),
	SETUP_CHOICE_STRING	= SETUP_ATTR(ATTR_INT_TRIPLE, 7),
	SETUP_ALL		= SETUP_ATTR(ATTR_NO_VALUE, 8),
	SETUP_NONE		= SETUP_ATTR(ATTR_NO_VALUE, 9),
	SETUP_PRIVLEDGED	= SETUP_ATTR(ATTR_NO_VALUE, 10),
	SETUP_NOTCHANGEABLE	= SETUP_ATTR(ATTR_INT, 11),
	SETUP_UPGRADE		= SETUP_ATTR(ATTR_INT, 139),

	WS_NAME			= SETUP_ATTR(ATTR_STRING, 12), 	
	WS_ETHERTYPE		= SETUP_ATTR(ATTR_INT, 13),
	WS_HOST_NUMBER 		= SETUP_ATTR(ATTR_STRING, 14),	
	WS_DOMAIN		= SETUP_ATTR(ATTR_STRING, 15),	
	WS_MODEL		= SETUP_ATTR(ATTR_INT, 16),	
	WS_ARCH			= SETUP_ATTR(ATTR_INT, 17),	
	WS_ARCH_SERVED		= SETUP_ATTR(ATTR_INT, 18),	
	WS_TYPE			= SETUP_ATTR(ATTR_INT, 19),	
	WS_TAPE_LOC		= SETUP_ATTR(ATTR_INT, 20),	
	WS_TAPE_TYPE		= SETUP_ATTR(ATTR_INT, 21),	
	WS_TAPE_SERVER		= SETUP_ATTR(ATTR_STRING, 22),	
	WS_HOST_INTERNET_NUMBER	= SETUP_ATTR(ATTR_STRING, 23),	
	WS_NETWORK		= SETUP_ATTR(ATTR_STRING, 25),
	WS_NETWORK_CLASS	= SETUP_ATTR(ATTR_STRING, 26),
	WS_ND_PARTITION		= SETUP_ATTR(ATTR_INT_PAIR, 27),
	WS_NUM_CONTROLLERS	= SETUP_ATTR(ATTR_INT, 28),
	WS_CONTROLLER		= SETUP_ATTR(ATTR_INT_PAIR, 29),
	WS_DISK_INDEX		= SETUP_ATTR(ATTR_INT, 135),
	WS_CLIENT		= SETUP_ATTR(ATTR_INT_PAIR, 30),
	WS_CLIENT_NAME		= SETUP_ATTR(ATTR_STRING, 31),
	WS_CARD			= SETUP_ATTR(ATTR_INT_PAIR, 32),
	WS_CARD_NAME		= SETUP_ATTR(ATTR_STRING, 33),
	WS_CARD_NAME_TO_DEFAULT	= SETUP_ATTR(ATTR_STRING, 199),
	WS_DEFAULT_CARD		= SETUP_ATTR(ATTR_OPAQUE, 34),
	WS_OSWG			= SETUP_ATTR(ATTR_INT_PAIR, 35),
	WS_NUM_ARCH_SERVED	= SETUP_ATTR(ATTR_INT, 36),
	WS_ARCH_SERVED_ARRAY	= SETUP_ATTR(ATTR_INT_PAIR, 37),
	WS_SERVED_NDHARD	= SETUP_ATTR(ATTR_INT, 38),
	WS_SERVED_HOMEDIRHARD	= SETUP_ATTR(ATTR_INT, 39),
	WS_SERVED_ROOTHARD	= SETUP_ATTR(ATTR_INT, 40),
	WS_YPTYPE		= SETUP_ATTR(ATTR_INT, 41),
	WS_YPMASTER_NAME	= SETUP_ATTR(ATTR_STRING, 42),
	WS_YPMASTER_INTERNET	= SETUP_ATTR(ATTR_STRING, 43),
	WS_PRESERVED		= SETUP_ATTR(ATTR_INT, 44),
	WS_MAILTYPE		= SETUP_ATTR(ATTR_INT, 45),
	WS_MOVE_HARD_PART	= SETUP_ATTR(ATTR_INT_PAIR, 155),
	WS_CONSOLE_FD		= SETUP_ATTR(ATTR_INT, 178),
	WS_GET_CONSOLE_MESSAGES	= SETUP_ATTR(ATTR_NO_VALUE, 179),
	
	CONTROLLER_NAME		= SETUP_ATTR(ATTR_STRING, 46),
	CONTROLLER_TYPE		= SETUP_ATTR(ATTR_INT, 47),
	CONTROLLER_NUM_DISKS	= SETUP_ATTR(ATTR_INT, 48),
	CONTROLLER_DISK		= SETUP_ATTR(ATTR_INT_PAIR, 49),
	
	DISK_NAME		= SETUP_ATTR(ATTR_STRING, 50),
	DISK_TYPE		= SETUP_ATTR(ATTR_INT, 51),
	DISK_NCYLS		= SETUP_ATTR(ATTR_INT, 52),
	DISK_NHEADS		= SETUP_ATTR(ATTR_INT, 53),
	DISK_SEC_TRACK		= SETUP_ATTR(ATTR_INT, 54),
	DISK_SIZE		= SETUP_ATTR(ATTR_INT, 55),
	DISK_SIZE_STRING_LEFT	= SETUP_ATTR(ATTR_STRING, 56),
	DISK_FREE_SPACE		= SETUP_ATTR(ATTR_INT, 57), 	
	DISK_FREE_SPACE_STRING_LEFT	= SETUP_ATTR(ATTR_STRING, 58),
	DISK_HARD_PARTITION	= SETUP_ATTR(ATTR_INT_PAIR, 59),
	DISK_FREE_HOG_INDEX	= SETUP_ATTR(ATTR_INT, 137),
	DISK_PARAM_FLOATING	= SETUP_ATTR(ATTR_INT, 60),
	DISK_PARAM_OVERLAPPING	= SETUP_ATTR(ATTR_INT, 61),
	DISK_PARAM_CYLROUNDING	= SETUP_ATTR(ATTR_INT, 62),
   
	SOFT_TYPE		= SETUP_ATTR(ATTR_INT, 63), 	
	SOFT_SIZE		= SETUP_ATTR(ATTR_INT, 64), 	
	SOFT_SIZE_STRING	= SETUP_ATTR(ATTR_STRING, 65), 	
	SOFT_SIZE_STRING_LEFT	= SETUP_ATTR(ATTR_STRING, 66), 	
	SOFT_OFFSET		= SETUP_ATTR(ATTR_INT, 67), 	
	SOFT_OFFSET_STRING	= SETUP_ATTR(ATTR_STRING, 68), 	
	SOFT_HARD_PARTITION	= SETUP_ATTR(ATTR_OPAQUE, 69), 
	SOFT_CLIENT		= SETUP_ATTR(ATTR_INT, 70), 
	SOFT_CLIENT_NAME	= SETUP_ATTR(ATTR_STRING, 71), 
	SOFT_NDL		= SETUP_ATTR(ATTR_INT, 72), 
	
	CLIENT_NAME		= SETUP_ATTR(ATTR_STRING, 73),
	CLIENT_E_ADDR		= SETUP_ATTR(ATTR_STRING, 74),
	CLIENT_HOST_NUMBER	= SETUP_ATTR(ATTR_STRING, 75),
	CLIENT_MODEL		= SETUP_ATTR(ATTR_INT, 76),
	CLIENT_ARCH		= SETUP_ATTR(ATTR_INT, 77),
	CLIENT_3COM_INTERFACE	= SETUP_ATTR(ATTR_INT, 190),
	CLIENT_DISPLAY_LOC_X	= SETUP_ATTR(ATTR_X, 78),
	CLIENT_DISPLAY_LOC_Y	= SETUP_ATTR(ATTR_Y, 79),
	CLIENT_ROOT_SIZE	= SETUP_ATTR(ATTR_INT, 80),
	CLIENT_ROOT_SIZE_STRING	= SETUP_ATTR(ATTR_STRING, 81),
	CLIENT_ROOT_SIZE_STRING_LEFT	= SETUP_ATTR(ATTR_STRING, 82),
	CLIENT_ROOT_PARTITION	= SETUP_ATTR(ATTR_OPAQUE, 83),
	CLIENT_ROOT_PARTITION_INDEX	= SETUP_ATTR(ATTR_INT, 84),
	CLIENT_SWAP_SIZE	= SETUP_ATTR(ATTR_INT, 85),
	CLIENT_SWAP_SIZE_STRING	= SETUP_ATTR(ATTR_STRING, 86),
	CLIENT_SWAP_SIZE_STRING_LEFT	= SETUP_ATTR(ATTR_STRING, 87),
	CLIENT_SWAP_PARTITION	= SETUP_ATTR(ATTR_OPAQUE, 88),
	CLIENT_SWAP_PARTITION_INDEX	= SETUP_ATTR(ATTR_INT, 89),

	CARD_APPLY_TO		= SETUP_ATTR(ATTR_STRING, 90),

	HARD_NAME		= SETUP_ATTR(ATTR_STRING, 91), 	
	HARD_LETTER             = SETUP_ATTR(ATTR_STRING, 92),
	HARD_TYPE		= SETUP_ATTR(ATTR_INT, 93), 	
	HARD_MOUNT_PT           = SETUP_ATTR(ATTR_STRING, 94),
	HARD_SIZE		= SETUP_ATTR(ATTR_INT, 95), 	
	HARD_SIZE_STRING	= SETUP_ATTR(ATTR_STRING, 96), 	
	HARD_SIZE_STRING_LEFT	= SETUP_ATTR(ATTR_STRING, 97), 	
	HARD_SIZE_REAL		= SETUP_ATTR(ATTR_INT, 98), 	
	HARD_OFFSET		= SETUP_ATTR(ATTR_INT, 99), 	
	HARD_OFFSET_STRING	= SETUP_ATTR(ATTR_STRING, 100), 	
	HARD_OFFSET_STRING_LEFT	= SETUP_ATTR(ATTR_STRING, 101), 	
	HARD_OFFSET_CYL		= SETUP_ATTR(ATTR_INT, 102), 	
	HARD_OFFSET_CYL_REAL	= SETUP_ATTR(ATTR_INT, 103),
	HARD_DISK		= SETUP_ATTR(ATTR_OPAQUE, 104), 
	HARD_DISK_NAME		= SETUP_ATTR(ATTR_STRING, 105), 
	HARD_NUM_SOFT_PARTITION	= SETUP_ATTR(ATTR_INT, 166),
	HARD_SOFT_PARTITION	= SETUP_ATTR(ATTR_INT_PAIR, 106),
	HARD_INDEX		= SETUP_ATTR(ATTR_INT, 136),
	HARD_ND_INDEX		= SETUP_ATTR(ATTR_INT, 107),
	HARD_RESERVED_TYPE	= SETUP_ATTR(ATTR_INT, 108),
	HARD_PARAM_FREEHOG	= SETUP_ATTR(ATTR_INT, 109),
	HARD_MOVEABLE		= SETUP_ATTR(ATTR_INT, 177),
	HARD_WHAT_IT_IS		= SETUP_ATTR(ATTR_STRING, 112), 
	HARD_TEST_FIT		= SETUP_ATTR(ATTR_INT, 113),
	HARD_MIN_SIZE		= SETUP_ATTR(ATTR_INT, 114),
	HARD_MIN_SIZE_IN_UNITS	= SETUP_ATTR(ATTR_INT, 144),
	HARD_MAX_SIZE_IN_UNITS	= SETUP_ATTR(ATTR_INT, 115),
	
	PARAM_DISK_DISPLAY_UNITS= SETUP_ATTR(ATTR_INT, 116),
	PARAM_AUTOHOST		= SETUP_ATTR(ATTR_INT, 117),
	PARAM_FIRSTHOST		= SETUP_ATTR(ATTR_INT, 118), 
	PARAM_FIRSTHOST_STRING_LEFT	= SETUP_ATTR(ATTR_INT, 134), 
	PARAM_DEFAULT_CARD	= SETUP_ATTR(ATTR_INT_PAIR, 119),
	PARAM_DEFAULT_CARD_NAME	= SETUP_ATTR(ATTR_STRING, 120),
	PARAM_DEFAULT_OSWG	= SETUP_ATTR(ATTR_INT, 121),
	
	OSWG_NAME		= SETUP_ATTR(ATTR_STRING, 122),
	OSWG_USRSIZE		= SETUP_ATTR(ATTR_INT_PAIR, 123),
	OSWG_DESCRIPTION	= SETUP_ATTR(ATTR_INT_PAIR, 124),
	OSWG_DIRECTORY		= SETUP_ATTR(ATTR_INT_PAIR, 138),

	ARCH_SERVED_NAME	= SETUP_ATTR(ATTR_STRING, 125),
	ARCH_SERVED_TYPE	= SETUP_ATTR(ATTR_INT, 126),
	ARCH_SERVED_PUBHARD	= SETUP_ATTR(ATTR_INT, 127),
	ARCH_SERVED_USRHARD	= SETUP_ATTR(ATTR_INT, 128),
	ARCH_OSWG		= SETUP_ATTR(ATTR_INT, 129),
	ARCH_OSWG_SELECTED	= SETUP_ATTR(ATTR_INT, 130),
	ARCH_OSWG_CLEAR		= SETUP_ATTR(ATTR_NO_VALUE, 131),
	ARCH_OSWG_ALL		= SETUP_ATTR(ATTR_NO_VALUE, 132),
	ARCH_OSWG_DEFAULT	= SETUP_ATTR(ATTR_NO_VALUE, 133),
	
	SETUP_LAST_ATTR
} Setup_attribute;

typedef caddr_t	*Avlist;

typedef enum  {
	CONFIG_PARAMETERS,
	CONFIG_MODEL,
	CONFIG_TYPE,
	CONFIG_CPU,
	CONFIG_TAPESTATE,
	CONFIG_DISKTYPE,
	CONFIG_TAPETYPE,
	CONFIG_ETHERTYPE,
	CONFIG_DISKCONTROLLER,
	CONFIG_DISK,
	CONFIG_HARD_PARTITION_TYPE,
	CONFIG_HARD_PARTITION_RESERVED,
	CONFIG_SOFT_PARTITION_TYPE,
	CONFIG_DISK_DISPLAY_UNITS,
	CONFIG_OSWG, 
	CONFIG_YP_TYPE, 
	CONFIG_CARD_CPU,
	CONFIG_CARD_ND,
	CONFIG_TAPE_LOCATION,
	CONFIG_MAIL_TYPES,
	CONFIG_LAST
} Config_type;


typedef enum {
	SETUP_SCSI,
	SETUP_XYLOGICS,
	SETUP_LAST_SETTING
} Setup_setting;


typedef enum {
	WORKSTATION_NONE,
	WORKSTATION_STANDALONE,
	WORKSTATION_SERVER,
	WORKSTATION_LAST,
} Workstation_type;

typedef enum {
	YP_NONE,
	YP_CLIENT,
	YP_SLAVE_SERVER,
	YP_MASTER_SERVER,
} Yp_type;


typedef enum {
	HARD_FREE,
	HARD_SWAP,
	HARD_UNIX,
	HARD_ND,
	HARD_OTHER,
	HARD_LAST_TYPE
} Hard_type;



/* constants for show/no show decisions */
#define	SETUP_TAPELESS		0
#define	SETUP_NOETHERNET	0

/* global constants */
/* number of hard partitions from 'a' to 'h' */
#define SETUP_MAX_HARD_PARTITIONS	8
#define	SETUP_MAX_CONTROLLERS		4
#define	SETUP_MAX_DISKS_PER_CONTROLLER	2
#define	SETUP_MAX_DISKS			\
    (SETUP_MAX_CONTROLLERS * SETUP_MAX_DISKS_PER_CONTROLLER)
    
    
#define	SETUP_FOREACH_CHOICE(ws, attr, index, string)	\
    { \
	for ((index) = 0; \
	     (string) = (char *) \
		 setup_get((ws), SETUP_CHOICE_STRING, (attr), (index)); \
	     (index)++) {

#define	SETUP_FOREACH_OBJECT(parent, attr, index, child)	\
    {   \
	for ((index) = 0; \
	     (child) = setup_get((parent), attr, (index)); \
	     (index)++) {

#define	SETUP_END_FOREACH		}}

#define SETUP_APPEND	(-1)

/* public object names for setup_create() */
#define	WORKSTATION	workstation_create
#define	CONTROLLER	controller_create
#define	DISK		disk_create
#define	HARD_PARTITION	hard_partition_create
#define	SOFT_PARTITION	soft_partition_create
#define	CLIENT		client_create
#define	CARD		card_create
#define	ARCH_SERVED	arch_served_create
#define	OSWG		oswg_create

/* corresponding function names */
extern Workstation	workstation_create();
extern Controller	controller_create();
extern Disk		disk_create();
extern Hard_partition	hard_partition_create();
extern Soft_partition	soft_partition_create();
extern Client		client_create();
extern Card		card_create();
extern Arch_served	arch_served_create();
extern Oswg		oswg_create();


/* public routines */
extern Opaque		setup_create();
extern void		setup_set();
extern Opaque		setup_get();
extern void		card_apply();
extern Workstation	mid_init();
