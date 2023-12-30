#ifndef __SMOOTH_MOVER_H__
#define __SMOOTH_MOVER_H__

#include <defines.h>
#include <systems/component_system.h>

typedef struct
{
  int entity_id;
  int target_id;
  t_co_camera* camera;
  f32 speed;
  f32 height;
} t_smooth_mover;


int
co_smooth_mover_create_from_cfg(int entity_id, int cfg)
{
  t_smooth_mover mover = {0};
  mover.entity_id = entity_id;
  mover.speed = 2.0f;
  mover.height = 60.0f;
  return component_system_insert(entity_id, "co_smooth_mover", &mover);
}


void
smooth_mover_system_init()
{
  component_system_add_component("co_smooth_mover", sizeof(t_smooth_mover), 4, 0, &co_smooth_mover_create_from_cfg);
}


void
smooth_mover_system_update(f32 dt)
{
  u64 count = 0;
  t_smooth_mover* mover = cast_ptr(t_smooth_mover) component_system_get("co_smooth_mover", &count);

  if (count == 0) return;

  Matrix transform = component_system_get_local_transform(mover->target_id);
  Vector3 pos = extract_position(&transform);

  Matrix mover_transform = component_system_get_local_transform(mover->entity_id);
  pos.y = mover->height;
  Vector3 mover_pos = extract_position(&mover_transform);
  pos = Vector3Lerp(mover_pos, pos, dt * mover->speed);
  mover_transform.m12 = pos.x;
  mover_transform.m13 = pos.y;
  mover_transform.m14 = pos.z;
  component_system_set_local_transform(mover->entity_id, mover_transform);

  mover->camera->yaw = Lerp(mover->camera->yaw, 0, dt * mover->speed);
}



#endif