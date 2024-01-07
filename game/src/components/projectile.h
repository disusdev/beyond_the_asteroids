#ifndef __PROJECTILE_H__
#define __PROJECTILE_H__

#include <defines.h>
#include <systems/component_system.h>
#include "collision.h"

typedef struct
{
  i32 entity_id;
  i32 target_id;

  t_co_ship* ship;

  Vector3 dir;

  f32 life_timer;

  f32 speed;
} t_co_projectile;


int
co_projectile_create(int entity_id, int cfg_id)
{
  t_co_projectile proj = {0};
  proj.entity_id = entity_id;
  proj.speed = 40.0f;
  proj.life_timer = 3.0f;
  proj.target_id = -1;

  return component_system_insert(entity_id, "co_projectile", &proj);
}


void
projectile_system_init()
{
  component_system_add_component("co_projectile", sizeof(t_co_projectile), 32, 0, &co_projectile_create);
}


void
projectile_system_update(f32 dt, b32 (*damage)(i32, i32))
{
  u64 count = 0;
  t_co_projectile* projs = cast_ptr(t_co_projectile) component_system_get("co_projectile", &count);
  for (u64 i = 0; i < count; i++)
  {
    if (!component_system_entity_is_enabled(projs[i].entity_id))
    {
      continue;
    }

    Matrix transform = component_system_get_global_transform(projs[i].entity_id);

    if (projs[i].target_id == -1)
    {
      projs[i].life_timer -= dt;
      if (projs[i].life_timer <= 0.0f)
      {
        component_system_push(projs[i].entity_id);
        return;
      }
    }
    else
    {
      // follow target
      Vector3 pos = extract_position(&transform);

      Matrix target_transform = component_system_get_global_transform(projs[i].target_id);
      Vector3 target_pos = extract_position(&target_transform);

      projs[i].dir = Vector3Subtract(target_pos, pos);
    }
    
    Vector3 vel = Vector3Scale(Vector3Normalize(projs[i].dir), projs[i].speed * dt);

    Matrix translate = MatrixTranslate(vel.x, vel.y, vel.z);
    transform = MatrixMultiply(transform, translate);

    component_system_set_local_transform(projs[i].entity_id, transform);

    u64 coll_count = 0;
    t_co_collision* collisions = cast_ptr(t_co_collision) component_system_get("co_collision", &coll_count);
    for (u64 j = 0; j < coll_count; j++)
    {
      if (collisions[j].layer == 1) continue;

      t_entity* entities = component_system_get_entities(0);

      if (!entities[collisions[j].entity_id].enabled)
      {
        return;
      }

      Matrix m1 = component_system_get_global_transform(collisions[j].entity_id);
      Vector3 forward = (Vector3) {m1.m8, m1.m9, m1.m10};
      forward = Vector3Scale(forward, collisions[j].offset.z);
      Vector3 right = (Vector3) {m1.m0, m1.m1, m1.m2};
      right = Vector3Scale(right, collisions[j].offset.x);
      Vector3 c1_pos = Vector3Add(Vector3Add(extract_position(&m1), forward), right);

      
      if (CheckCollisionSpheres(c1_pos, collisions[j].radius, extract_position(&transform), 0.2f))
      {
        if (damage(collisions[j].entity_id, 20))
        {
          projs[i].ship->target = -1;
        }
        component_system_push(projs[i].entity_id);
      }
    }
  }
}



#endif