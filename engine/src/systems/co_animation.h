#ifndef CO_ANIMATION_H
#define CO_ANIMATION_H

#include <defines.h>
#include <raylib.h>


typedef enum
{
  ANIMATION_PLAY_ONCE = 0,
  ANIMATION_PLAY_LOOP
} e_animation_play;


typedef struct
{
  ModelAnimation* animator;
  Model model;
  int current;
  int count;
  float frame_time;
  int frame_counter;
  e_animation_play play_type;
  bool enabled;
} t_co_animation;


int
co_animation_create(int entity_id);


int
co_animation_play(int id,
                  int animation_idx,
                  e_animation_play play_type);


int
co_animation_duplicate(int ent_id, int comp_id);


void
animation_system_init();


void
animation_system_term();


int
animation_system_create_component(int ent_id, int cfg_id);


#endif
