/**
 * This hashtable.c is implemented by Jacob Sorber
 * the source video is at https://www.youtube.com/watch?v=KI_V91UdL1I
 */

#include "include/hashtable.h"
#include <stdio.h>
#include <string.h>

typedef struct entry {
  char* key;
  void* object;
  struct entry* next;
} entry;

typedef struct _hash_table {
  uint32_t size;
  hashfunction* hash;
  entry** elements;
} hash_table;

static size_t hash_table_index(hash_table* ht, const char* key) {
  size_t result = (ht->hash(key) % ht->size);
  return result;
}

uint64_t djb2_hash(const char* str) {
  // http://www.cse.yorku.ca/~oz/hash.html
  unsigned int hash = 5381;
  int c;
  while ((c = *str++)) {
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  }
  return hash;
}

hash_table* hash_table_create(uint32_t size, hashfunction* hf) {
  hash_table* ht = malloc(sizeof(*ht));
  ht->size = size;
  if (hf != NULL) {
    ht->hash = hf;
  } else {
    ht->hash = djb2_hash;
  }
  // note that calloc zeros out the memory
  ht->elements = calloc(sizeof(entry*), ht->size);
  return ht;
};

void hash_table_destroy(hash_table* ht) {
  // what to do about individual elements
  for (uint32_t i = 0; i < ht->size; i++) {
    free(ht->elements[i]);
  }
  free(ht->elements);
  free(ht);
};

void hash_table_print(hash_table* ht) {
  printf("Start Table\n");
  for (uint32_t i = 0; i < ht->size; i++) {
    if (ht->elements[i] == NULL) {
      //   printf("\t%i\t---\n", i);
    } else {
      printf("\t%i\t\n", i);
      entry* tmp = ht->elements[i];
      while (tmp != NULL) {
        printf("\"%s\"(%p) - ", tmp->key, tmp->object);
        tmp = tmp->next;
      }
      printf("\n");
    }
  }
  printf("End Table\n");
};

bool hash_table_insert(hash_table* ht, const char* key, void* obj) {
  if (key == NULL || obj == NULL || ht == NULL)
    return false;
  size_t index = hash_table_index(ht, key);

  if (hash_table_lookup(ht, key) != NULL)
    return false;

  // create a new entry
  entry* e = malloc(sizeof(*e));
  e->object = obj;
  // NOTE: do not make assumption key is relly string.
  e->key = malloc(strlen(key) + 1);
  strcpy(e->key, key);

  // insert entry
  e->next = ht->elements[index];
  ht->elements[index] = e;
  return true;
};

bool hash_table_update(hash_table* ht, const char* key, void* obj) {
  if (key == NULL || obj == NULL || ht == NULL)
    return false;
  size_t index = hash_table_index(ht, key);

  entry* tmp = ht->elements[index];
  while (tmp != NULL && strcmp(tmp->key, key) != 0) {
    tmp = tmp->next;
  }
  if (tmp == NULL)
    return false;
  tmp->object = obj;
  return true;
};

bool hash_table_upsert(hash_table* ht, const char* key, void* obj) {
  if (key == NULL || obj == NULL || ht == NULL)
    return false;
  entry* tmp = hash_table_lookup(ht, key);
  if (tmp == NULL) {
    return hash_table_insert(ht, key, obj);
  } else {
    return hash_table_update(ht, key, obj);
  }
};

void* hash_table_lookup(hash_table* ht, const char* key) {
  if (key == NULL || ht == NULL)
    return NULL;
  size_t index = hash_table_index(ht, key);

  entry* tmp = ht->elements[index];
  while (tmp != NULL && strcmp(tmp->key, key) != 0) {
    tmp = tmp->next;
  }
  if (tmp == NULL)
    return NULL;
  return tmp->object;
};

bool hash_table_delete(hash_table* ht, const char* key) {
  if (key == NULL || ht == NULL)
    return false;
  size_t index = hash_table_index(ht, key);

  entry* tmp = ht->elements[index];
  entry* prev = NULL;
  while (tmp != NULL && strcmp(tmp->key, key) != 0) {
    prev = tmp;
    tmp = tmp->next;
  }
  if (tmp == NULL)
    return NULL;
  if (prev == NULL) {
    // deleting the head of the list
    ht->elements[index] = tmp->next;
  } else {
    // deleting from somewhere not the head
    prev->next = tmp->next;
  }
  // free(tmp->object);
  free(tmp);
  return true;
};