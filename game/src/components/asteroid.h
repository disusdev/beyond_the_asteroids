#ifndef __ASTEROID_H__
#define __ASTEROID_H__

#include <defines.h>
#include <systems/component_system.h>

typedef struct
{
  int entity_id;
  int camera_id;

  int model_id;

  f32 speed;
  f32 rotation_speed;
  f32 tilt;

  Vector3 velocity;
  f32 angular_velocity;
  f32 current_angle;

  cvec(Vector3) points;
} t_co_asteroid;


int
co_asteroid_create_from_cfg(int entity_id, int cfg_id)
{
  t_co_asteroid asteroid = {0};
  asteroid.speed = GetRandomValue(1, 5);
  asteroid.tilt = GetRandomValue(-180,180) * DEG2RAD;
  asteroid.rotation_speed = 0.05f;
  asteroid.entity_id = entity_id;
  asteroid.points = cvec_create(sizeof(Vector3), 128, false);

  Vector3 dir = (Vector3) { GetRandomValue(0,180)/180.0f, 0, GetRandomValue(0, 180)/180.0f };
  Vector3 vel = Vector3Scale(Vector3Normalize(dir), asteroid.speed);
  asteroid.velocity = vel;

  asteroid.current_angle = GetRandomValue(-180,180) * DEG2RAD;

  asteroid.angular_velocity = GetRandomValue(-3, 3);

  return component_system_insert(entity_id, "co_asteroid", &asteroid);
}


void
asteroid_system_init()
{
  component_system_add_component("co_asteroid", sizeof(t_co_asteroid), 1, 0, &co_asteroid_create_from_cfg);
}


void
asteroid_system_update(f32 dt, void (*respawn)(int))
{
  Camera camera = camera_system_get_camera();
  u64 asteroids_count = 0;
  t_co_asteroid* asteroids = cast_ptr(t_co_asteroid) component_system_get("co_asteroid", &asteroids_count);

  for (u64 i = 0; i < asteroids_count; i++)
  {
    t_co_asteroid* asteroid = &asteroids[i];
    if (!component_system_get_entities(0)[asteroid->entity_id].enabled) continue;
#if 0
    {// predict until reach the point
      cvec_clear(asteroid->points);
      Matrix transform = component_system_get_local_transform(asteroid->entity_id);
      // Vector3 pvel = asteroid->velocity;
      f32 pavel = asteroid->angular_velocity * asteroid->rotation_speed;
      f32 pdt = 0.2f;
      f32 dst = 0.0f;
      f32 current_angle = 0;
      for (int step = 0; step < 100; step++)
      {
        Vector3 pos = extract_position(&transform);
        Vector3 euler_rot = QuaternionToEuler(QuaternionFromMatrix(transform));
        current_angle += pavel * pdt;

        Quaternion curr_rot = QuaternionFromMatrix(transform);
        Quaternion rot = QuaternionFromEuler(0,current_angle,0);
        transform = QuaternionToMatrix(rot);// QuaternionToMatrix(QuaternionSlerp(curr_rot, rot, pdt * asteroid->rotation_speed));
        Vector3 forward = (Vector3) { transform.m8, transform.m9, transform.m10 };

        pos.x += forward.x * pdt * asteroid->speed;
        pos.y += forward.y * pdt * asteroid->speed;
        pos.z += forward.z * pdt * asteroid->speed;

        transform.m12 = pos.x;
        transform.m13 = pos.y;
        transform.m14 = pos.z;

        asteroid->points = cvec_push(asteroid->points, &pos);
      }
    }
#endif

    Matrix transform = component_system_get_local_transform(asteroid->entity_id);
    Vector3 pos = extract_position(&transform);

    camera.position.y = 1;
    f32 out_dst = Vector3Distance(camera.position, pos);
    if (out_dst > 200)
    {
      respawn(asteroid->entity_id);
      return;
    }

    Vector3 euler_rot = QuaternionToEuler(QuaternionFromMatrix(transform));
    asteroid->current_angle += asteroid->angular_velocity * asteroid->rotation_speed * dt;
    Quaternion curr_rot = QuaternionFromMatrix(transform);
    Quaternion rot = QuaternionFromEuler(0,asteroid->current_angle,0);
    Vector3 forward = (Vector3) { transform.m8, transform.m9, transform.m10 };
    transform = QuaternionToMatrix(rot);
    pos.x += forward.x * dt * asteroid->speed;
    pos.y += forward.y * dt * asteroid->speed;
    pos.z += forward.z * dt * asteroid->speed;
    transform.m12 = pos.x;
    transform.m13 = pos.y;
    transform.m14 = pos.z;
    component_system_set_local_transform(asteroid->entity_id, transform);

    {
      Matrix model_transform = component_system_get_local_transform(asteroid->model_id);
      model_transform = MatrixMultiply(model_transform, MatrixRotateXYZ((Vector3){dt * asteroid->tilt * 0.5f,0,dt * asteroid->tilt})); 
      component_system_set_local_transform(asteroid->model_id, model_transform);
    }
  }
}



#endif