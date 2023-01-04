#ifndef lint
static char sccsid[] = "@(#)hash.c 1.3 87/01/07 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * This file contains a bunch of routines that implement a general purpose
 * hash table.
 */

#include <sunwindow/sun.h>	/* Get things like TRUE and FALSE defined */

#include "hash.h"		/* Include hash table exported routines */

/*
 * Internally used routines:
 */
void free_data();
void free_memory();
long *get_data();
char *get_memory();
char hash_int_equal();
int hash_int_hash();
int hash_int_insert();
void hash_initialize();
void hash_resize();
char *malloc();

/*
 * hash_create(Size, Key_Empty, Key_Equal, Key_Hash, Key_Insert, Value_Empty,
 * Value_Insert, 7) will create and return a hash table using the parameters.
 * Due to the large  number of arguments, the last argument must be the number
 * 7 so that a quick check can be made to make sure that they are all there.
 * All of the arguments except the last one can be NULL'ed out.
 */

Hash
hash_create(size, key_empty, key_equal, key_hash, key_insert,
				value_empty, value_insert, check)
	register int	size;		/* Number of initial slots */
	int		 key_empty;	/* Empty key value */
	char		(*key_equal)();	/* Key equality routine */
	int		(*key_hash)();	/* Key hash function */
	int		(*key_insert)();/* Key insertion routine */
	int		value_empty;	/* Empty value */
	int		(*value_insert)();/* Value insertion routine */
	int		check;		/* Argument count check */
{
	register Hash hash;	/* New hash table */

	if (check != 7){
		(void)fprintf(stderr, "HASH_CREATE: Wrong number of arguments\n");
		exit(1);
	}
	hash = (Hash)get_memory(sizeof *hash);
	hash->count = 0;	
	hash->key_empty = key_empty;
	hash->key_equal = (key_equal != NULL) ? key_equal : hash_int_equal;
	hash->key_hash = (key_hash != NULL) ? key_hash : hash_int_hash;
	hash->key_insert =
		(key_insert != NULL) ? key_insert : hash_int_insert;
	hash->value_empty = value_empty;
	hash->value_insert =
		(value_insert != NULL) ? value_insert : hash_int_insert;
	hash_initialize(hash, size);
	return hash;
}

/*
 * hash_destroy(Hash, Key_Destroy, Value_Destroy) will deallocate the storage
 * associated with Hash.  Each key-value pair in Hash will be destroyed by
 * calling Key_Destroy(Key) and Value_Destroy(Value).  If Key_Destroy is NULL,
 * it will not be called.  Likewise, if Value_Destroy is NULL, it will not
 * be called.
 */
void
hash_destroy(hash, key_destroy, value_destroy)
	Hash		hash;		/* Hash table to destroy */
	void		(*key_destroy)(); /* Key destroy routine */
	void		(*value_destroy)(); /* Value destroy routine */
{
	register int	count;		/* Number of remaining slots */
	register long	key_empty;	/* Empty key */
	register long	key;		/* Current key */
	register long	*keys;		/* Hash keys */
	register long	slots;		/* Number of slots */
	register long	*values;	/* Hash values */

	key_empty = hash->key_empty;
	keys = hash->keys;
	slots = hash->slots;
	values = hash->values;
	if ((key_destroy != NULL) || (value_destroy != NULL)){
		for (count = slots; count > 0; count--){
			key = *keys++;
			if (key != key_empty){
				if (key_destroy != NULL)
					key_destroy(key);
				if (value_destroy != NULL)
					value_destroy(*values);
			}
			values++;
		}
	}
	free_data(hash->keys, (int)slots);
	free_data(hash->values, (int)slots);
	free_memory((char *)hash, sizeof *hash);
}

/*
 * hash_find(Hash, Key, &Value)=>{True,False} will lookup Key in Hash.  If Key
 * is found in Hash, True will be returned and the associated value will be
 * stored into Value, provided Value is non-Null.  Otherwise, False will be
 * returned.
 */
Bool
hash_find(hash, key, value)
	register Hash	hash;	/* Hash table */
	long		key;	/* Key to lookup */
	long		*value;	/* Place to store value */
{
	int		index;	/* Index into table */

	index = hash_index(hash, key);
	if (hash->keys[index] == hash->key_empty)
		return False;
	if (value != NULL)
		*value = hash->values[index];
	return True;
}

/*
 * hash_get(Hash, Key)=>Value will lookup the value for Key in Hash.  If Key
 * is not in Hash, a fatal error occurs.
 */
