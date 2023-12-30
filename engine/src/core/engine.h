#ifndef __ENGINE_H__
#define __ENGINE_H__

#include <defines.h>

#include "app.h"

#include <containers/cvec.h>

#include <systems/component_system.h>

typedef struct
t_engine
{
  t_app* app;
  b8 is_running;
}
t_engine;

void
engine_create(t_app* app);

b8
engine_run();

// void
// engine_push_bone(t_anim_bone* anim_bone);

#endif