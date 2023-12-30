
#include <defines.h>
#include <systems/component_system.h>


typedef struct
{
  int (*update)();
} t_state;


typedef struct
{
  i32 current_state;
  cvec(t_state) states;
} t_state_machine;


typedef struct
{
  int entity_id;
  int camera_id;
  f32 speed;
  f32 friction;
  f32 mass;
  Vector3 velocity;
  Vector3 gravity;
  Ray ray;
} t_co_movement;


int
co_movement_create(int entity_id, int cfg_id);


void
co_movement_init()
{
  component_system_add_component("co_movement", sizeof(t_co_movement), 1, 0, &co_movement_create);
}


void
co_movement_term()
{
  
}


int
co_movement_create(int entity_id, int cfg_id)
{
  t_co_movement movement = {0};
  movement.speed = 5.0f;
  movement.friction = 0.2f;
  movement.mass = 4.0f;
  movement.entity_id = entity_id;
  movement.gravity = (Vector3){0, -9.8f, 0};

  return component_system_insert(entity_id, "co_movement", &movement);
}


Quaternion get_rotation_between(Vector3 u, Vector3 v)
{
  float k_cos_theta = Vector3DotProduct(u, v);
  float k = sqrtf(Vector3LengthSqr(u) * Vector3LengthSqr(v));

  if (k_cos_theta / k == -1)
  {
    // 180 degree rotation around any orthogonal vector
    Vector3OrthoNormalize(&u, &v);
    return (Quaternion){u.x, u.y, u.z, 0.0f};
  }

  Vector3 cross_uv = Vector3CrossProduct(u, v);
  return QuaternionNormalize((Quaternion){cross_uv.x, cross_uv.y, cross_uv.z, k_cos_theta + k});
}


void
co_movement_update(f32 dt)
{
  u64 count = 0;
  t_co_movement* movements = cast_ptr(t_co_movement) component_system_get("co_movement", &count);
  t_co_camera* cameras = cast_ptr(t_co_camera) component_system_get("co_camera", 0);

  for (u64 i = 0; i < count; i++)
  {
    Vector2 inputAxis = (Vector2)
	  {
	  	(f32)IsKeyDown(KEY_D) - (f32)IsKeyDown(KEY_A),
	  	(f32)IsKeyDown(KEY_W) - (f32)IsKeyDown(KEY_S)
	  };

    Matrix transform = component_system_get_local_transform(movements[i].entity_id);
    Vector3 position = extract_position(&transform);

    // Matrix transform = component_system_get_local_transform(movements[i].entity_id);
    Vector3 planet_position = Vector3Zero();
    f32 planet_radius = 30;

    Quaternion rot = QuaternionFromMatrix(transform);
    Vector3 move_vec = Vector3Add(Vector3Scale(Vector3RotateByQuaternion(cameras[movements[i].camera_id].yaw_forward, rot), inputAxis.y),
                                  Vector3Scale(Vector3RotateByQuaternion(cameras[movements[i].camera_id].right, rot), inputAxis.x));
    move_vec = Vector3Normalize(move_vec);


    movements[i].ray = (Ray) { position, Vector3RotateByQuaternion(cameras[movements[i].camera_id].yaw_forward, rot) };

    Vector3 desired_move = Vector3Scale(move_vec, movements[i].speed * dt);
    
    Vector3 desired_position = Vector3Add(position, desired_move);

    Vector3 surface_desired_position = Vector3Scale(Vector3Normalize(desired_position), planet_radius);

    desired_move = Vector3Subtract(surface_desired_position, position);

    Vector3 old_right = (Vector3) { transform.m0, transform.m1, transform.m2 };
    Vector3 old_up = (Vector3) { transform.m4, transform.m5, transform.m6 };
    Vector3 old_forward = (Vector3) { transform.m8, transform.m9, transform.m10 };

    Vector3 new_up = Vector3Normalize(surface_desired_position);

    Quaternion resultQuat = QuaternionFromVector3ToVector3(old_up, new_up);

    transform = MatrixMultiply(transform, QuaternionToMatrix(resultQuat));


    transform.m12 = surface_desired_position.x;
    transform.m13 = surface_desired_position.y;
    transform.m14 = surface_desired_position.z;

    component_system_set_local_transform(movements[i].entity_id, transform);
    component_system_update_global_transform(movements[i].entity_id);
  }
  
}