#ifndef MESH_SYSTEM_H
#define MESH_SYSTEM_H

#include <defines.h>

#include <raylib.h>


void
mesh_system_init();


void
mesh_system_get_model_by_id(i32 idx, Model* model);


i32
mesh_system_get_model(const char* path, Model* model);


i32
mesh_system_model_upload(const char* path,
                         Model* m);


void
mesh_system_clear();


const char*
mesh_system_get_src_path(i32 idx);


#endif