#ifndef CO_CAMERA_H
#define CO_CAMERA_H

#include <defines.h>
#include <raylib.h>
#include <raymath.h>

typedef struct
{
  Camera c;
  int m;
  int entity_id;

  // runtime
  Vector3 position;
  f32 pitch;
  f32 yaw;

  Vector3 up;
  Vector3 right;
  Vector3 forward;

  Vector3 yaw_forward;
} t_co_camera;


typedef struct
{
  int id;
  float fov;
  int projection;
  int mode;
} t_co_camera_cfg;


void
co_camera_update(int asset_pick_camera,
                 t_co_camera* camera);


Matrix
co_camera_get_transform(t_co_camera* camera);


void
co_camera_set_transform(t_co_camera* camera,
                        Matrix t);


void
co_camera_set(t_co_camera* camera,
              Camera c,
              int mode);


int
co_camera_duplicate(int ent_id, int comp_id);


void
camera_system_init();


int
co_camera_create_by_cfg(int entity_id, int cfg_id);


int
camera_system_create_component(int ent_id, int cfg_id);


void
camera_system_update(f32 dt);


int
camera_system_add_cfg(t_co_camera_cfg cfg);


t_co_camera_cfg
camera_system_get_cfg(int id);


Camera
camera_system_get_camera();


void
camera_system_switch(int camera_id);


#endif