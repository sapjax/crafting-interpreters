#ifndef __HASHTABLE__H__
#define __HASHTABLE__H__

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef uint64_t(hashfunction)(const char*);
typedef struct _hash_table hash_table;

hash_table* hash_table_create(uint32_t size, hashfunction* hf);
void hash_table_destroy(hash_table* ht);
void hash_table_print(hash_table* ht);
bool hash_table_insert(hash_table* ht, const char* key, void* obj);
void* hash_table_lookup(hash_table* ht, const char* key);
void* hash_table_delete(hash_table* ht, const char* key);
#endif  //!__HASHTABLE__H__