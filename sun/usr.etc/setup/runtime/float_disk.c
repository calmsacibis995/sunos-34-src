
#ifndef lint
static	char sccsid[] = "@(#)float_disk.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "setup_runtime.h"


init_floating_disk(disk)
Disk	disk;
{
	Hard_partition		hp;
	int			i;
	int			used_space = 0;
	int			free_space;

	SETUP_FOREACH_OBJECT(disk, DISK_HARD_PARTITION, i, hp) {
		if ((! is_entiredisk(hp)) &&
		    (! (int)setup_get(hp, HARD_PARAM_FREEHOG))) {
			used_space += (int)setup_get(hp, HARD_SIZE);
		}
	} SETUP_END_FOREACH

	free_space = (int)setup_get(disk, DISK_SIZE) - used_space;

	if (free_space < 0) {
		runtime_message(SETUP_ENOFLOAT, 
		    (char *)setup_get(disk, DISK_NAME));
		return(TRUE);
	} else {
		setup_set(disk, DISK_FREE_SPACE, free_space, 0);
		return(FALSE);
	}

}


typedef struct {
	int	index;
	int	offset;
	int	size;
	char	partition;
} Hp_float_array;
Hp_float_array	hp_float_array[SETUP_MAX_HARD_PARTITIONS];
int		hp_offset_compare();

float_disk(disk)
Disk	disk;
{
	int		i;
	int		count;
	Hard_partition	hp;
	int		total_offset;
	char		*ptr;

	count = 0;
	SETUP_FOREACH_OBJECT(disk, DISK_HARD_PARTITION, i, hp) {
                if (is_entiredisk(hp)) {
			continue;
		}
		hp_float_array[count].index = i;
		hp_float_array[count].offset = (int)setup_get(hp, HARD_OFFSET);
		hp_float_array[count].size = (int)setup_get(hp, HARD_SIZE);
		ptr = (char *)setup_get(hp, HARD_LETTER);
		hp_float_array[count].partition = ptr[0];
		count++;
	} SETUP_END_FOREACH

	qsort(hp_float_array, count, sizeof(Hp_float_array), hp_offset_compare);

	total_offset = 0;
	for (i = 0; i < count; i++) {
		hp = setup_get(disk, 
		    DISK_HARD_PARTITION, hp_float_array[i].index);
		privledged_setup_set(hp, 
			HARD_OFFSET, total_offset, 
			0);
		if ((int)setup_get(hp, HARD_PARAM_FREEHOG)) {
			callback(hp, HARD_SIZE_STRING, FALSE);
		}
		total_offset += round_to_cyls(disk, 
		    (int)setup_get(hp, HARD_SIZE));
	}
		
}


hp_offset_compare(elem1, elem2)
Hp_float_array	*elem1;
Hp_float_array	*elem2;
{
	if (elem2->offset == 0) {
		if (elem1->partition < elem2->partition) {
			return(-1);
		} else if (elem1->partition > elem2->partition) {
			return(1);
		} else {
			return(0);
		}
	} else {
		if (elem1->offset < elem2->offset) {
			return(-1);
		} else if (elem1->offset > elem2->offset) {
			return(1);
		} else {
			return(0);
		}
	}
}


float_soft_partitions(sp)
Soft_partition	sp;
{
        Hard_partition  hp;
        int             total_offset;
	Soft_partition	sp1;
        int             i;

	hp = (Hard_partition)setup_get(sp, SOFT_HARD_PARTITION);
 
        total_offset = 0;
        SETUP_FOREACH_OBJECT(hp, HARD_SOFT_PARTITION, i, sp1) {
		setup_set(sp1, 
			SOFT_OFFSET, total_offset, 
			0);
                total_offset += (int) setup_get(sp1, SOFT_SIZE);
        } SETUP_END_FOREACH
}
