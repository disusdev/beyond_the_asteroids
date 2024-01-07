#ifndef COMPONENT_SYSTEM_H
#define COMPONENT_SYSTEM_H

#include <defines.h>
#include <raylib.h>

#include "co_camera.h"
#include "co_renderer.h"
#include "co_animation.h"

#include <containers/cvec.h>
#include <containers/ctbl.h>


typedef struct
{
  char name[128];
  ctbl(const char*, u64) components;
  ctbl(const char*, u64) components_cfgs;
  b32 enabled;
} t_entity;


typedef struct
{
  int id;
  int parent_id;
  int left_sibling;
  int right_sibling;
  int first_child;
  int depth;
} t_hierarchy;


typedef struct
{
  Vector3 position;
  Vector3 normal;
} Plane;


RayCollision
IntersectRayPlane(Ray ray, Plane plane);


Vector3 extract_position(Matrix* mat);


void update_position(Matrix* mat, Vector3 pos);


void update_rotation(Matrix* mat, f32 rot);


void
draw_hierarchy();


void
component_system_push(int entity_id);


int
component_system_pop(const char* component_id);


int
component_system_add_new_state();


void
component_system_remove_state(int state_id);


void
component_system_switch_state(int state_id);


t_entity*
component_system_get_entities(u64* size);


t_hierarchy
component_system_get_hierarchy(int id);


int
component_system_create_entity(int parent);


int
component_system_duplicate_entity(int id);


void
component_system_remove_entity(int id);


Matrix
component_system_get_local_transform(i32 idx);


Matrix
component_system_get_global_transform(i32 idx);


void
component_system_set_local_transform(i32 idx, Matrix transform);


void
component_system_add_local_transform(i32 idx, Matrix transform);


void
component_system_update_global_transform(int entity_id);


void
component_system_update();


void
component_system_add_component(const char* component_id,
                               u64 size,
                               u64 default_capacity,
                               int (*dup_comp)(int, int),
                               int (*create_comp)(int, int));


void*
component_system_get(const char* component_id, u64* size);


int
component_system_parent(int idx);


void*
component_system_at(const char* component_id, int idx);


int
component_system_insert(int entity_id, const char* component_id, void* component);


void
component_system_remove(const char* component_id, int id);


void
component_system_init();


void
component_system_term();


int
component_system_create_component_by_cfg(int entity_id, const char* component_key, int cfg_id);


int
component_system_create_component(int entity_id, const char* component_key);


void*
component_system_create_component_ptr(int entity_id, const char* component_key);


b32
component_system_entity_is_enabled(int entity_id);


void
component_system_entity_set_enabled(int entity_id, b32 enabled);


void*
component_system_entity_get_component(i32 entity_id, const char* component_id);


#endif