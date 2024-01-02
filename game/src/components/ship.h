#ifndef __SHIP_H__
#define __SHIP_H__

#include <defines.h>
#include <systems/component_system.h>
#include "asteroid.h"

typedef struct co_ship
{
  int entity_id;
  int camera_id;

  int model_id;

  f32 speed;
  f32 rotation_speed;
  f32 tilt;
  f32 speedup_time;
  Vector3 move_pos;
  b32 is_slowdown;
  b32 in_move;
  f32 time;
  f32 slow_time;

  Vector3 velocity;
  f32 angle;

  b32 reach_destination;

  i32 target;
  b32 move_to_target;

  t_co_health* health;
  f32 fuel;

  void (*on_death)(struct co_ship*);

  cvec(Vector3) points;
} t_co_ship;


void
on_death(t_co_ship* ship)
{
  stop_music(MUSIC_SHIP_LOW_FUEL);
  ship->target = -1;
}


int
co_ship_create_from_cfg(int entity_id, int cfg_id)
{
  t_co_ship ship = {0};
  ship.speed = 3.0f;
  ship.tilt = 0.5f;
  ship.rotation_speed = 2.0f;
  ship.entity_id = entity_id;
  ship.speedup_time = 1.0f;
  ship.is_slowdown = false;
  ship.slow_time = ship.speedup_time;
  ship.in_move = false;
  ship.reach_destination = true;
  ship.points = cvec_create(sizeof(Vector3), 128, false);
  ship.target = -1;
  ship.fuel = 100.0f;
  ship.on_death = &on_death;

  int model_id = component_system_create_entity(entity_id);
  component_system_create_component_by_cfg(model_id, "co_renderer", 3);
  ship.model_id = model_id;

  return component_system_insert(entity_id, "co_ship", &ship);
}


void
ship_system_init()
{
  component_system_add_component("co_ship", sizeof(t_co_ship), 4, 0, &co_ship_create_from_cfg);
}


void
check_for_target(t_co_ship* ship)
{
  Camera cam = camera_system_get_camera();
  Vector2 mouse_pos = GetMousePosition();
  Ray ray = GetMouseRay(mouse_pos, cam);

  u64 count = 0;
  t_co_collision* colls = component_system_get("co_collision", &count);

  for (u64 i = 0; i < count; i++)
  {
    if (colls[i].layer == 1) continue;
    Matrix m = component_system_get_global_transform(colls[i].entity_id);
    Vector3 forward = (Vector3) {m.m8, m.m9, m.m10};
    forward = Vector3Scale(forward, colls[i].offset.z);
    Vector3 right = (Vector3) {m.m0, m.m1, m.m2};
    right = Vector3Scale(right, colls[i].offset.x);
    Vector3 center = Vector3Add(Vector3Add(extract_position(&m), forward), right);
    RayCollision coll = GetRayCollisionSphere(ray, center, colls[i].radius);
    if (coll.hit)
    {
      ship->target = colls[i].entity_id;
      ship->move_to_target = true;
      return;
    }
  }

  ship->move_to_target = false;
}


f32 tilter = 0;
void
ship_system_tilt_update(f32 dt)
{
  u64 ships_count = 0;
  t_co_ship* ships = cast_ptr(t_co_ship) component_system_get("co_ship", &ships_count);

  if (ships_count == 0) return;

  t_co_ship* ship = &ships[0];

  {
    tilter += dt;
    f32 angle_diff = sinf(tilter)*(10*DEG2RAD);
    Quaternion tilt_rot = QuaternionFromEuler(0,0,angle_diff);
    Matrix model_transform = component_system_get_local_transform(ship->model_id);
    Quaternion model_curr_rot = QuaternionFromMatrix(model_transform);
    Vector3 model_pos = extract_position(&model_transform);
    model_transform = QuaternionToMatrix(QuaternionSlerp(model_curr_rot, tilt_rot, dt));
    model_transform.m12 = model_pos.x;
    model_transform.m13 = model_pos.y;
    model_transform.m14 = model_pos.z;
    component_system_set_local_transform(ship->model_id, model_transform);
  }
}


