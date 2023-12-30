#include "component_system.h"

#include <core/mem.h>
#include <core/logger.h>
#include <core/asserts.h>

#include <containers/cvec.h>
#include <containers/ctbl.h>

#include <raylib.h>
#include <raymath.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>


ctbl(const char*, void*) components_dup_fns;
ctbl(const char*, void*) components_create_fns;


typedef struct
{
  ctbl(const char*, void*) components;
  cvec(t_hierarchy) hierarchy;
  cvec(t_entity) entities;
  cvec(Matrix) local_transforms;
  cvec(Matrix) global_transforms;
  cvec(i32) changed_this_frame;
  cvec(i32) remove_this_frame;
  cvec(i32) entity_pool;
} t_component_system_state;


// @fix Should be allocated with systems mem allocator
// @fix Add default state, empty scene
static int current_state = 0;
static cvec(t_component_system_state) states;


RayCollision
IntersectRayPlane(Ray ray, Plane plane)
{
  RayCollision coll = {0};
  
  float denom = Vector3DotProduct(plane.normal, ray.direction);
  
  if (denom > 1e-6)
  {
    Vector3 offset = Vector3Subtract(plane.position, ray.position);
    coll.distance = Vector3DotProduct(offset, plane.normal) / denom;
    coll.hit = (coll.distance >= 0);
    coll.normal = Vector3Scale(plane.normal, -1);
    coll.point = Vector3Add(ray.position, Vector3Scale(ray.direction, coll.distance));
  }
  
  return coll;
}


Vector3 extract_position(Matrix* mat)
{
  return (Vector3) { mat->m12, mat->m13, mat->m14 };
}


void update_position(Matrix* mat, Vector3 pos)
{
  mat->m12 = pos.x;
  mat->m13 = pos.y;
  mat->m14 = pos.z;
}


void update_rotation(Matrix* mat, f32 rot)
{
  *mat = MatrixRotateZ(rot);
}


t_entity*
component_system_get_entities(u64* size)
{
  if (size != 0) *size = cvec_header(states[current_state].entities)->size;
  return cast_ptr(t_entity) states[current_state].entities;
}


t_hierarchy
component_system_get_hierarchy(int id)
{
  return states[current_state].hierarchy[id];
}


Matrix
component_system_get_local_transform(i32 idx)
{
  return states[current_state].local_transforms[idx];
}


Matrix
component_system_get_global_transform(i32 idx)
{
  return states[current_state].global_transforms[idx];
}


static void
childs_apply(int idx, void (*apply)(int idx));


static void
siblings_apply(int idx, void (*apply)(int idx))
{
  int sibling = states[current_state].hierarchy[idx].right_sibling;
  while (sibling != -1)
  {
    // @warning recursion detected, check the memory usage,
    //          should be low, but check to be sure
    childs_apply(sibling, apply);
    sibling = states[current_state].hierarchy[sibling].right_sibling;
  }
}


void
childs_apply(int idx, void (*apply)(int idx))
{
  while (idx != -1)
  {
    apply(idx);

    idx = states[current_state].hierarchy[idx].first_child;
    if (idx != -1)
    {
      siblings_apply(idx, apply);
    }
  }
}


static void
mark_entity_dirty(int idx)
{
  states[current_state].changed_this_frame = cvec_push(states[current_state].changed_this_frame, (u8*) & idx);
}


void
component_system_set_local_transform(i32 idx, Matrix transform)
{
  states[current_state].local_transforms[idx] = transform;
  childs_apply(idx, &mark_entity_dirty);
}


static void
remove_entity(int idx)
{
  states[current_state].hierarchy[idx].id = -1;
}


static void
shift_hierarchy(int removed_id)
{
  u64 hierarchy_count = cvec_header(states[current_state].hierarchy)->size;
  for (u64 i = 0; i < hierarchy_count; i++)
  {
    if (states[current_state].hierarchy[i].id > removed_id)
    {
      states[current_state].hierarchy[i].id -= 1;
    }
    if (states[current_state].hierarchy[i].first_child > removed_id)
    {
      states[current_state].hierarchy[i].first_child -= 1;
    }
    if (states[current_state].hierarchy[i].left_sibling > removed_id)
    {
      states[current_state].hierarchy[i].left_sibling -= 1;
      if (states[current_state].hierarchy[i].left_sibling == removed_id)
      {
        printf("We in trouble!\n");
      }
    }
    if (states[current_state].hierarchy[i].right_sibling > removed_id)
    {
      states[current_state].hierarchy[i].right_sibling -= 1;
    }
  }
}


