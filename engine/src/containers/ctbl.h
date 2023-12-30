#ifndef CTBL_H
#define CTBL_H

#include <defines.h>


#define T t_ctbl
typedef struct T *T;
#define ctbl(a, b) t_ctbl


extern T
ctbl_create(int hint,
            int cmp(const void *x, const void *y),
            u64 hash(const void *key));


extern T
ctbl_destroy(T *table);


extern int
ctbl_size(T table);


extern i32
ctbl_capacity(T table);


extern void*
ctbl_insert(T table,
            const void *key,
            void *value);


extern b8
ctbl_exist(T table,
           const void *key);


extern void*
ctbl_get(T table,
         const void *key);


extern void*
ctbl_remove(T table,
            const void *key);


extern void
ctbl_map(T table,
         void apply(const void *key, void **value, void *cl),
         void *cl);


extern void**
ctbl_to_array(T table,
              void *end);


u64
ctbl_hash_str(const void *key);


i32
ctbl_cmp_str(const void *x, const void *y);


#undef T
#endif