void
ship_system_update(f32 dt, void (*end)())
{
  Camera camera = camera_system_get_camera();
  u64 ships_count = 0;
  t_co_ship* ships = cast_ptr(t_co_ship) component_system_get("co_ship", &ships_count);

  if (ships_count == 0) return;

  t_co_ship* ship = &ships[0];

  b32 out_of_fuel = false;
  if (ship->fuel < EPSILON)
  {
    out_of_fuel = true;
  }

  t_entity* entities = component_system_get_entities(0);
  if (!entities[ship->entity_id].enabled) return;

  if (out_of_fuel)
  {
    Matrix transform = component_system_get_local_transform(ship->entity_id);
    ship->move_pos = extract_position(&transform);
    if (cvec_header(ship->points)->size > 0)
    {
      cvec_clear(ship->points);
    }
  }
  else
  if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))//IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
  {
    check_for_target(ship);

    Vector2 mouse_pos = GetMousePosition();
    Ray ray = GetMouseRay(mouse_pos, camera);
    Plane plane = (Plane) {Vector3Zero(), (Vector3){0,-1,0}};
    RayCollision coll = IntersectRayPlane(ray, plane);
    Matrix transform = component_system_get_local_transform(ship->entity_id);
    f32 dst = Vector3Distance(extract_position(&transform), coll.point);
    if (coll.hit && dst < 5.0f)
    {
      coll.hit = false;
    }

    if (coll.hit)
    {
      ship->reach_destination = false;
      ship->move_pos = coll.point;
      ship->is_slowdown = false;
      // ship->slow_time = ship->speedup_time;
      if (!ship->in_move)
      {
        ship->time = 0;
      }
    }

#if 1
    if (coll.hit)
    {// predict until reach the point
      cvec_clear(ship->points);
      // Matrix transform = component_system_get_local_transform(ship->entity_id);
      // Vector3 pvel = Vector3Zero();
      // f32 pdt = 0.1f;
      // f32 dst = 0.0f;
      // f32 time = 0;
      // f32 real_speed = 0;
      // b32 is_slowdown = false;
      // f32 slow_time = ship->speedup_time;
      // do
      // {
      //   Vector3 forward = (Vector3) { transform.m8, transform.m9, transform.m10 };
      //   Vector3 pos = extract_position(&transform);
        
      //   f32 pdst = Vector3Distance(pos, ship->move_pos);
      //   f32 time_to_end = (pdst / ship->speed) * 2;
      //   time += pdt;

      //   if (time_to_end < ship->speedup_time && !is_slowdown)
      //   {
      //     is_slowdown = true;
      //   }
      //   else
      //   if (time < ship->speedup_time && !is_slowdown)
      //   {
      //     real_speed = Lerp(0, ship->speed, time / ship->speedup_time);
      //   }

      //   // if (is_slowdown)
      //   // {
      //   //   slow_time -= pdt;
      //   //   real_speed = Lerp(0.1, ship->speed, slow_time);
      //   // }
        
      //   real_speed = ship->speed;// CLAMP(real_speed, 0.1, ship->speed);

      //   pvel = Vector3Scale(forward, real_speed);

      //   Vector3 dir = Vector3Normalize(Vector3Subtract(ship->move_pos, pos));
      //   f32 angle = Vector2Angle((Vector2){0, 1}, (Vector2){dir.x, dir.z});

      //   f32 angle_diff = Vector2Angle((Vector2){forward.x, forward.z}, (Vector2){dir.x, dir.z});

      //   Quaternion curr_rot = QuaternionFromMatrix(transform);
      //   Quaternion rot = QuaternionFromEuler(0,angle,0);
      //   Matrix trans = QuaternionToMatrix(QuaternionSlerp(curr_rot, rot, pdt * ship->rotation_speed));
      //   Matrix act_rot = MatrixRotateY(angle_diff * pdt * ship->rotation_speed);
      //   forward = (Vector3) { transform.m8, transform.m9, transform.m10 };

      //   {
      //     f32 angle_diff = Vector2Angle((Vector2){forward.x, forward.z}, (Vector2){dir.x, dir.z});
      //     transform = MatrixMultiply(transform, act_rot);// QuaternionToMatrix(QuaternionSlerp(curr_rot, rot, pdt * ship->rotation_speed * cosf(angle_diff / 2)));
      //   }
        
      //   pos.x += pvel.x * pdt;
      //   pos.y += pvel.y * pdt;
      //   pos.z += pvel.z * pdt;

      //   transform.m12 = pos.x;
      //   transform.m13 = pos.y;
      //   transform.m14 = pos.z;

      //   ship->points = cvec_push(ship->points, &pos);

      //   dst = Vector3Distance(pos, ship->move_pos);
      // } while(dst > 1.2f);
      ship->points = cvec_push(ship->points, &ship->move_pos);
    }
#endif
  }

  // if (!in_move) return;

  Matrix transform = component_system_get_local_transform(ship->entity_id);
  Vector3 pos = extract_position(&transform);

  // 1. we should consider the target speed, and move with the same speed
  // 2. we should consider the target speed is equal, and stop close to target (probably can be solved by stopping distance)
  // ship->move_to_target

  f32 real_speed = ship->speed;

  if (ship->move_to_target && ship->target != -1)
  {
    Matrix m_target = component_system_get_global_transform(ship->target);
    ship->move_pos = extract_position(&m_target);

    t_co_asteroid* asteroids = component_system_get("co_asteroid", 0);

    f32 dst = Vector3Distance(pos, ship->move_pos);
    f32 target_speed = asteroids[ship->target].speed;

    if (dst < 10.0f && ship->speed > target_speed)
    {
      real_speed = target_speed;
    }
    else
    if (ship->angle < (5 * DEG2RAD) && ship->angle > (-5 * DEG2RAD))
    {
      real_speed = real_speed * 1.5f;
    }
  }

  f32 pdst = Vector3Distance(pos, ship->move_pos);
  f32 time_to_end = (pdst / ship->speed) * 2;
  ship->time += dt;


  if (time_to_end < ship->speedup_time && !ship->is_slowdown)
  {
    ship->is_slowdown = true;
  }
  else
  if (ship->time < ship->speedup_time && !ship->is_slowdown)
  {
    real_speed = Lerp(0, ship->speed, ship->time / ship->speedup_time);
  }

  // if (ship->is_slowdown)
  // {
  //   ship->slow_time -= dt;
  //   real_speed = Lerp(0.1, ship->speed, ship->slow_time);
  // }

  b32 prev_move = ship->in_move;

  ship->in_move = Vector3Distance(pos, ship->move_pos) > 1.2f;

  if (ship->in_move && !prev_move)
  {
    play_once(SFX_SHIP_LUNCH);
    play_music(MUSIC_SHIP_MOVE);
  }

  if (!ship->in_move && prev_move)
  {
    stop_music(MUSIC_SHIP_MOVE);
  }

  if (out_of_fuel)
  {
    {
      ship->angle += ship->angle < 0 ? -dt : dt;
      Quaternion tilt_rot = QuaternionFromEuler(0,ship->angle,0);
      Matrix model_transform = component_system_get_local_transform(ship->model_id);
      Quaternion model_curr_rot = QuaternionFromMatrix(model_transform);
      Vector3 model_pos = extract_position(&model_transform);
      model_transform = QuaternionToMatrix(QuaternionSlerp(model_curr_rot, tilt_rot, dt));
      model_transform.m12 = model_pos.x;
      model_transform.m13 = model_pos.y;
      model_transform.m14 = model_pos.z;
      
      component_system_set_local_transform(ship->model_id, model_transform);

      pos.x += ship->velocity.x * dt;
      pos.y += ship->velocity.y * dt;
      pos.z += ship->velocity.z * dt;

      ship->velocity = Vector3Add(ship->velocity, Vector3Scale(Vector3Normalize(ship->velocity), dt));

      transform.m12 = pos.x;
      transform.m13 = pos.y;
      transform.m14 = pos.z;

      component_system_set_local_transform(ship->entity_id, transform);
    }

    return;
  }

  if (ship->in_move)
  {
    Vector3 dir = Vector3Normalize(Vector3Subtract(ship->move_pos, pos));
    f32 angle = Vector2Angle((Vector2){0, 1}, (Vector2){dir.x, dir.z});

    Quaternion curr_rot = QuaternionFromMatrix(transform);
    Quaternion rot = QuaternionFromEuler(0,angle,0);

    Matrix trans = QuaternionToMatrix(QuaternionSlerp(curr_rot, rot, dt * ship->rotation_speed));
    Vector3 forward = (Vector3) { transform.m8, transform.m9, transform.m10 };

    {
      f32 angle_diff = Vector2Angle((Vector2){forward.x, forward.z}, (Vector2){dir.x, dir.z});
      transform = QuaternionToMatrix(QuaternionSlerp(curr_rot, rot, dt * ship->rotation_speed * cosf(angle_diff / 2)));
    }

    forward = (Vector3) { transform.m8, transform.m9, transform.m10 };
    ship->velocity = Vector3Scale(forward, real_speed);

    {
      f32 angle_diff = Vector2Angle((Vector2){forward.x, forward.z}, (Vector2){dir.x, dir.z});
      angle_diff = CLAMP(angle_diff, -(PI / 2), PI / 2);
      ship->angle = angle_diff;
      Quaternion tilt_rot = QuaternionFromEuler(0,0,-angle_diff * ship->tilt);
      Matrix model_transform = component_system_get_local_transform(ship->model_id);
      Quaternion model_curr_rot = QuaternionFromMatrix(model_transform);
      Vector3 model_pos = extract_position(&model_transform);
      model_transform = QuaternionToMatrix(QuaternionSlerp(model_curr_rot, tilt_rot, dt));
      model_transform.m12 = model_pos.x;
      model_transform.m13 = model_pos.y;
      model_transform.m14 = model_pos.z;
      component_system_set_local_transform(ship->model_id, model_transform);
    }

    pos.x += ship->velocity.x * dt;
    pos.y += ship->velocity.y * dt;
    pos.z += ship->velocity.z * dt;

    transform.m12 = pos.x;
    transform.m13 = pos.y;
    transform.m14 = pos.z;

    component_system_set_local_transform(ship->entity_id, transform);

    ship->fuel -= real_speed * dt;
    if (ship->fuel < 0)
    {
      ship->fuel = 0;
      if (!ship->reach_destination)
      {
        // call end move callback
        end();
        play_music(MUSIC_SHIP_LOW_FUEL);
      }
      ship->reach_destination = true;
    }
  }
  else
  {
    if (!ship->reach_destination)
    {
      // call end move callback
      end();
    }
    ship->reach_destination = true;
  }
}



#endif