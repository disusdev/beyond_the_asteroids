#include "co_animation.h"
#include "component_system.h"
#include "mesh_system.h"
#include <core/event.h>
#include <containers/cvec.h>

static void
update_animations(t_event* event)
{
  float dt = event->ctx.f32[0];

  u64 animations_count = 0;
  t_co_animation* animations = cast_ptr(t_co_animation) component_system_get("co_animation", &animations_count);
  for (u64 i = 0; i < animations_count; i++)
  {
    if (!animations[i].enabled) continue;
    animations[i].frame_time += dt * 60.0f;
    animations[i].frame_time = MIN(animations[i].frame_time, animations[i].animator[animations[i].current].frameCount);
    animations[i].frame_counter = (int)animations[i].frame_time;

    if (animations[i].frame_counter >= animations[i].animator[animations[i].current].frameCount)
    {
      if (animations[i].play_type == ANIMATION_PLAY_LOOP)
      {
        animations[i].frame_time = 0.0f;
        animations[i].frame_counter = 0;
      }
      else
      if (animations[i].play_type == ANIMATION_PLAY_ONCE)
      {
        animations[i].frame_counter = animations[i].animator[animations[i].current].frameCount - 1;
        animations[i].frame_time = animations[i].frame_counter;
      }
    }

    Model model = animations[i].model;
    ModelAnimation anim = animations[i].animator[animations[i].current];

    UpdateModelAnimation(model, anim, animations[i].frame_counter);

    // @fix Update bones, use bones as child entities
    //      should have indices map, or bone component,
    //      bone component can have index to the animation,
    //      and update position after model updates.
    //      What we can do when the entity was removed?
    //      Bones should be removed by the animation component
    //      Mb bones should live as separate entities?
  }
}


int
co_animation_create(int entity_id)
{
  int renderer_id = (u64)ctbl_get(component_system_get_entities(0)[entity_id].components, "co_renderer");

  t_co_animation animation = {0};
  animation.play_type = ANIMATION_PLAY_LOOP;
  t_co_renderer* renderers = cast_ptr(t_co_renderer) component_system_get("co_renderer", 0);
  int model_id = renderers[renderer_id].model_id;
  mesh_system_get_model_by_id(model_id, &animation.model);
  // animation.model = renderers[renderer_id].m;
  animation.animator = LoadModelAnimations(mesh_system_get_src_path(model_id),
                                           &animation.count);
  animation.enabled = false;
  if (animation.animator == 0)
  {
    return -1;
  }
  return component_system_insert(entity_id, "co_animation", &animation);
}


int
co_animation_duplicate(int ent_id, int comp_id)
{
  (void)comp_id;
  int new_comp_id = co_animation_create(ent_id);
  return new_comp_id;
}


void
animation_system_init()
{
  event_register(update_animations, APP_UPDATE);
  component_system_add_component("co_animation",
                                 sizeof(t_co_animation),
                                 1024,
                                 &co_animation_duplicate,
                                 &animation_system_create_component);
}


void
animation_system_term()
{
  // @fix add possibility to unregister event function
  // event_unregister(update_animations, APP_UPDATE);
}


int
animation_system_create_component(int ent_id, int cfg_id)
{
  (void)cfg_id;
  return co_animation_create(ent_id);
}