static Matrix
calculate_global_transform(int entity_id)
{
  Matrix global = MatrixIdentity();
  while (entity_id != -1)
  {
    Matrix local = states[current_state].local_transforms[entity_id];
    global = MatrixMultiply(global, local);
    entity_id = states[current_state].hierarchy[entity_id].parent_id;
  }
  return global;
}


void
component_system_update_global_transform(int entity_id)
{
  states[current_state].global_transforms[entity_id] = calculate_global_transform(entity_id);
}


bool
is_parent_checked_for_remove(int id)
{
  while (id != -1)
  {
    int left = states[current_state].hierarchy[id].left_sibling;
    if (left != -1)
    {
      id = left;
    }
    else
    {
      id = states[current_state].hierarchy[id].parent_id;
      u64 remove_this_frame_count = cvec_header(states[current_state].remove_this_frame)->size;
      for (u64 i = 0; i < remove_this_frame_count; i++)
      {
        int remove_id = states[current_state].remove_this_frame[i];
        if (remove_id == id)
        {
          return true;
        }
      }
      return false;
    }
  }
  return false;
}


int compare(const void *a, const void *b)
{
  return (*(int*)b - *(int*)a);
}


void
component_system_update()
{
  u64 remove_this_frame_count = cvec_header(states[current_state].remove_this_frame)->size;
  if (remove_this_frame_count > 0)
  {
    int remove_size = remove_this_frame_count;
    for (int i = remove_size - 1; i >= 0; i--)
    {
      int id = states[current_state].remove_this_frame[i];
      if (is_parent_checked_for_remove(id))
      {
        states[current_state].remove_this_frame = cvec_remove(states[current_state].remove_this_frame, i);
      }
    }

    qsort(states[current_state].remove_this_frame, cvec_header(states[current_state].remove_this_frame)->size, sizeof(int), compare);

    remove_this_frame_count = cvec_header(states[current_state].remove_this_frame)->size;
    for (u64 i = 0; i < remove_this_frame_count; i++)
    {
      int id = states[current_state].remove_this_frame[i];

      childs_apply(id, &remove_entity);

      {// proccess parent|child hierarchy
        if (states[current_state].hierarchy[id].left_sibling != -1)
        {
          states[current_state].hierarchy[states[current_state].hierarchy[id].left_sibling].right_sibling = states[current_state].hierarchy[id].right_sibling;
        }
        else if (states[current_state].hierarchy[id].depth != 0)
        {
          u64 hierarchy_count = cvec_header(states[current_state].hierarchy)->size;
          for (u64 j = 0; j < hierarchy_count; j++)
          {
            if (states[current_state].hierarchy[j].depth != (states[current_state].hierarchy[id].depth - 1)) continue;
            if (states[current_state].hierarchy[j].first_child == id)
            {
              states[current_state].hierarchy[j].first_child = -1;
              break;
            }
          } 
        }
      }  

      u64 hierarchy_count = cvec_header(states[current_state].hierarchy)->size;
      for (int j = hierarchy_count - 1; j >= 0; j--)
      {
        if (states[current_state].hierarchy[j].id == -1)
        {
          {// remove all components related to this entity
            void** components = ctbl_to_array(states[current_state].entities[j].components, 0);
            u64 components_count = (u64) ctbl_size(states[current_state].entities[j].components);
            for (u64 k = 0; k < components_count; k++)
            {
              // i * 2 + 1
              const char* key = components[k * 2];
              u64 id = (u64) components[k * 2 + 1];
              component_system_remove(key, id);
            }
            mem_free(components, sizeof(*components), MEM_TAG_NONE);
          }
          ctbl_destroy(&states[current_state].entities[j].components);
          states[current_state].entities = cvec_remove(states[current_state].entities, j);
          states[current_state].local_transforms = cvec_remove(states[current_state].local_transforms, j);
          states[current_state].global_transforms = cvec_remove(states[current_state].global_transforms, j);
          states[current_state].hierarchy = cvec_remove(states[current_state].hierarchy, j);

          shift_hierarchy(j);
        }
      }
      
    }
    cvec_clear(states[current_state].remove_this_frame);
  }

  // @fix? probably should remove ids of already deleted entities,
  //       or skip one frame
  u64 changed_this_frame_count = cvec_header(states[current_state].changed_this_frame)->size;
  if (changed_this_frame_count > 0)
  {
    for (u64 i = 0; i < changed_this_frame_count; i++)
    {
      i32 entity_id = states[current_state].changed_this_frame[i];
      states[current_state].global_transforms[entity_id] = calculate_global_transform(entity_id);
    }
    cvec_clear(states[current_state].changed_this_frame);
  }
}


