/* hash.c - hash tables for opkg

   Steven M. Ayer, Jamey Hicks

   Copyright (C) 2002 Compaq Computer Corporation

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash_table.h"
#include "opkg_message.h"
#include "libbb/libbb.h"


static unsigned long
djb2_hash(const unsigned char *str)
{
	unsigned long hash = 5381;
	int c;
	while ((c = *str++))
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
	return hash;
}

static int
hash_index(hash_table_t *hash, const char *key)
{
	return djb2_hash((const unsigned char *)key) % hash->n_buckets;
}

/*
 * this is an open table keyed by strings
 */
void
hash_table_init(const char *name, hash_table_t *hash, int len)
{
	if (hash->entries != NULL) {
		opkg_msg(ERROR, "Internal error: non empty hash table.\n");
		return;
	}

	memset(hash, 0, sizeof(hash_table_t));

	hash->name = name;
	hash->n_buckets = len;
	hash->entries = xcalloc(hash->n_buckets, sizeof(hash_entry_t));
}

void
hash_print_stats(hash_table_t *hash)
{
	printf("hash_table: %s, %d bytes\n"
		"\tn_buckets=%d, n_elements=%d, n_collisions=%d\n"
		"\tmax_bucket_len=%d, n_used_buckets=%d, ave_bucket_len=%.2f\n"
		"\tn_hits=%d, n_misses=%d\n",
		hash->name,
		hash->n_buckets*(int)sizeof(hash_entry_t),
		hash->n_buckets,
		hash->n_elements,
		hash->n_collisions,
		hash->max_bucket_len,
		hash->n_used_buckets,
		(hash->n_used_buckets ?
			((float)hash->n_elements)/hash->n_used_buckets : 0.0f),
		hash->n_hits,
		hash->n_misses);
}

void hash_table_deinit(hash_table_t *hash)
{
    int i;
    if (!hash)
        return;

    /* free the reminaing entries */
    for (i = 0; i < hash->n_buckets; i++) {
	hash_entry_t *hash_entry = (hash->entries + i);
	free (hash_entry->key);
	/* skip the first entry as this is part of the array */
	hash_entry = hash_entry->next;
        while (hash_entry)
	{
	  hash_entry_t *old = hash_entry;
	  hash_entry = hash_entry->next;
	  free (old->key);
	  free (old);
	}
    }

    free (hash->entries);

    hash->entries = NULL;
    hash->n_buckets = 0;
}

void *hash_table_get(hash_table_t *hash, const char *key)
{
  int ndx= hash_index(hash, key);
  hash_entry_t *hash_entry = hash->entries + ndx;
  while (hash_entry)
  {
    if (hash_entry->key)
    {
      if (strcmp(key, hash_entry->key) == 0) {
	 hash->n_hits++;
	 return hash_entry->data;
      }
    }
    hash_entry = hash_entry->next;
  }
  hash->n_misses++;
  return NULL;
}

int hash_table_insert(hash_table_t *hash, const char *key, void *value)
{
     int bucket_len = 0;
     int ndx= hash_index(hash, key);
     hash_entry_t *hash_entry = hash->entries + ndx;
     if (hash_entry->key) {
	  if (strcmp(hash_entry->key, key) == 0) {
	       /* alread in table, update the value */
	       hash_entry->data = value;
	       return 0;
	  } else {
	       /*
		* if this is a collision, we have to go to the end of the ll,
		* then add a new entry
		* before we can hook up the value
		*/
	       while (hash_entry->next) {
		    hash_entry = hash_entry->next;
                    if (strcmp(hash_entry->key, key) == 0) {
                        hash_entry->data = value;
                        return 0;
                    }
		    bucket_len++;
               }
	       hash_entry->next = xcalloc(1, sizeof(hash_entry_t));
	       hash_entry = hash_entry->next;
	       hash_entry->next = NULL;

	       hash->n_collisions++;
	       if (++bucket_len > hash->max_bucket_len)
		    hash->max_bucket_len = bucket_len;
	  }
     } else
	  hash->n_used_buckets++;

     hash->n_elements++;
     hash_entry->key = xstrdup(key);
     hash_entry->data = value;

     return 0;
}

int hash_table_remove(hash_table_t *hash, const char *key)
{
    int ndx= hash_index(hash, key);
    hash_entry_t *hash_entry = hash->entries + ndx;
    hash_entry_t *next_entry=NULL, *last_entry=NULL;
    while (hash_entry)
    {
        if (hash_entry->key)
        {
            if (strcmp(key, hash_entry->key) == 0) {
                free(hash_entry->key);
                if (last_entry) {
                    last_entry->next = hash_entry->next;
                    free(hash_entry);
                } else {
                    next_entry = hash_entry->next;
                    if (next_entry) {
                        memmove(hash_entry, next_entry, sizeof(hash_entry_t));
                        free(next_entry);
                    } else {
                        memset(hash_entry, 0 , sizeof(hash_entry_t));
                    }
                }
                return 1;
            }
        }
        last_entry = hash_entry;
        hash_entry = hash_entry->next;
    }
    return 0;
}

void hash_table_foreach(hash_table_t *hash, void (*f)(const char *key, void *entry, void *data), void *data)
{
    int i;
    if (!hash || !f)
	return;

    for (i = 0; i < hash->n_buckets; i++) {
	hash_entry_t *hash_entry = (hash->entries + i);
	do {
	    if(hash_entry->key) {
		f(hash_entry->key, hash_entry->data, data);
	    }
	} while((hash_entry = hash_entry->next));
    }
}

