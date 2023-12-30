#include <core/mem.h>
#include <core/logger.h>
#include <core/asserts.h>
#include "ctbl.h"
#include <stdio.h>
#include <string.h>

#define T t_ctbl


struct T
{
  i32 size;
  int (*cmp)(const void *x, const void *y);
  u64 (*hash)(const void *key);
  i32 length;
  u32 timestamp;

  struct binding
  {
    struct binding *link;
    const void *key;
    void *value;
  } **buckets;
};


static i32
cmpatom(const void *x, const void *y)
{
  return x != y;
}


static u64
hashatom(const void *key)
{
  return (u64)key>>2;
}


T
ctbl_create(int hint,
            int cmp(const void *x, const void *y),
            u64 hash(const void *key))
{
  T table;
  i32 i;
  static int primes[] =
  {
    509, 509, 1021, 2053, 4093, 8191, 16381, 32771, 65521, INT_MAX
  };

  ASSERT(hint >= 0);
  for (i = 1; primes[i] < hint; i++);
  table = mem_alloc(sizeof(*table) + primes[i-1] * sizeof(table->buckets[0]), MEM_TAG_NONE);
  table->size = primes[i-1];
  table->cmp = cmp ? cmp : cmpatom;
  table->hash = hash ? hash : hashatom;
  table->buckets = (struct binding **)(table + 1);
  for (i = 0; i < table->size; i++)
  {
    table->buckets[i] = 0;
  }
  table->length = 0;
  table->timestamp = 0;
  
  return table;
}


T
ctbl_destroy(T *table)
{
  ASSERT(table && *table);
  if ((*table)->length > 0)
  {
    i32 i;
    struct binding *p, *q;
    for (i = 0; i < (*table)->size; i++)
    {
      for (p = (*table)->buckets[i]; p; p = q)
      {
        q = p->link;
        mem_free(p, sizeof(struct binding), MEM_TAG_NONE);
        p = 0;
      }
    }
  }
  mem_free(*table, sizeof(**table), MEM_TAG_NONE);
  *table = 0;
  return *table;
}


i32
ctbl_size(T table)
{
  ASSERT(table);
  return table->length;
}


i32
ctbl_capacity(T table)
{
  ASSERT(table);
  return table->size;
}


extern void*
ctbl_insert(T table,
            const void *key,
            void *value)
{
  i32 i;
  struct binding *p;
  void *prev;

  ASSERT(table);
  ASSERT(key);

  i = (*table->hash)(key)%table->size;
  for (p = table->buckets[i]; p; p = p->link)
  {
    if ((*table->cmp)(key, p->key) == 0)
    {
      break;
    }
  }

  if (p == 0)
  {
    p = mem_alloc(sizeof(struct binding), MEM_TAG_NONE);
    p->key = key;
    p->link = table->buckets[i];
    table->buckets[i] = p;
    table->length++;
    prev = 0;
  }
  else
  {
    prev = p->value;
  }
  p->value = value;
  table->timestamp++;
  return prev;
}


extern b8
ctbl_exist(T table,
           const void *key)
{
  i32 i;
  struct binding *p;
  ASSERT(table);
  ASSERT(key);

  i = (*table->hash)(key)%table->size;
  for (p = table->buckets[i]; p; p = p->link)
  {
    if ((*table->cmp)(key, p->key) == 0)
    {
      return true;
    }
  }

  return false;
}


void*
ctbl_get(T table,
         const void *key)
{
  i32 i;
  struct binding *p;
  ASSERT(table);
  ASSERT(key);

  i = (*table->hash)(key)%table->size;
  for (p = table->buckets[i]; p; p = p->link)
  {
    if ((*table->cmp)(key, p->key) == 0)
    {
      break;
    }
  }

  return p ? p->value : 0;
}


extern void*
ctbl_remove(T table,
            const void *key)
{
  i32 i;
  struct binding **pp;

  ASSERT(table);
  ASSERT(key);
  table->timestamp++;
  i = (*table->hash)(key)%table->size;
  for (pp = &table->buckets[i]; *pp; pp = &(*pp)->link)
  {
    if ((*table->cmp)(key, (*pp)->key) == 0)
    {
      struct binding *p = *pp;
      void *value = p->value;
      *pp = p->link;
      mem_free(p, sizeof(struct binding), MEM_TAG_NONE);
      table->length--;
      return value;
    }
  }
  return 0;
}


void
ctbl_map(T table,
         void apply(const void *key, void **value, void *cl),
         void *cl)
{
  i32 i;
  u32 stamp;
  struct binding *p;

  ASSERT(table);
  ASSERT(apply);
  stamp = table->timestamp;
  for (i = 0; i < table->size; i++)
  {
    for (p = table->buckets[i]; p; p = p->link)
    {
      apply(p->key, &p->value, cl);
      ASSERT(table->timestamp == stamp);
    }
  }
}


void**
ctbl_to_array(T table,
              void *end)
{
  i32 i, j = 0;
  void **array;
  struct binding *p;
  ASSERT(table);
  array = mem_alloc((2 * table->length + 1) * sizeof(*array), MEM_TAG_NONE);
  for (i = 0; i < table->size; i++)
  {
    for (p = table->buckets[i]; p; p = p->link)
    {
      array[j++] = (void*)p->key;
      array[j++] = p->value;
    }
  }
  array[j] = end;
  return array;
}


u64
ctbl_hash_str(const void *key)
{
  const u8* str = (const u8*) key;
  u64 hash = 0;
  u8 c;
  while((c = (*str++)))
  {
    hash = c + (hash << 6) + (hash << 16) - hash;
  }
  return hash;
}


i32
ctbl_cmp_str(const void *x, const void *y)
{
  return strcmp(x, y);
}