void
draw_hierarchy()
{
  u64 hierarchy_count = cvec_header(states[current_state].hierarchy)->size;
  for (u64 i = 0; i < hierarchy_count; i++)
  {
    if (states[current_state].hierarchy[i].first_child != -1)
    {
      Matrix start_mtx = component_system_get_global_transform(i);
      Matrix end_mtx = component_system_get_global_transform(states[current_state].hierarchy[i].first_child);
      Vector3 start = (Vector3){start_mtx.m12, start_mtx.m13, start_mtx.m14};
      Vector3 end = (Vector3){end_mtx.m12, end_mtx.m13, end_mtx.m14};
      Vector3 up = (Vector3){0,1,0};
      DrawLine3D(Vector3Add(start, up), Vector3Add(end, up), GREEN);
    }

    if (states[current_state].hierarchy[i].right_sibling != -1)
    {
      Matrix start_mtx = component_system_get_global_transform(i);
      Matrix end_mtx = component_system_get_global_transform(states[current_state].hierarchy[i].right_sibling);
      Vector3 start = (Vector3){start_mtx.m12, start_mtx.m13, start_mtx.m14};
      Vector3 end = (Vector3){end_mtx.m12, end_mtx.m13, end_mtx.m14};
      Vector3 up = (Vector3){0,1,0};
      DrawLine3D(Vector3Add(start, up), Vector3Add(end, up), YELLOW);
    }
  }
}


static int
calculate_depth(int id)
{
  int depth = 0;
  while (id != -1)
  {
    depth++;
    id = states[current_state].hierarchy[id].parent_id;
  }
  return depth;
}


static int
find_left_sibling(int depth, int parent)
{
  int sibling = -1;
  u64 hierarchy_count = cvec_header(states[current_state].hierarchy)->size;
  for (u64 i = 0; i < hierarchy_count; i++)
  {
    if (states[current_state].hierarchy[i].depth == depth &&
        states[current_state].hierarchy[i].parent_id == parent &&
        states[current_state].hierarchy[i].right_sibling == -1)
    {
      sibling = i;
      break;
    }
  }
  return sibling;
}


static void
set_disabled(int idx)
{
  t_entity* entity = &component_system_get_entities(0)[idx];
  entity->enabled = false;
}


static void
set_enabled(int idx)
{
  t_entity* entity = &component_system_get_entities(0)[idx];
  entity->enabled = true;
}


void
component_system_push(int entity_id)
{
  t_entity* entity = &component_system_get_entities(0)[entity_id];

  // set for all childrens
  childs_apply(entity_id, &set_disabled);

  states[current_state].entity_pool = cvec_push(states[current_state].entity_pool, &entity_id);
}


int
component_system_pop(const char* component_id)
{
  u64 count = cvec_header(states[current_state].entity_pool)->size;
  for (u64 i = 0; i < count; i++)
  {
    int id = states[current_state].entity_pool[i];
    t_entity* entity = &component_system_get_entities(0)[id];
    if (ctbl_exist(entity->components, component_id))
    {
      childs_apply(id, &set_enabled);
      states[current_state].entity_pool = cvec_remove(states[current_state].entity_pool, i);
      return id;
    }
  }
  return -1;
}


int
component_system_create_entity(int parent)
{
  t_entity entity = {0};
  entity.enabled = true;

  u64 entities_count = cvec_header(states[current_state].entities)->size;
  int entity_id = entities_count;
  sprintf(entity.name, "game_object_%d", entity_id);
  entity.components = ctbl_create(0, &ctbl_cmp_str, &ctbl_hash_str);
  entity.components_cfgs = ctbl_create(0, &ctbl_cmp_str, &ctbl_hash_str);
  states[current_state].entities = cvec_push(states[current_state].entities, cast_ptr(u8) &entity);

  t_hierarchy hierarchy = {0};
  hierarchy.id = entity_id;
  hierarchy.parent_id = parent;
  hierarchy.depth = calculate_depth(parent);
  hierarchy.left_sibling = find_left_sibling(hierarchy.depth, parent);
  hierarchy.right_sibling = -1;
  hierarchy.first_child = -1;

  if (hierarchy.left_sibling != -1)
  {
    states[current_state].hierarchy[hierarchy.left_sibling].right_sibling = hierarchy.id;
  }
  else
  if (parent != -1)
  {
    states[current_state].hierarchy[parent].first_child = hierarchy.id;
  }

  if (hierarchy.left_sibling == entity_id)
  {
    printf("We in trouble!\n");
  }

  states[current_state].hierarchy = cvec_push(states[current_state].hierarchy, cast_ptr(u8) & hierarchy);

  Matrix local = MatrixIdentity();
  states[current_state].local_transforms = cvec_push(states[current_state].local_transforms, cast_ptr(u8) &local);

  Matrix global = calculate_global_transform(entity_id);
  states[current_state].global_transforms = cvec_push(states[current_state].global_transforms, cast_ptr(u8) &global);

  return entity_id;
}


