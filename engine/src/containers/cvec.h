/* date = October 16th 2023 8:19 am */

#ifndef CVEC_H
#define CVEC_H

#include <defines.h>

typedef struct
{
  u8* data;
  u64 stride;
  u64 size;
  u64 capacity;
} t_cvec;

#define cvec(vec_type) vec_type*

t_cvec* cvec_header(void* vec);

void* cvec_create(u64 stride, u64 capacity, b8 resize);

void cvec_destroy(void* vec);

void* cvec_at(void* vec, u64 idx);

void* cvec_resize(void* vec, u64 new_capacity);

void* cvec_push(void* vec, u8* data);

void* cvec_remove(void* vec, u64 idx);

void* cvec_push_new(void* vec, u8** elem);

void* cvec_back(void* vec);

void cvec_clear(void* vec);

void cvec_shift(u8* vec, u64 from, u64 size);

#endif //CVEC_H
