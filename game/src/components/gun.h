#ifndef __GUN_H__
#define __GUN_H__

#include <defines.h>

#include <systems/component_system.h>
#include "components/ship.h"


typedef struct
{
  i32 entity_id;
  t_co_ship* ship;

  f32 frequency;
  i32 ammo_count;
  f32 restore_ammo_time;


  f32 timer;

  i32 state;
  f32 range;

  void (*fire_callback)(Vector3 from, Vector3 to);
} t_co_gun;


int
co_gun_create(i32 entity_id, i32 cfg_id)
{
  t_co_gun gun = {0};
  gun.entity_id = entity_id;

  gun.frequency = 0.5f;
  gun.ammo_count = 3;
  gun.restore_ammo_time = 3.0f;
  
  gun.timer = 0.0f;

  gun.ship = 0;

  gun.state = 0;

  gun.range = 20.0f;

  return component_system_insert(entity_id, "co_gun", &gun);
}


void
gun_system_init()
{
  component_system_add_component("co_gun", sizeof(t_co_gun), 4, 0, &co_gun_create);
}


void
co_gun_update(t_co_gun* gun, f32 dt)
{
  if (!component_system_entity_is_enabled(gun->entity_id)) return;

  // check distance to target
  b32 target_in_range = false;
  if (gun->ship->target != -1)
  {
    Matrix target_transform = component_system_get_global_transform(gun->ship->target);
    Vector3 target_pos = extract_position(&target_transform);
    Matrix ship_transform = component_system_get_global_transform(gun->ship->entity_id);
    Vector3 ship_pos = extract_position(&ship_transform);
    f32 dst = Vector3Distance(target_pos, ship_pos);
    if (dst < gun->range)
    {
      //gun->ship->target = -1;
      target_in_range = true;
    }

    if (gun->ship->fuel < EPSILON)
    {
      gun->ship->target = -1;
    }
  }

  gun->timer -= dt;
  switch (gun->state)
  {
  case 0:
  {// idle
    if (gun->ship->target != -1)
    {
      gun->state = 1;
      gun->ammo_count = 3;
      return;
    }
  } break;
  case 1:
  {// fire
    if (gun->timer < 0.0f)
    {
      if (gun->ship->target == -1)
      {
        gun->timer = gun->restore_ammo_time;
        gun->state = 2;
        return;
      }
      
      if (!component_system_entity_is_enabled(gun->ship->target))
      {
        gun->timer = gun->restore_ammo_time;
        gun->state = 2;
        return;
      }

      if (!target_in_range)
      {
        gun->state = 0;
        return;
      }

      Matrix m = component_system_get_local_transform(gun->ship->entity_id);
      Vector3 from = extract_position(&m);
      m = component_system_get_local_transform(gun->ship->target);
      Vector3 to = extract_position(&m);
      gun->fire_callback(from, to);
      gun->ammo_count--;
      if (gun->ammo_count == 0)
      {
        gun->timer = gun->restore_ammo_time;
        gun->state = 2;
        return;
      }
      gun->timer = gun->frequency;
    }
  } break;
  case 2:
  {// wait
    if (gun->timer < 0.0f)
    {
      gun->state = 0;
      return;
    }
  } break;
  }
}


void
gun_system_update(f32 dt)
{
  u64 count = 0;
  t_co_gun* guns = cast_ptr(t_co_gun) component_system_get("co_gun", &count);
  for (u64 i = 0; i < count; i++)
  {
    co_gun_update(&guns[i], dt);
  }
}


#endif