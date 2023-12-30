#include "mesh_system.h"

#include <containers/cvec.h>
#include <containers/ctbl.h>
#include <core/mem.h>

#include <raylib.h>
#include <rlgl.h>
#include <string.h>


static cvec(Model) models_to_unload;


static i32 cached_paths_count = 0;
static char cached_paths[1024][128];


void
mesh_system_init()
{
  models_to_unload = cvec_create(sizeof(Model), 16, false);
}


void
mesh_system_get_model_by_id(i32 idx, Model* model)
{
  *model = ptr2(Model) cvec_at(models_to_unload, idx);
}


i32
mesh_system_get_model(const char* path, Model* model)
{
  for (i32 i = 0; i < cached_paths_count; i++)
  {
    if (strcmp(path, cached_paths[i]) == 0)
    {
      *model = ptr2(Model) cvec_at(models_to_unload, i);
      return i;
    }
  }

  Model m = LoadModel(path);
  *model = m;
  return mesh_system_model_upload(path, &m);
}


i32
mesh_system_model_upload(const char* path,
                         Model* m)
{
  u64 idx = cvec_header(models_to_unload)->size;
  models_to_unload = cvec_push(models_to_unload, cast_ptr(u8) m);
  strcat(cached_paths[idx], path);
  cached_paths_count++;
  return (i32) idx;
}


void
mesh_system_clear()
{
  u64 models_count = cvec_header(models_to_unload)->size;
  for (u64 i = 0; i < models_count; i++)
  {
    Model* model = cvec_at(models_to_unload, i);
    UnloadModel(*model);
  }
  cvec_clear(models_to_unload);
}


const char*
mesh_system_get_src_path(i32 idx)
{
  return cached_paths[idx];
}
