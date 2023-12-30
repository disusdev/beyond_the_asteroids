#include "co_camera.h"
#include <raylib.h>
#include <raymath.h>
#include "component_system.h"


static cvec(t_co_camera_cfg) cfgs = 0;
static int current = 0;


Matrix
get_rotation_matrix(t_co_camera* camera)
{
	Quaternion rot = QuaternionFromEuler(camera->pitch, camera->yaw, 0.0f);
	return QuaternionToMatrix(rot);
}


void
set_camera_runtime(t_co_camera* camera, Matrix transform)
{
  camera->position = (Vector3){ transform.m12, transform.m13, transform.m14 };
  camera->pitch = asin(-transform.m9);
  camera->yaw = atan2(transform.m8, transform.m10);
}


void
co_camera_update(int asset_pick_camera,
                 t_co_camera* camera)
{
  if (asset_pick_camera)
  {
    UpdateCameraPro(&camera->c,
                    (Vector3) {
                      0.0f,
                      GetMouseDelta().x*0.05f,
                      -GetMouseDelta().y*0.05f
                    },
                    (Vector3) {
                      0.0f,
                      0.0f,
                      0.0f
                    },
                    -GetMouseWheelMove()*1.0f);
  }
  else
  {
    UpdateCamera(&camera->c, camera->m);
  }
}


Matrix
co_camera_get_transform(t_co_camera* camera)
{
  return MatrixInvert(GetCameraMatrix(camera->c));
}


void
co_camera_set_transform(t_co_camera* camera, Matrix t)
{
  Vector3 pos = (Vector3){ t.m12, t.m13, t.m14 };
  Vector3 up = (Vector3){ t.m4, t.m5, t.m6 };
  Vector3 target = Vector3Add(pos, (Vector3){ t.m8, t.m9, t.m10 });
  
  camera->c.position = pos;
  camera->c.up = up;
  camera->c.target = target;
}


void
co_camera_set(t_co_camera* camera, Camera c, int mode)
{
  camera->c = c;
  camera->m = mode;
}


int
co_camera_create(int entity_id, Matrix transform, int mode)
{
  (void)entity_id;
  t_co_camera camera = {0};
  co_camera_set_transform(&camera, transform);
  camera.m = mode;
  return component_system_insert(entity_id, "co_camera", &camera);
}


int
co_camera_create_by_cfg(int entity_id, int cfg_id)
{
  t_co_camera_cfg cfg = camera_system_get_cfg(cfg_id);
  t_co_camera camera = {0};
  set_camera_runtime(&camera, component_system_get_local_transform(entity_id));
  camera.entity_id = entity_id;
  camera.c.fovy = cfg.fov;
  camera.c.projection = cfg.projection;
  camera.m = cfg.mode;
  return component_system_insert(entity_id, "co_camera", &camera);
}


int
co_camera_duplicate(int ent_id, int comp_id)
{
  t_co_camera* cameras = component_system_get("co_camera", 0);
  int new_comp_id = co_camera_create(ent_id, component_system_get_global_transform(ent_id), cameras[comp_id].m);
  return new_comp_id;
}


int
camera_system_add_cfg(t_co_camera_cfg cfg)
{
  int id = cvec_header(cfgs)->size;
  cfgs = cvec_push(cfgs, cast_ptr(u8) & cfg);
  return id;
}


t_co_camera_cfg
camera_system_get_cfg(int id)
{
  return cfgs[id];
}


void
camera_system_init()
{
  cfgs = cvec_create(sizeof(t_co_camera_cfg), 16, false);
  component_system_add_component("co_camera", sizeof(t_co_camera), 1024, &co_camera_duplicate, &camera_system_create_component);
}


int
camera_system_create_component(int ent_id, int cfg_id)
{
  if (cfg_id == -1)
  {
    return -1;
  }
  ctbl_insert(component_system_get_entities(0)[ent_id].components_cfgs, "co_camera", (void*)(u64)cfg_id);
  return co_camera_create_by_cfg(ent_id, cfg_id);
}


