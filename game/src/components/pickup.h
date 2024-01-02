#ifndef __PICKUP_H__
#define __PICKUP_H__

#include <defines.h>
#include <systems/component_system.h>
#include "collision.h"
#include "ship.h"
#include "health.h"


typedef struct
{
  i32 entity_id;

  i32 fuel;
  i32 hp;

  f32 angle;
} t_co_pickup;


static void
pickup_collide(t_co_collision* pickup_coll, t_co_collision* other_coll)
{
  if (other_coll->layer == 1)
  {
    t_co_pickup* pickup = cast_ptr(t_co_pickup) component_system_entity_get_component(pickup_coll->entity_id, "co_pickup");
    t_co_ship* ship = cast_ptr(t_co_ship) component_system_entity_get_component(other_coll->entity_id, "co_ship");
    t_co_health* ship_hp = cast_ptr(t_co_health) component_system_entity_get_component(other_coll->entity_id, "co_health");
    ship->fuel += pickup->fuel;
    ship_hp->health += pickup->hp;
    component_system_push(pickup_coll->entity_id);
    play_once(SFX_SHIP_PICK_UP);
  }
}

static i32
pickup_create(i32 entity_id, i32 cfg_id)
{
  t_co_pickup pickup = {0};
  pickup.entity_id = entity_id;
  pickup.fuel = 0;
  pickup.hp = 0;

  t_co_collision* coll = cast_ptr(t_co_collision) component_system_create_component_ptr(entity_id, "co_collision");
  coll->layer = 5;
  coll->radius = 1;
  coll->collide_callback = &pickup_collide;

  component_system_create_component_by_cfg(entity_id, "co_renderer", 4);

  return component_system_insert(entity_id, "co_pickup", &pickup);
}


void
pickup_system_init()
{
  component_system_add_component("co_pickup", sizeof(t_co_pickup), 16, 0, &pickup_create);
}


void
pickup_system_update(f32 dt)
{
  u64 count = 0;
  t_co_pickup* pickups = component_system_get("co_pickup", &count);
  for (u64 i = 0; i < count; i++)
  {
    pickups[i].angle += pickups[i].angle < 0 ? -dt : dt;
    Quaternion tilt_rot = QuaternionFromEuler(pickups[i].angle,0,-pickups[i].angle * 0.5f);
    Matrix model_transform = component_system_get_local_transform(pickups[i].entity_id);
    Quaternion model_curr_rot = QuaternionFromMatrix(model_transform);
    Vector3 model_pos = extract_position(&model_transform);
    model_transform = QuaternionToMatrix(QuaternionSlerp(model_curr_rot, tilt_rot, dt));
    model_transform.m12 = model_pos.x;
    model_transform.m13 = model_pos.y;
    model_transform.m14 = model_pos.z;
    component_system_set_local_transform(pickups[i].entity_id, model_transform);
  }
  
}


#endif