int
component_system_duplicate_entity(int id)
{
  int new_entity_id = component_system_create_entity(-1);

  t_entity* entity = &component_system_get_entities(0)[id];
  t_entity* new_entity = &component_system_get_entities(0)[new_entity_id];

  component_system_set_local_transform(new_entity_id, component_system_get_global_transform(id));

  void** components_array = ctbl_to_array(entity->components, 0);
  u64 components_count = (u64) ctbl_size(entity->components);
  for (u64 i = 0; i < components_count; i += 1)
  {
    const char* component_key = (const char*) components_array[i * 2 + 0];
    int comp_id = (u64) ctbl_get(entity->components, component_key);

    int (*dup_func)(int, int) = (int (*)(int, int)) ctbl_get(components_dup_fns, component_key);
    u64 new_comp_id = (u64) dup_func(new_entity_id, comp_id);

    ctbl_insert(new_entity->components, component_key, (void*) new_comp_id);
  }

  return new_entity_id;
}


void
component_system_remove_entity(int id)
{
  // add id to remove later
  states[current_state].remove_this_frame = cvec_push(states[current_state].remove_this_frame, cast_ptr(u8) & id);
}


void
component_system_add_component(const char* component_id, u64 size, u64 default_capacity, int (*dup_comp)(int, int), int (*create_comp)(int, int))
{
  if (ctbl_get(states[current_state].components, component_id))
  {
    ASSERT(false);
  }

  ctbl_insert(states[current_state].components, component_id, cvec_create(size, default_capacity, false));

  ctbl_insert(components_dup_fns, component_id, dup_comp);
  ctbl_insert(components_create_fns, component_id, create_comp);
}


void*
component_system_get(const char* component_id, u64* size)
{
  void* components_vec = ctbl_get(states[current_state].components, component_id);
  ASSERT(components_vec);

  if (size != 0)
  {
    *size = cvec_header(components_vec)->size;
  }
  return components_vec;
}


int
component_system_parent(int idx)
{
  return states[current_state].hierarchy[idx].parent_id;
}


void*
component_system_at(const char* component_id, int idx)
{
  void* components_vec = ctbl_get(states[current_state].components, component_id);
  ASSERT(components_vec);
  return cvec_at(components_vec, idx);
}


int
component_system_insert(int entity_id, const char* component_id, void* component)
{
  void* components_vec = ctbl_get(states[current_state].components, component_id);
  ASSERT(components_vec);
  i32 idx = cvec_header(components_vec)->size;
  components_vec = cvec_push(components_vec, component);
  // should swap with proper pointer
  ctbl_insert(states[current_state].components, component_id, components_vec);
  ctbl_insert(states[current_state].entities[entity_id].components, component_id, (void*)(u64)idx);
  return idx;
}


static void
shift_entities_components(const char* component_id, int remove_id)
{
  u64 entities_count = cvec_header(states[current_state].entities)->size;
  for (u64 i = 0; i < entities_count; i++)
  {
    int comp_id = (u64) ctbl_get(states[current_state].entities[i].components, component_id);
    if (comp_id <= remove_id) continue;
    comp_id -= 1;
    if (comp_id == -1)
    {
      continue;
    }
    ASSERT(comp_id != -1);
    ctbl_remove(states[current_state].entities[i].components, component_id);
    ctbl_insert(states[current_state].entities[i].components, component_id, (void*) (u64) comp_id);
  }
}


void
component_system_remove(const char* component_id, int id)
{
  void* components_vec = ctbl_get(states[current_state].components, component_id);
  components_vec = cvec_remove(components_vec, id);

  ctbl_remove(states[current_state].components, component_id);
  ctbl_insert(states[current_state].components, component_id, components_vec);

  shift_entities_components(component_id, id);
}


