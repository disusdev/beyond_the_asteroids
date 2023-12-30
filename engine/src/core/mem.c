#include "mem.h"
#include <stdlib.h>

#include <core/logger.h>
#include <defines.h>

// #include <platform/platform.h>

static b8 log_allocations = false;

void
set_allocated(u64 size, e_mem_tag tag, void* block)
{
  if (log_allocations)
  {
    LOG_DEBUG("[alloc][%s] block: %p, size: %llu", tag, block, size);
  }
}

void
set_freed(u64 size, e_mem_tag tag, void* block)
{
  if (log_allocations)
  {
    LOG_DEBUG("[free][%s] block: %p, size: %llu", tag, block, size);
  }
}

void*
mem_alloc(u64 size,
          e_mem_tag tag)
{
  void* alloc_mem = malloc(size);
  set_allocated(size, tag, alloc_mem);
  return alloc_mem;// platform_alloc(size, false);
}


void*
mem_calloc(u64 length,
           u64 elem_size,
           e_mem_tag tag)
{
  void* alloc_mem = calloc(length, elem_size);
  set_allocated(elem_size * length, tag, alloc_mem);
  return alloc_mem;// platform_alloc(size, false);
}


void
mem_free(void* block,
         u64 size,
         e_mem_tag tag)
{
  set_freed(size, tag, block);
  free(block);// platform_free(block, false);
}

void*
mem_set(void* dst,
        i32 value,
        u64 size)
{
  u8* ptr = (u8*)dst;
  while (size > 0)
  {
    *ptr = value;
    ptr++;
    size--;
  }
  return dst;
}

void*
mem_set_32(void* dst,
           i32 value,
           u64 size)
{
  i32* ptr = (i32*)dst;
  while (size > 0)
  {
    *ptr = value;
    ptr++;
    size--;
  }
  return dst;
}

void*
mem_zero(void* block,
         u64 size)
{
  return mem_set(block, 0, size);
}

void*
mem_copy(void* dst,
         const void* src,
         u64 size)
{
  u8* ptr_src = (u8*)src;
  u8* ptr_dst = (u8*)dst;
  while (size > 0)
  {
    *ptr_dst = *ptr_src;
    ptr_dst++;
    ptr_src++;
    size--;
  }
  return ptr_dst;
}

b8
mem_cmp(const void* p1,
        const void* p2,
        u64 size)
{
  u8* ptr_src = (u8*)p1;
  u8* ptr_dst = (u8*)p2;
  while (size > 0)
  {
    if (*ptr_dst != *ptr_src)
    {
      return false;
    }
    ptr_dst++;
    ptr_src++;
    size--;
  }
  return true;
}
