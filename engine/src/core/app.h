#ifndef __APP_H__
#define __APP_H__

#include <defines.h>

typedef struct
t_app
{
  const char* name;
  void (*init)();
  void (*update)(f32);
  void (*late_update)(f32);
  void (*pre_render)(f32);
  void (*fixed_update)(f32);
  void (*render)(f32);
  void (*draw_ui)();
  void (*term)();
  void (*draw_gizmo)(f32);
  i32 window_pos_x;
  i32 window_pos_y;
  i32 window_size_x;
  i32 window_size_y;
}
t_app;

#endif