long
hash_get(hash, key)
	register Hash	hash;	/* Hash table */
	long		key;	/* Key to lookup */
{
	int		index;	/* Index into table */

	index = hash_index(hash, key);
	if (hash->keys[index] == hash->key_empty){
		(void)fprintf(stderr,
			"HASH_GET:Could not find key (%x) in table (%x)\n",
			key, hash);
		exit(1);
		/*NOTREACHED*/
	} else	return hash->values[index];
}

/*
 * hash_insert(Hash, Key, Value)=>{True,False} will insert Value into Hash
 * under Key.  If Key is already in Hash, False will be returned and the
 * previous value will not be changed.  Otherwise, True will be returned.
 */
/*VARARGS1*/
Bool
hash_insert(hash, key, value)
	Hash	hash;	/* Hash table */
	long		key;	/* Key to insert under */
	long		value;	/* Value to insert */
{
	register int	index;	/* Index into hash table */

	if (hash->count == hash->limit)
		hash_resize(hash);
	index = hash_index(hash, key);
	if (hash->keys[index] == hash->key_empty){
		hash->keys[index] = hash->key_insert(key);
		hash->values[index] = hash->value_insert(value);
		hash->count++;
		return True;
	} else	return False;
}

/*
 * hash_lookup(Hash, Key)=>value will lookup Key in Hash.  If Key is not
 * in Hash, Empty_Value will be returned.
 */
/*VARARGS1*/
long
hash_lookup(hash, key)
	Hash	hash;	/* Hash table */
	long		key;	/* Key to lookup */
{
	return hash->values[hash_index(hash, key)];
}

/*
 * hash_replace(Hash, Key, Value)=>{True,False} will insert Value into Hash
 * under Key.  If Key was already in Hash, True will be returned and the
 * previous value will be replaced.  Otherwise, False will be returned and
 * the Value will be inserted under Key.
 */
Bool
hash_replace(hash, key, value)
	register Hash	hash;	/* Hash table */
	long		key;	/* Key to insert under */
	long		value;	/* Value to insert */
{
	register int	index;	/* Index into hash table */

	if (hash->count == hash->limit)
		hash_resize(hash);
	index = hash_index(hash, key);
	if (hash->keys[index] == hash->key_empty){
		hash->keys[index] = hash->key_insert(key);
		hash->values[index] = hash->value_insert(value);
		hash->count++;
		return False;
	}
	hash->values[index] = hash->value_insert(value);
	return True;
}

/*
 * hash_scan(Hash, Routine, Data) will scan the entire contents of the hash
 * table calling Routine(Key, Value, Data) for each key-value pair in Hash.
 * The sum of the values returned by routine will be returned.
 */
int
hash_scan(hash, routine, data)
	Hash		hash;		/* Hash table */
	register int	(*routine)();	/* Routine to scan with */
	register long	data;		/* Data value */
{
	register int	count;		/* Remaining slots to scan */
	register int	sum;		/* Sum of returned values */
	register long	key_empty;	/* Empty key value */
	register long	key;		/* Current key */
	register long	*keys;		/* Hash table keys */
	register long	*values;	/* Hash table values */

	sum = 0;
	key_empty = hash->key_empty;
	keys = hash->keys;
	values = hash->values;
	for (count = hash->slots; count > 0; count--){
		key = *keys++;
		if (key != key_empty)
			sum += routine(key, *values, data);
		values++;
	}
	return sum;
}

/*
 * hash_show(Hash) will show the contents of Hash on the console.  This
 * routine is used for testing and debugging purposes only.
 */
void
hash_show(hash)
	register Hash	hash;	/* Hash table */
{
	int		index;	/* Index into table */
	register long	*keys;	/* Keys array */
	int		slots;	/* Slots in table */
	register long	*values;/* Values array */

	(void)printf("Hash:%x Count:%d Limit:%d Slots:%d\n",
			hash, hash->count, hash->limit, hash->slots);
	(void)printf("Key_Empty:%d Key_Equal:%x Key_Hash:%x Key_Insert:%x\n",
		hash->key_empty, hash->key_equal,
		hash->key_hash, hash->key_insert);
	(void)printf("Value_Empty:%d Value_Insert:%d Keys:%x Values:%x\n",
		hash->value_empty, hash->value_insert,
		hash->keys, hash->values);
	keys = hash->keys;
	slots = hash->slots;
	values = hash->values;
	for (index = 0; index < slots; index++)
		(void)printf("[%d] %d:%d\n", index, *keys++, *values++);
}

/*
 * hash_size(Hash) returns the number of entries in Hash.
 */
int
hash_size(hash)
	Hash hash;		/* Hash table to use */
{
	return hash->count;
}

/*****************************************************************/
/* Internal routines:                                            */
/*****************************************************************/

/*
 * hash_index(Hash, Key) will compute the index into Hash where Key should
 * go (if it is not already there.)
 */