void
component_system_init()
{
  states = cvec_create(sizeof(t_component_system_state), 4, false);

  states = cvec_push_new(states, 0);

  components_dup_fns = ctbl_create(0, &ctbl_cmp_str, &ctbl_hash_str);
  components_create_fns = ctbl_create(0, &ctbl_cmp_str, &ctbl_hash_str);

  states[current_state].components = ctbl_create(0, &ctbl_cmp_str, &ctbl_hash_str);

  states[current_state].hierarchy = cvec_create(sizeof(t_hierarchy), 1024, false);
  states[current_state].entities = cvec_create(sizeof(t_entity), 1024, false);
  states[current_state].local_transforms = cvec_create(sizeof(Matrix), 1024, false);
  states[current_state].global_transforms = cvec_create(sizeof(Matrix), 1024, false);
  states[current_state].changed_this_frame = cvec_create(sizeof(i32), 128, false);
  states[current_state].remove_this_frame = cvec_create(sizeof(i32), 128, false);

  states[current_state].entity_pool = cvec_create(sizeof(i32), 128, false);
}


void
component_system_term()
{
  void** components_array = ctbl_to_array(states[current_state].components, 0);
  u64 components_count = (u64) ctbl_size(states[current_state].components);

  for (u64 i = 1; i < (components_count - 1) * 2; i += 2) 
  {
    // @fix? should call destroy for component if he needs one
    if (components_array[i])
    {
      cvec_destroy(components_array[i]);
    }
  }
  ctbl_destroy(&states[current_state].components);
  mem_free(components_array, sizeof(components_array), MEM_TAG_NONE);

  u64 entities_count = cvec_header(states[current_state].entities)->size;
  for (u64 i = 0; i < entities_count; i++)
  {
    ctbl_destroy(&states[current_state].entities[i].components);
  }
  cvec_destroy(states[current_state].entities);

  cvec_destroy(states[current_state].local_transforms);
  cvec_destroy(states[current_state].global_transforms);
}


int
component_system_add_new_state()
{
  u64 states_count = cvec_header(states)->size;
  int id = states_count;

  states = cvec_push_new(states, 0);

  states[id].components = ctbl_create(0, &ctbl_cmp_str, &ctbl_hash_str);

  {
    u64 components_count = ctbl_size(states[0].components);
    void** components_arr = ctbl_to_array(states[0].components, 0);
    for (u64 i = 0; i < components_count; i++)
    {
      const char* key = components_arr[i * 2 + 0];
      void* vec = components_arr[i * 2 + 1];
      ctbl_insert(states[id].components, key, cvec_create(cvec_header(vec)->stride, 1024, false));
    }
    mem_free(components_arr, sizeof(components_arr), MEM_TAG_NONE);
  }

  states[id].hierarchy = cvec_create(sizeof(t_hierarchy), 1024, false);
  states[id].entities = cvec_create(sizeof(t_entity), 1024, false);
  states[id].local_transforms = cvec_create(sizeof(Matrix), 1024, false);
  states[id].global_transforms = cvec_create(sizeof(Matrix), 1024, false);
  states[id].changed_this_frame = cvec_create(sizeof(i32), 128, false);
  states[id].remove_this_frame = cvec_create(sizeof(i32), 128, false);
  states[id].entity_pool = cvec_create(sizeof(i32), 128, false);

  return id;
}


void
component_system_remove_state(int state_id)
{
  states = cvec_remove(states, state_id);
}


void
component_system_switch_state(int state_id)
{
  int states_count = (int) cvec_header(states)->size;
  ASSERT(state_id < states_count);
  current_state = state_id;
}


int
component_system_create_component_by_cfg(int entity_id, const char* component_key, int cfg_id)
{
  int (*create_func)(int, int) = (int (*)(int, int)) ctbl_get(components_create_fns, component_key);
  return create_func(entity_id, cfg_id);
}


int
component_system_create_component(int entity_id, const char* component_key)
{
  int (*create_func)(int, int) = (int (*)(int, int)) ctbl_get(components_create_fns, component_key);
  return create_func(entity_id, -1);
}


void*
component_system_create_component_ptr(int entity_id, const char* component_key)
{
  int (*create_func)(int, int) = (int (*)(int, int)) ctbl_get(components_create_fns, component_key);
  u8* components = component_system_get(component_key, 0);
  u64 stride = cvec_header(components)->stride;
  i32 id = create_func(entity_id, -1);
  return (components + id * stride);
}


b32
component_system_entity_is_enabled(int entity_id)
{
  return states[current_state].entities[entity_id].enabled;
}


void
component_system_entity_set_enabled(int entity_id, b32 enabled)
{
  states[current_state].entities[entity_id].enabled = enabled;
}


void*
component_system_entity_get_component(i32 entity_id, const char* component_id)
{
  t_entity* entity = &component_system_get_entities(0)[entity_id];
  if (ctbl_exist(entity->components, component_id))
  {
    i32 id = (u64) ctbl_get(entity->components, component_id);
    u8* components = component_system_get(component_id, 0);
    u64 stride = cvec_header(components)->stride;
    return (components + id * stride);
  }
  return 0;
}
