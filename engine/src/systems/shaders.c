#include "shaders.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <containers/cvec.h>
#include <containers/ctbl.h>
#include <core/mem.h>
#include <raylib.h>
#include <core/logger.h>
#include <core/asserts.h>

#if  defined(_WIN32)
#include <malloc.h>
#endif


#define SHADER_DEFAULT_CAPACITY 128


static ctbl(const char*, cvec(u8)) cached_map = 0;


void
shaders_init()
{
  cached_map = ctbl_create(0, &ctbl_cmp_str, &ctbl_hash_str);
}


void
shaders_term()
{
  // @todo clear cached strings and map
}


// key should be a file name of the shader
const char*
shaders_get(const char* key)
{
  return ctbl_get(cached_map, key);
}


void
shaders_set(const char* path, cvec(u8) src)
{
  const char* key = GetFileName(path);
  ctbl_insert(cached_map, key, src);
}


cvec(u8)
str_replace(cvec(u8) arr, const char* find, u64 find_size, const char* replace, u64 replace_size)
{
  u64 current_length = strlen((const char*)arr);
  if ((current_length - find_size) + replace_size >= cvec_header(arr)->size)
  {
    arr = cvec_resize(arr, (current_length - find_size) + replace_size + 1);
    arr[(current_length - find_size) + replace_size] = 0;
  }

  u8* from_pos = (u8*) strstr((const char*)arr, find);
  cvec_shift(arr, from_pos - arr, replace_size);
  mem_copy(from_pos, replace, replace_size);

  return arr;
}


cvec(u8) shader_load(const char* path)
{
  const char* key = GetFileName(path);
  if (ctbl_exist(cached_map, key))
  {
    return (u8*) shaders_get(key);
  }

  FILE* file = fopen(path, "r");

	if (!file)
	{
		printf("I/O error. Cannot open shader file '%s'\n", path);
    ASSERT(false);
	}

	fseek(file, 0L, SEEK_END);
	u64 bytesinfile = ftell(file);
	fseek(file, 0L, SEEK_SET);

	char* buffer = (char*)alloca(bytesinfile + 1);
	const size_t bytesread = fread(buffer, 1, bytesinfile, file);
	fclose(file);
	buffer[bytesread] = 0;

  cvec(u8) result = cvec_create(sizeof(u8), bytesinfile + 1, false);

  char replace_path[128] = {0};
  int repl_pos = 0;

  const char* ptr = buffer;
  b8 write_path_mode = false;
  while (*ptr != 0)
  {
    if (!write_path_mode)
    {
      if (*ptr == '#' && memcmp(ptr, "#include <", 10) == 0)
      {
        write_path_mode = true;
        repl_pos = 0;
        ptr += 10;
        continue;
      }
      result = cvec_push(result, (u8*) ptr);
    }
    else
    {
      if (*ptr == '>')
      {
        replace_path[repl_pos] = 0;
        cvec(u8) raw = shader_load(replace_path);
        u64 size = cvec_header(raw)->size - 1;
        for (u64 i = 0; i < size; i++)
        {
          result = cvec_push(result, &raw[i]);
        }
        write_path_mode = false;
        ptr++;
        continue;
      }
      replace_path[repl_pos++] = *ptr;
    }

    ptr++;
  }
  
  char nullch = 0;
  result = cvec_push(result, cast_ptr(u8) & nullch);
  fclose(file);

  shaders_set(path, result);

	return result;
}
