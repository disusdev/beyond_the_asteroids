#ifndef __COLLISION_H__
#define __COLLISION_H__


#include <defines.h>
#include <systems/component_system.h>


typedef struct co_collision
{
  int entity_id;
  Vector3 offset;
  f32 radius;
  i32 layer;
  Vector3 normal;
  void (*collide_callback)(struct co_collision*, struct co_collision*);
} t_co_collision;


static void (*collide_fn)(t_co_collision* c1, t_co_collision* c2);


int
co_collision_create(int entity_id, int cfg)
{
  t_co_collision coll = {0};
  coll.entity_id = entity_id;
  coll.radius = 1.2;
  coll.layer = 0;
  Matrix transform = component_system_get_global_transform(entity_id);
  return component_system_insert(entity_id, "co_collision", &coll);
}


void
collision_system_init(void (*callback)(t_co_collision* c1, t_co_collision* c2))
{
  collide_fn = callback;
  component_system_add_component("co_collision", sizeof(t_co_collision), 128, 0, &co_collision_create);
}


void
check_collision(t_co_collision* c1,
                t_co_collision* c2)
{
  t_entity* entities = component_system_get_entities(0);
  if (!entities[c1->entity_id].enabled) return;
  if (!entities[c2->entity_id].enabled) return;

  Matrix m1 = component_system_get_global_transform(c1->entity_id);
  Vector3 forward = (Vector3) {m1.m8, m1.m9, m1.m10};
  forward = Vector3Scale(forward, c1->offset.z);
  Vector3 right = (Vector3) {m1.m0, m1.m1, m1.m2};
  right = Vector3Scale(right, c1->offset.x);
  Vector3 c1_pos = Vector3Add(Vector3Add(extract_position(&m1), forward), right);

  Matrix m2 = component_system_get_global_transform(c2->entity_id);
  forward = (Vector3) {m2.m8, m2.m9, m2.m10};
  forward = Vector3Scale(forward, c2->offset.z);
  right = (Vector3) {m2.m0, m2.m1, m2.m2};
  right = Vector3Scale(right, c2->offset.x);
  Vector3 c2_pos = Vector3Add(Vector3Add(extract_position(&m2), forward), right);


  if (CheckCollisionSpheres(c1_pos, c1->radius, c2_pos, c2->radius))
  {
    c1->normal = Vector3Normalize(Vector3Subtract(c1_pos, c2_pos));
    c2->normal = Vector3Normalize(Vector3Subtract(c2_pos, c1_pos));
  
    if (c1->collide_callback)
    {
      c1->collide_callback(c1, c2);
    }
    else
    if (c2->collide_callback)
    {
      c2->collide_callback(c2, c1);
    }
    else
    {
      collide_fn(c1, c2);
    }
  }
}


void
collision_system_update()
{
  u64 coll_count = 0;
  t_co_collision* collisions = cast_ptr(t_co_collision) component_system_get("co_collision", &coll_count);

  for (u64 i = 0; i < coll_count; i++)
  {
    for (u64 j = 0; j < coll_count; j++)
    {
      if (i == j) continue;
      check_collision(&collisions[i], &collisions[j]);
    }
  }
}


#endif