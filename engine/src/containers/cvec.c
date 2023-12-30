#include "cvec.h"

#include <core/mem.h>
#include <core/logger.h>
#include <core/asserts.h>
#include <string.h>


t_cvec* cvec_header(void* vec)
{
  t_cvec* header = (t_cvec*)((u8*)vec - sizeof(t_cvec));
  return header;
}


void* cvec_create(u64 stride, u64 capacity, b8 resize)
{
  u64 header_size = sizeof(t_cvec);
  u64 array_size = stride * capacity;
  void* vec = mem_alloc(header_size + array_size, MEM_TAG_CVEC);

  t_cvec* vec_header = (t_cvec*)vec;
  vec_header->capacity = capacity == 0 ? 2 : capacity;
  vec_header->stride = stride;
  vec_header->size = resize == true ? capacity : 0;
  vec_header->data = (u8*)vec + header_size;

  mem_set(vec_header->data, 0, stride * capacity);

  return vec_header->data;
}

void cvec_destroy(void* vec)
{
  t_cvec* header = cvec_header(vec);
  u64 header_size = sizeof(t_cvec);
  u64 array_size = header->stride * header->capacity;
  mem_free(header, header_size + array_size, MEM_TAG_CVEC);
}

void* cvec_at(void* vec, u64 idx)
{
  t_cvec* header = cvec_header(vec);
  ASSERT(header->size > idx);
  return header->data + header->stride * idx;
}

void* cvec_resize(void* vec, u64 new_capacity)
{
  if (new_capacity == 0) return vec;
  t_cvec* header = cvec_header(vec);
  if (new_capacity <= header->capacity)
  {
    header->size = new_capacity;
    return vec;
  }
  void* new_vec = cvec_create(header->stride, new_capacity, false);
  t_cvec* new_header = cvec_header(new_vec);
  mem_copy(new_header->data, header->data, header->size * header->stride);
  new_header->size = new_capacity;
  cvec_destroy(vec);
  return new_vec;
}


t_cvec* _resize_capacity(t_cvec* header, u64 new_capacity)
{
  if (new_capacity == 0) return header;
  if (new_capacity <= header->capacity)
  {
    header->size = new_capacity;
    return header;
  }
  void* new_vec = cvec_create(header->stride, new_capacity, false);
  t_cvec* new_header = cvec_header(new_vec);
  mem_copy(new_vec, header->data, header->size * header->stride);
  new_header->size = header->size;
  cvec_destroy(header->data);
  return new_header;
}


void* cvec_push(void* vec, u8* data)
{
  t_cvec* header = cvec_header(vec);
  if (header->capacity <= header->size)
  {
    header = _resize_capacity(header, header->capacity * 2);
  }
  u8* place = header->data + header->stride * header->size;
  mem_copy(place, data, header->stride);
  header->size++;
  return header->data;
}


void* cvec_remove(void* vec, u64 idx)
{
  t_cvec* header = cvec_header(vec);
  void* new_vec = cvec_create(header->stride, header->capacity, true);
  u8* new_ptr = mem_copy(new_vec, header->data, idx * header->stride);
  mem_copy(new_ptr, header->data + ((idx + 1) * header->stride), (header->size - idx + 1) * header->stride);
  cvec_header(new_vec)->size = header->size - 1;
  return new_vec;
}


void* cvec_push_new(void* vec, u8** elem)
{
  t_cvec* header = cvec_header(vec);
  if (header->capacity == (header->size + 1))
  {
    header = _resize_capacity(header, header->capacity * 2);
  }
  u8* place = header->data + header->stride * header->size;
  mem_set(place, 0, header->stride);
  header->size++;
  if (elem != 0)
  {
    *elem = place;
  }
  return header->data;
}

void* cvec_back(void* vec)
{
  t_cvec* header = cvec_header(vec);
  u8* place = header->data + header->stride * (header->size - 1);
  return place;
}

void cvec_clear(void* vec)
{
  t_cvec* header = cvec_header(vec);
  mem_set(header->data, 0, header->stride * header->size);
  header->size = 0;
}

void cvec_shift(u8* vec, u64 from, u64 size)
{
  memmove(vec + from + size, vec + from, size);
}