static int
hash_index(hash, key)
	register Hash	hash;		/* Hash table */
	long		key;		/* Key to use */
{
	register unsigned int	index;	/* Index into table */
	long		key_empty;	/* Empty key */
	char		(*key_equal)();	/* Key equality function */
	long		key_temp;	/* Temporary key */
	register long	*keys;		/* Hash table keys */
	int		slots;		/* Number of slots in table */

	key_empty = hash->key_empty;
	key_equal = hash->key_equal;
	keys = hash->keys;
	slots = hash->slots;
	index = hash->key_hash(key, slots);
	if (index >= slots)
		index %= slots;
	while (True){
		key_temp = keys[index];
		if (key_temp == key_empty)
			return index;
		if (key_equal(key, key_temp))
			return index;
		if (++index >= slots)
			index = 0;
	}
}

/*
 * hash_initialize(Hash, Size) will initialize Hash to contain Size table
 * entries.
 */
static void
hash_initialize(hash, size)
	register Hash	hash;		/* Hash table */
	register int	size;		/* Number of slots */
{
	hash->keys = get_data(size, (int)hash->key_empty);
	hash->values = get_data(size, (int)hash->value_empty);
	hash->slots = size;
	hash->limit = size * 8 / 10;
}

/*
 * hash_int_equal(Int1, Int2) will return True if Int1 equals Int2.
 */
static char
hash_int_equal(int1, int2)
	int	int1;		/* First integer */
	int	int2;		/* Second integer */
{
	return (int1 == int2);
}

/*
 * hash_int_hash(number) will return a hash on number.
 */
static int
hash_int_hash(number)
	int	number;		/* Number to hash */
{
	return number;
}

/*
 * hash_int_insert(number) will return a copy of number.
 */

static int
hash_int_insert(number)
	int	number;		/* Number to insert */
{
	return number;
}

/*
 * hash_resize(Hash) will increase the number of slots in the hash table.
 */
static void
hash_resize(hash)
	register Hash	hash;		/* Hash table to resize. */
{
	int		count;		/* Number of entries to reinsert */
	int		index;		/* Index into hash table */
	long		key;		/* Current key */
	long		key_empty;	/* Empty key value */
	register long	*key_pointer;	/* Pointer into old keys */
	long		*new_keys;	/* New keys */
	long		*new_values;	/* New values */
	long		*old_keys;	/* Old keys */
	long		*old_values;	/* Old values */
	int		slots;		/* Number of slots in old table */
	long		value;		/* Current value */
	register long	*value_pointer;	/* Pointer into old values */

	key_empty = hash->key_empty;
	old_keys = hash->keys;
	old_values = hash->values;
	slots = hash->slots;
	hash_initialize(hash, slots<<1);
	new_keys = hash->keys;
	new_values = hash->values;
	key_pointer = old_keys;
	value_pointer = old_values;
	for (count = slots; count > 0; count--){
		key = *key_pointer++;
		value = *value_pointer++;
		if (key != key_empty){
			index = hash_index(hash, key);
			new_keys[index] = key;
			new_values[index] = value;
		}
	}
	free_data(old_keys, slots);
	free_data(old_values, slots);
}

/*
 * get_data(Size, Value) will get Size words of data and initialize them to
 * Value.
 */
static long *
get_data(size, value)
	register int	size;		/* Number of words to allocate */
	register int	value;		/* Value to initialize to */
{
	long		*data;		/* Data array */
	register long	*pointer;	/* Pointer into data */

	data = (long *)get_memory(size<<2);  /* Bleech! Machine dependent */
	pointer = data;
	while (size-- > 0)
		*pointer++ = value;
	return data;
}

/*
 * get_memory(Size) will allocate Size bytes of memory.  If the memory is not
 * available, a fatal error will occur.
 */
static char *
get_memory(size)
	int		size;	/* Number of bytes to allocate */
{
	register char	*data;	/* Newly allocated data */

	data = malloc((unsigned)size);
	if (data != NULL)
		return data;
	else {
		(void)fprintf(stderr,
			"Could not allocate %d bytes (Out of swap space?)\n",
			size);
		exit(1);
		/*NOTREACHED*/
	}
}

/*
 * free_data(Data, Size) will free Size words of Data.
 */
static void
free_data(data, size)
	long		*data;		/* Data to free */
	int		size;		/* Number of words to free */
{
	free_memory((char *)data, size<<2);	/* Bleech! Machine dependent */
}

/*
 * free_memory(Data, Size) will free the Size bytes of Data.
 */
/*ARGSUSED*/
static void
free_memory(data, size)
	char		*data;		/* Data to free */
	int		size;		/* Number of bytes to free */
{
	free(data);
}