void
co_camera_input_handle(t_co_camera* camera, f32 dt)
{
  if (camera->m == 1)
  {
	  camera->pitch = 90 * DEG2RAD;
    // camera->yaw = 90 * DEG2RAD;

    Vector3 right_vec = (Vector3) {1, 0, 0};
    Vector3 up_vec = (Vector3) {0, 1, 0};

	  Quaternion q_pitch = QuaternionFromAxisAngle(right_vec, camera->pitch);
    Quaternion q_yaw = QuaternionFromAxisAngle(up_vec, camera->yaw);

    Quaternion orientation = QuaternionMultiply(q_yaw, q_pitch);

    Matrix rotate = QuaternionToMatrix(orientation);

	  camera->forward = (Vector3) { rotate.m8, rotate.m9, rotate.m10 };
	  camera->right = (Vector3) { -rotate.m0, -rotate.m1, -rotate.m2 };
	  camera->up = (Vector3) { rotate.m4, rotate.m5, rotate.m6 };
  }
  else
  {
    Vector2 mouse_delta = GetMouseDelta();

	  camera->yaw -=  mouse_delta.x * DEG2RAD * 0.06f;
	  camera->pitch += mouse_delta.y * DEG2RAD * 0.04f;

	  camera->pitch = CLAMP(camera->pitch, -90 * DEG2RAD, 90 * DEG2RAD);

    Vector3 right_vec = (Vector3) {1, 0, 0};
    Vector3 up_vec = (Vector3) {0, 1, 0};

	  Quaternion q_pitch = QuaternionFromAxisAngle(right_vec, camera->pitch);
	  Quaternion q_yaw = QuaternionFromAxisAngle(up_vec, camera->yaw);

	  Quaternion orientation = QuaternionMultiply(q_yaw, q_pitch);

    Matrix rotate = QuaternionToMatrix(orientation);

    Matrix yaw_rotate = QuaternionToMatrix(q_yaw);
    camera->yaw_forward = (Vector3) { yaw_rotate.m8, yaw_rotate.m9, yaw_rotate.m10 };

	  camera->forward = (Vector3) { rotate.m8, rotate.m9, rotate.m10 };
	  camera->right = (Vector3) { -rotate.m0, -rotate.m1, -rotate.m2 };
	  camera->up = (Vector3) { rotate.m4, rotate.m5, rotate.m6 };
  }

  if (camera->m == 0)
  {
    f32 cam_vel = 2.0f + IsKeyDown(KEY_LEFT_SHIFT) * 20.0f;

	  Vector3 inputAxis = (Vector3)
	  {
	  	(f32)IsKeyDown(KEY_D) - (f32)IsKeyDown(KEY_A),
	  	(f32)IsKeyDown(KEY_E) - (f32)IsKeyDown(KEY_Q),
	  	(f32)IsKeyDown(KEY_W) - (f32)IsKeyDown(KEY_S)
	  };

	  Vector3 velocity = Vector3Add(Vector3Add(Vector3Scale(camera->forward, inputAxis.z),
                                             Vector3Scale(camera->right, inputAxis.x)),
                                             Vector3Scale(camera->up, inputAxis.y));

	  camera->position = Vector3Add(camera->position, Vector3Scale(velocity, cam_vel * dt));
  }
  else
  if (camera->m == 1)
  {
    // top down camera
    // camera->position = (Vector3) {0, 0, 0};
  }
  else
  {
    camera->position = (Vector3) {0, 1.7f, 0};
  }
}


void
camera_system_update(f32 dt)
{
  u64 cameras_count = 0;
  t_co_camera* cameras = component_system_get("co_camera", &cameras_count);

  for (u64 i = 0; i < cameras_count; i++)
  {
    co_camera_input_handle(&cameras[i], dt);

    Matrix transform = component_system_get_local_transform(cameras[i].entity_id);
    Matrix rot = get_rotation_matrix(&cameras[i]);
    if (cameras[i].m != 1)
    {
      transform = MatrixMultiply(rot, MatrixTranslate(cameras[i].position.x, cameras[i].position.y, cameras[i].position.z));
    }
    else
    {
      Vector3 pos = extract_position(&transform);
      transform = rot;
      update_position(&transform, pos);
    }
    component_system_set_local_transform(cameras[i].entity_id, transform);
    component_system_update_global_transform(cameras[i].entity_id);
  }
}


Camera
camera_system_get_camera()
{
  t_co_camera* cameras = component_system_get("co_camera", 0);
  co_camera_set_transform(&cameras[current], component_system_get_global_transform(cameras[current].entity_id));
  return cameras[current].c;
}


void
camera_system_switch(int camera_id)
{
  current = camera_id;
}

