#include <entry.h>

#include <raylib.h>

#include <systems/component_system.h>
#include <systems/scene_system.h>
#include <systems/shaders.h>

#if defined(PLATFORM_WEB)
#include <emscripten.h>
#endif

#include <raygui.h>
#include <rlgl.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#if defined(PLATFORM_DESKTOP)
#define GLSL_VERSION 330
#else
#define GLSL_VERSION 100
#endif

static int WINDOW_WIDTH = 1920;
static int WINDOW_HEIGHT = 1080;

i32 max_score = 0;
i32 score = 0;

i32 boost_counter = 3;
i32 score_boost = 1000;

static void
create_asteroid();

void
add_score(i32 points)
{
  score += points;
  
  score_boost -= points;
  
  // @todo there is no asteroid destruction on
  //       game restart which will lead to the
  //       same ammount of the asteroids after restart.
  // if (score_boost <= 0)
  // {
  //   score_boost = 1000;
  //   for (int i = 0; i < boost_counter; i++)
  //   {
  //     create_asteroid();
  //   }
  //   boost_counter *= 3;
  // }
}

void
save_score()
{
#if defined(PLATFORM_WEB)
  // @todo save with IDBFS
#else
  max_score = MAX(score, max_score);
  FILE* file = fopen("data/score.sv", "wb+");
  fwrite(&max_score, sizeof(i32), 1, file);
  fclose(file);
#endif
}

void
read_score()
{
#if defined(PLATFORM_WEB)
  // @todo save with IDBFS
#else
  FILE* file = fopen("data/score.sv", "rb+");
  if (file)
  {
    fread(&max_score, sizeof(i32), 1, file);
    fclose(file);
  }
#endif
}

typedef enum
{
  GAME_STATE_MENU = 0,
  GAME_STATE_GAME,
  GAME_STATE_PAUSE
} e_game_state;

e_game_state state = GAME_STATE_MENU;

typedef enum
{
  MUSIC_GAME,
  MUSIC_MENU,
  MUSIC_SHIP_MOVE,
  MUSIC_SHIP_LOW_FUEL,
  MUSIC_COUNT
} e_music;


typedef enum
{
  SFX_COLLISION,
  SFX_SHIP_FIRE,
  SFX_SHIP_LUNCH,
  SFX_SHIP_LOW_HEALTH,
  SFX_SHIP_PICK_UP,
  SFX_ASTEROID_HIT,
  SFX_SHIP_STOPS,
  
  SFX_UI_BACK,
  SFX_UI_SCROLL,
  SFX_UI_SELECT,
  SFX_UI_START,

  SFX_COUNT
} e_sfx;

const char* music_path[] =
{
  "data/audio/cosmohunter_game_loop.ogg",
  "data/audio/cosmohunter_menu_loop.ogg",
  "data/audio/space_ship_move_loop.ogg",
  "data/audio/spaceship_low_fuel.ogg"
};

const char* sfx_path[] =
{
  "data/audio/space_collision.ogg",
  "data/audio/space_ship_fire.ogg",
  "data/audio/space_ship_lunch.ogg",
  "data/audio/spaceship_low_health.ogg",
  "data/audio/spaceship_pick_up.ogg",
  "data/audio/asteroid_hitted.ogg",
  "data/audio/space_ship_stops.ogg",
  
  "data/audio/ui_back.ogg",
  "data/audio/ui_scroll.ogg",
  "data/audio/ui_select.ogg",
  "data/audio/ui_start.ogg",
};

Music music_tbl[MUSIC_COUNT] = {0};
Sound sfx_tbl[SFX_COUNT] = {0};


void
play_once(e_sfx sfx)
{
  if (state == GAME_STATE_MENU) return;
  PlaySound(sfx_tbl[sfx]);
}


void
play_once_ui(e_sfx sfx)
{
  PlaySound(sfx_tbl[sfx]);
}


void
play_music(e_music music)
{
  PlayMusicStream(music_tbl[music]);
}


void
stop_music(e_music music)
{
  StopMusicStream(music_tbl[music]);
}


#include "components/text_3d.h"
#include "components/movement.h"
#include "components/smooth_mover.h"
#include "components/collision.h"
#include "components/health.h"
#include "components/ship.h"
#include "components/asteroid.h"
#include "components/gun.h"
#include "components/projectile.h"
#include "components/pickup.h"
#include "components/particle.h"

#include <containers/ctbl.h>


typedef struct
{
  Vector3 pos;
  i32 id;
  i32 frame;
} t_sprite_anim;


// static Mesh grid_mesh;
// static Shader grid_shader;
#define EXPLOSION_COUNT 4
static Texture2D explosion[EXPLOSION_COUNT];
i32 explosion_scale = 2;
i32 explosion_frame = 0;
f32 explosion_time = 0;

static b32 in_move = false;

static Texture2D background;

static cvec(t_sprite_anim) animations;

t_co_ship* ship = 0;


b32 sfx_mute = false;
b32 music_mute = false;


void
load_textures()
{
  explosion[0] = LoadTexture("data/sprites/fire/explosion_0.png");
  explosion[1] = LoadTexture("data/sprites/fire/explosion_1.png");
  explosion[2] = LoadTexture("data/sprites/fire/explosion_2.png");
  explosion[3] = LoadTexture("data/sprites/fire/explosion_3.png");
}


void
update_music()
{
  if (music_mute) return;
  for (u64 i = 0; i < MUSIC_COUNT; i++)
  {
    UpdateMusicStream(music_tbl[i]);
  }
}


static float exponent = 1.0f;
static float averageVolume[400] = { 0.0f };

void ProcessAudio(void *buffer, unsigned int frames)
{
    float *samples = (float *)buffer;
    float average = 0.0f;
    for (unsigned int frame = 0; frame < frames; frame++)
    {
        float *left = &samples[frame * 2 + 0], *right = &samples[frame * 2 + 1];

        *left = powf(fabsf(*left), exponent) * ( (*left < 0.0f)? -1.0f : 1.0f );
        *right = powf(fabsf(*right), exponent) * ( (*right < 0.0f)? -1.0f : 1.0f );

        average += fabsf(*left) / frames;
        average += fabsf(*right) / frames;
    }
    for (int i = 0; i < 399; i++) averageVolume[i] = averageVolume[i + 1];
    averageVolume[399] = average;
}


void
load_audio()
{
  InitAudioDevice();

  AttachAudioMixedProcessor(ProcessAudio);

  for (u64 i = 0; i < MUSIC_COUNT; i++)
  {
    music_tbl[i] = LoadMusicStream(music_path[i]);
  }

  music_tbl[MUSIC_GAME].looping = true;
  SetMusicVolume(music_tbl[MUSIC_GAME], 0.55f);
  music_tbl[MUSIC_SHIP_MOVE].looping = true;
  SetMusicVolume(music_tbl[MUSIC_SHIP_MOVE], 3.0f);

  for (u64 i = 0; i < SFX_COUNT; i++)
  {
    sfx_tbl[i] = LoadSound(sfx_path[i]);
  }
  SetSoundVolume(sfx_tbl[SFX_SHIP_LUNCH], 0.5f);

  play_music(MUSIC_MENU);
}


void
trigger_turn()
{
  if (!ship->reach_destination)
  {
    in_move = true;
    play_once(SFX_SHIP_LUNCH);
    play_music(MUSIC_SHIP_MOVE);
  }
}

void
stop_turn()
{
  // end on this turn
  in_move = false;
  stop_music(MUSIC_SHIP_MOVE);
}


void end_move()
{
  cvec_clear(ship->points);
  in_move = false;
  stop_music(MUSIC_SHIP_MOVE);
}


static void
create_pickup(Vector3 position)
{
  if (GetRandomValue(0, 10) > 3) return;

  int ent_id = component_system_pop("co_pickup");
  t_co_pickup* pickup = 0;
  if (ent_id == -1)
  {
    ent_id = component_system_create_entity(-1);
    pickup = cast_ptr(t_co_pickup) component_system_create_component_ptr(ent_id, "co_pickup");
    component_system_create_component_by_cfg(ent_id, "co_renderer", 4);
  }
  else
  {
    pickup = cast_ptr(t_co_pickup) component_system_entity_get_component(ent_id, "co_pickup");
  }
  
  t_co_renderer* renderer = component_system_entity_get_component(ent_id, "co_renderer");
  
  if (GetRandomValue(0, 1))
  {
    pickup->fuel = GetRandomValue(10, 30);
    co_renderer_set_model_by_cfg(renderer, 4);
  }
  else
  {
    pickup->hp = GetRandomValue(10, 30);
    co_renderer_set_model_by_cfg(renderer, 6);
  }

  component_system_set_local_transform(ent_id, MatrixTranslate(position.x, position.y, position.z));
  component_system_update_global_transform(ent_id);
}


void
add_animation(int entity_id, Vector3 pos, i32 layer)
{
  t_entity* entities = component_system_get_entities(0);
  t_sprite_anim anim = {0};
  anim.pos = pos;
  anim.frame = 0;

  if (layer == 0)
  {
    anim.id = 0;
    u64 count = 0;
    t_co_projectile* projs = component_system_get("co_projectile", &count);
    for (u64 i = 0; i < count; i++)
    {
      if (projs[i].target_id == entity_id)
      {
        projs[i].target_id = -1;
      }
    }
    if (ship->target == entity_id)
    {
      ship->target = -1;
    }
  }
  else
  if (layer == 1)
  {
    anim.id = 2;
  }

  Vector2 screen_pos = GetWorldToScreen(pos, camera_system_get_camera());

  if (screen_pos.x < 0 || screen_pos.x > GetScreenWidth() ||
      screen_pos.y < 0 || screen_pos.y > GetScreenHeight())
  {
    return;
  }

  animations = cvec_push(animations, &anim);

  if (layer == 0)
  {
    play_once(SFX_COLLISION);
    create_pickup(pos);
  }
  else
  if (layer == 1)
  {
    play_once(SFX_SHIP_LOW_HEALTH);
    SetMusicVolume(music_tbl[MUSIC_GAME], 0.0f);
    ship->on_death(&ship);
  }
}


void
create_asteroid()
{
  // get pos outside the screen!
  Vector3 cam_pos = camera_system_get_camera().position;

  f32 hsize = 50;

  BoundingBox spawn_bbox = {0};
  spawn_bbox.min.x = cam_pos.x - hsize;
  spawn_bbox.min.z = cam_pos.z - hsize;
  spawn_bbox.max.x = cam_pos.x + hsize;
  spawn_bbox.max.z = cam_pos.z + hsize;

  i32 selector = GetRandomValue(0, 3);
  switch(selector)
  {
    case 0:
    {
      spawn_bbox.min.z = spawn_bbox.min.z + hsize * 2;
      spawn_bbox.max.z = spawn_bbox.max.z + hsize * 2;
    } break;
    case 1:
    {
      spawn_bbox.min.z = spawn_bbox.min.z - hsize * 2;
      spawn_bbox.max.z = spawn_bbox.max.z - hsize * 2;
    } break;
    case 2:
    {
      spawn_bbox.min.x = spawn_bbox.min.x - hsize * 2;
      spawn_bbox.max.x = spawn_bbox.max.x - hsize * 2;
    } break;
    case 3:
    {
      spawn_bbox.min.x = spawn_bbox.min.x + hsize * 2;
      spawn_bbox.max.x = spawn_bbox.max.x + hsize * 2;
    } break;
  }

  Vector3 pos = (Vector3) { GetRandomValue(spawn_bbox.min.x, spawn_bbox.max.x), 1, GetRandomValue(spawn_bbox.min.z, spawn_bbox.max.z) };

  int ent_id = component_system_pop("co_asteroid");
  if (ent_id == -1)
  {
    ent_id = component_system_create_entity(-1);
    component_system_set_local_transform(ent_id, MatrixTranslate(pos.x, pos.y, pos.z));
    int asteroid_co_id = component_system_create_component(ent_id, "co_asteroid");
    int asteroid_model_id = component_system_create_entity(ent_id);
    component_system_create_component(ent_id, "co_collision");
    component_system_create_component(ent_id, "co_health");
    component_system_create_component_by_cfg(asteroid_model_id, "co_renderer", 2);
    t_co_asteroid* asteroid = cast_ptr(t_co_asteroid) component_system_get("co_asteroid", 0);
    asteroid[asteroid_co_id].model_id = asteroid_model_id;
    asteroid[asteroid_co_id].speed *= 1.2f;
  }

  component_system_set_local_transform(ent_id, MatrixTranslate(pos.x, pos.y, pos.z));
  component_system_update_global_transform(ent_id);

  i32 hp_id = (u64) ctbl_get(component_system_get_entities(0)[ent_id].components, "co_health");
  t_co_health* health = cast_ptr(t_co_health) component_system_get("co_health", 0);
  health[hp_id].is_dead = false;
  health[hp_id].health = 100;
}


void
respawn_handle(int entity_id)
{
  //health_system_on_health_change(entity_id, -100);
  int hp_id = (u64) ctbl_get(component_system_get_entities(0)[entity_id].components, "co_health");
  t_co_health* hps = cast_ptr(t_co_health) component_system_get("co_health", 0);
  if (hps[hp_id].is_dead) return;
  hps[hp_id].health -= 100;
  if (hps[hp_id].health <= 0)
  {
    hps[hp_id].is_dead = true;
    component_system_push(entity_id);
  }

  create_asteroid();
}


void
collision_handle(t_co_collision* c1, t_co_collision* c2)
{
  if (c1->layer == 0)
  {
    if (health_system_on_health_change(c1->entity_id, -50))
    {
      Matrix transform = component_system_get_global_transform(c1->entity_id);
      add_animation(c1->entity_id, extract_position(&transform), c1->layer);
      create_asteroid();
    }
    else
    {
      component_system_add_local_transform(c1->entity_id, MatrixTranslate(c1->normal.x, c1->normal.y, c1->normal.z));
      t_co_asteroid* asteroid = component_system_entity_get_component(c1->entity_id, "co_asteroid");
      asteroid->current_angle += 180 * DEG2RAD;
    }
  }
  else
  if (c1->layer == 1)
  {
    if (health_system_on_health_change(c1->entity_id, -100))
    {
      Matrix transform = component_system_get_global_transform(c1->entity_id);
      add_animation(c1->entity_id, extract_position(&transform), c1->layer);
      save_score();
    }
  }
  
  if (c1->entity_id != c2->entity_id) return;
  
  if (c2->layer == 0)
  {
    if (health_system_on_health_change(c2->entity_id, -50))
    {
      Matrix transform = component_system_get_global_transform(c2->entity_id);
      add_animation(c2->entity_id, extract_position(&transform), c2->layer);
      create_asteroid();
    }
    else
    {
      component_system_add_local_transform(c2->entity_id, MatrixTranslate(c2->normal.x, c2->normal.y, c2->normal.z));
      t_co_asteroid* asteroid = component_system_entity_get_component(c2->entity_id, "co_asteroid");
      asteroid->current_angle += 180 * DEG2RAD;
    }
  }
  else
  if (c2->layer == 1)
  {
    if (health_system_on_health_change(c2->entity_id, -100))
    {
      Matrix transform = component_system_get_global_transform(c2->entity_id);
      add_animation(c2->entity_id, extract_position(&transform), c2->layer);
      save_score();
    }
  }
}


b32
damage_entity(i32 entity_id, i32 dmg)
{
  if (health_system_on_health_change(entity_id, -dmg))
  {
    Matrix transform = component_system_get_global_transform(entity_id);
    add_animation(entity_id, extract_position(&transform), 0);
    create_asteroid();
    add_score(100);
    return true;
  }
  play_once(SFX_ASTEROID_HIT);
  return false;
}


b32
play_explosion(t_sprite_anim* anim)
{
  Camera cam = camera_system_get_camera();
  Vector2 pos = GetWorldToScreen(anim->pos, cam);

  i32 count_per_raw = 8;
  f32 size = explosion[anim->id].width / count_per_raw;
  Rectangle rect = {0};
  rect.width = size;
  rect.height = size;

  rect.x = size * (anim->frame % count_per_raw);
  rect.y = size * (anim->frame / count_per_raw);
  
  Rectangle dst_rect = { pos.x - size * explosion_scale / 2, pos.y - size * explosion_scale / 2, fabsf(rect.width * explosion_scale), fabsf(rect.height * explosion_scale) };
  Vector2 origin = { 0.0f, 0.0f };

  DrawTexturePro(explosion[anim->id], rect, dst_rect, origin, 0.0f, WHITE);
  anim->frame += 1;

  return anim->frame > (count_per_raw * count_per_raw);
}


void
update_sprites()
{
  u64 count = cvec_header(animations)->size;

  for (u64 i = 0, r = count - 1; i < count; i++, r--)
  {
    if (play_explosion(&animations[r]))
    {
      animations = cvec_remove(animations, r);
    }
  }
}


static i32 camera_id;
static t_co_health* ship_hp;


void
lunch_projectile(Vector3 from, Vector3 to)
{
  play_once(SFX_SHIP_FIRE);

  // create projectile

  i32 proj_id = component_system_pop("co_projectile");
  if (proj_id == -1)
  {
    proj_id = component_system_create_entity(-1);
    
    // t_co_collision* coll = component_system_create_component_ptr(proj_id, "co_collision");
    // coll->layer = 2;
    // coll->radius = 0.25;
    // component_system_create_component(proj_id, "co_health");
    component_system_create_component(proj_id, "co_projectile");
  }

  component_system_set_local_transform(proj_id, MatrixTranslate(from.x, 1, from.z));
  component_system_update_global_transform(proj_id);
  
  t_co_projectile* projectile = component_system_entity_get_component(proj_id, "co_projectile");
  projectile->ship = ship;
  projectile->target_id = ship->target;
  projectile->life_timer = 3.0f;

  // lunch projectile
}


typedef void(*button_callback)();

typedef struct
{
  char* options_labels[6];
  button_callback options_callbacks[6];
  u32 count;
  i32 current;
  f32 angle_per_option;

  i32 entity_id;
  i32 options_entities[6];
  b32 blocked;
  const char* tag;
} t_buttons_wheal;


t_buttons_wheal menu_wheal;
t_buttons_wheal options_wheal;
t_buttons_wheal credits_wheal;

t_buttons_wheal* current_wheal;

i32 camera_entity_id;

void
menu_play()
{
  play_once_ui(SFX_UI_START);
  
  {// @todo should make mover when starting the game
    t_smooth_mover* mover = component_system_create_component_ptr(camera_entity_id, "co_smooth_mover");
    mover->target_id = ship->entity_id;
    // component_system_set_local_transform(camera_entity_id, MatrixTranslate(0, 60, 0));
    t_co_camera* cameras = component_system_get("co_camera", 0);
    mover->camera = &cameras[camera_id];
  }

  stop_music(MUSIC_MENU);
  play_music(MUSIC_GAME);
  state = GAME_STATE_GAME;
}

void
menu_options()
{
  current_wheal = &options_wheal;
}

void
menu_credits()
{
  current_wheal = &credits_wheal;
}

void
menu_exit()
{
  event_dispatch(APP_QUIT);
}

t_buttons_wheal
create_menu_wheal()
{
#if defined(PLATFORM_WEB)
  t_buttons_wheal wheal =
  {
    { "Play", "Options", "Credits" },
    { &menu_play, &menu_options, &menu_credits },
    3, 0, - 45 * DEG2RAD
  };
#else
  t_buttons_wheal wheal =
  {
    { "Play", "Options", "Credits", "Exit" },
    { &menu_play, &menu_options, &menu_credits, &menu_exit },
    4, 0, - 45 * DEG2RAD
  };
#endif
  wheal.blocked = false;
  
  return wheal;
}


void
options_sfx()
{
  sfx_mute = !sfx_mute;
  options_wheal.options_labels[0] = sfx_mute ? "Unmute Sfx" : "Mute Sfx";
  for (u64 i = 0; i < SFX_COUNT; i++)
  {
    SetSoundVolume(sfx_tbl[i], !sfx_mute);
  }
}

void
options_music()
{
  music_mute = !music_mute;
  options_wheal.options_labels[1] = music_mute ? "Unmute Music" : "Mute Music";
  for (u64 i = 0; i < MUSIC_COUNT; i++)
  {
    SetMusicVolume(music_tbl[i], !music_mute);
  }
}

void
options_return()
{
  play_once_ui(SFX_UI_BACK);
  current_wheal = &menu_wheal;
}


t_buttons_wheal
create_options_wheal()
{
  t_buttons_wheal wheal =
  {
    { "Mute Sfx", "Mute Music", "Return" },
    { &options_sfx, &options_music, &options_return },
    3, 0, - 45 * DEG2RAD
  };
  
  wheal.current = 2;
  wheal.blocked = false;
  
  return wheal;
}


void
credits_return()
{
  play_once_ui(SFX_UI_BACK);
  current_wheal = &menu_wheal;
}


t_buttons_wheal
create_credits_wheal()
{
  t_buttons_wheal wheal =
  {
    { "Programming: ",
      "Oleksandr Melnichenko",
      "",
      "Sound & Music: ",
      "Aleksander Zaleski"
    },
    { &credits_return, &credits_return, &credits_return, &credits_return, &credits_return },
    5, 0, - 45 * DEG2RAD
  };
  
  wheal.current = 2;
  wheal.blocked = true;
  wheal.tag = "credits";
  
  return wheal;
}


void
update_wheal(t_buttons_wheal* wheal)
{
  if (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)) return;
  if (IsKeyPressed(KEY_ENTER))
  {
    play_once_ui(SFX_UI_SELECT);
    wheal->options_callbacks[wheal->current]();
  }
  
  if (wheal->blocked) return;
  
  if (IsKeyPressed(KEY_UP))
  {
    play_once_ui(SFX_UI_SCROLL);
    wheal->current = MAX(0, wheal->current - 1);
  }
  else
  if (IsKeyPressed(KEY_DOWN))
  {
    play_once_ui(SFX_UI_SCROLL);
    wheal->current = MIN(wheal->count - 1, wheal->current + 1);
  }
}

static i32 hower_select = -1;

void
draw_wheal(t_buttons_wheal* wheal)
{
  Vector2 origin = (Vector2){ GetScreenWidth() - 60, GetScreenHeight() / 2 };
  f32 start_x = origin.x;
  f32 start_y = origin.y - 64 - (60 + 64) * wheal->current;

  Vector2 mouse_pos = GetMousePosition();
  i32 mouse_hover = wheal->current;

  for (u32 i = 0; i < wheal->count; i++)
  {
    f32 font_size = 64 * (wheal->current == i ? 2 : 1);
    f32 text_size = MeasureText(wheal->options_labels[i], font_size);
    f32 posX = start_x - text_size - (wheal->current == i ? 40 : 0);
    f32 posY = start_y;
    f32 min_x = posX;
    f32 max_x = posX + text_size;
    f32 min_y = posY - 30;
    f32 max_y = posY + font_size + 30;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
      if (mouse_pos.x > min_x &&
          mouse_pos.x < max_x &&
          mouse_pos.y > min_y &&
          mouse_pos.y < max_y)
      {
        play_once_ui(SFX_UI_SELECT);
        wheal->options_callbacks[i]();
        
        if (!wheal->blocked)
        {
          wheal->current = i;
        }
      }
    }
    else
    {
      if (mouse_pos.x > min_x &&
          mouse_pos.x < max_x &&
          mouse_pos.y > min_y &&
          mouse_pos.y < max_y)
      {
        if (hower_select != i)
        {
          play_once_ui(SFX_UI_SCROLL);
        }
        hower_select = i;
        mouse_hover = i;
      }
    }
    start_y += font_size + 60;
  }

  start_y = origin.y - 64 - (60 + 64) * wheal->current;
  for (u32 i = 0; i < wheal->count; i++)
  {
    f32 font_size = 64 * (wheal->current == i ? 2 : 1);
    f32 text_size = MeasureText(wheal->options_labels[i], font_size);
    Color color = (wheal->current == i ? GOLD : RAYWHITE);
    if (IsKeyDown(KEY_ENTER) && wheal->current == i)
    {
      color = GREEN;
    }
    f32 posX = start_x - text_size - (wheal->current == i ? 40 : 0);
    f32 posY = start_y;
    DrawText(wheal->options_labels[i], posX, posY, font_size, color);
    if (mouse_hover == i)
    {
      DrawText(">", start_x - text_size - font_size, start_y, font_size, GREEN);
    }

    start_y += font_size + 60;
  }
  
  if (wheal->tag && wheal->tag == "credits")
  {
    const char* assets_str[] =
    {
      "Assets from OpenGameArt.org:  ",
      "CarzyMoose - background | ",
      "quaternius - spaceship | ",
      "Michael Kurinnoy - asteroids | ",
      "oglsdl - first aid | ",
      "para - low poly rocks | ",
      "Yughues - fuel tank pack"
    };
    
    i32 start_x = 26;
    for (u32 i = 0; i < 7; i++)
    {
      DrawText(assets_str[i], start_x, GetScreenHeight() - 20 - 10, 20, WHITE);
      start_x += MeasureText(assets_str[i], 20);
    }
  }
}


Shader tiling_shader;
int space_id;


void
game_init()
{
  read_score();
  
  renderer_system_init();
  animation_system_init();
  camera_system_init();
  text_3d_init();
  co_movement_init();
  ship_system_init();
  asteroid_system_init();
  smooth_mover_system_init();
  collision_system_init(&collision_handle);
  health_system_init();
  gun_system_init();
  projectile_system_init();
  pickup_system_init();
  particle_system_init();

  load_textures();
  animations = cvec_create(sizeof(t_sprite_anim), 32, false);

  load_audio();

  renderer_system_add_cfg((t_co_renderer_cfg){ 0, "data/models/characters/body/Rogue_Naked.glb", false, -1 });
  renderer_system_add_cfg((t_co_renderer_cfg){ 1, "data/models/characters/armory/crossbow_2handed.gltf", false, -1 });
	renderer_system_add_cfg((t_co_renderer_cfg){ 2, "data/models/asteroids/asteroid_0.glb", false, -1 });
  renderer_system_add_cfg((t_co_renderer_cfg){ 3, "data/models/ship/bob.gltf", false, -1 });
  renderer_system_add_cfg((t_co_renderer_cfg){ 4, "data/models/can/can.glb", false, -1 });
  renderer_system_add_cfg((t_co_renderer_cfg){ 5, "data/models/bg/space.glb", false, -1 });
  renderer_system_add_cfg((t_co_renderer_cfg){ 6, "data/models/aid/aid.glb", false, -1 });

  camera_system_add_cfg((t_co_camera_cfg){ 0, 85.0f, CAMERA_PERSPECTIVE, 2});
  camera_system_add_cfg((t_co_camera_cfg){ 1, 45.0f, CAMERA_PERSPECTIVE, 1});

  int ship_id = component_system_create_entity(-1);
  component_system_set_local_transform(ship_id, MatrixTranslate(0, 1, 0));
  int ship_co_id = component_system_create_component(ship_id, "co_ship");
  t_co_collision* colls = component_system_get("co_collision", 0);
  int collider_id = component_system_create_component(ship_id, "co_collision");
  colls[collider_id].offset = (Vector3){0,0,-1};
  colls[collider_id].layer = 1;
  collider_id = component_system_create_component(ship_id, "co_collision");
  colls[collider_id].offset = (Vector3){0,0,2};
  colls[collider_id].layer = 1;
  collider_id = component_system_create_component(ship_id, "co_collision");
  colls[collider_id].offset = (Vector3){3.2,0,-0.5};
  colls[collider_id].layer = 1;
  collider_id = component_system_create_component(ship_id, "co_collision");
  colls[collider_id].offset = (Vector3){-3.2,0,-0.5};
  colls[collider_id].layer = 1;
  int gun_id = component_system_create_component(ship_id, "co_gun");

  t_co_ship* ships = component_system_get("co_ship", 0);
  ship = &ships[ship_co_id];

  t_co_gun* guns = component_system_get("co_gun", 0);
  guns[gun_id].ship = ship;
  guns[gun_id].fire_callback = &lunch_projectile;

  int ship_hp_id = component_system_create_component(ship_id, "co_health");
  t_co_health* hps = cast_ptr(t_co_health) component_system_get("co_health", 0);
  hps[ship_hp_id].health = 300;
  ship_hp = &hps[ship_hp_id];
  ship->health = ship_hp;

  camera_entity_id = component_system_create_entity(-1);
  camera_id = component_system_create_component_by_cfg(camera_entity_id, "co_camera", 1);
  component_system_set_local_transform(camera_entity_id, MatrixTranslate(0, 15, 7));
  t_co_camera* cameras = component_system_get("co_camera", 0);
  cameras[camera_id].yaw = 90 * DEG2RAD;
  camera_system_switch(camera_id);

  for (u64 i = 0; i < 100; i++)
  {
    create_asteroid();
  }

  menu_wheal = create_menu_wheal();
  options_wheal = create_options_wheal();
  credits_wheal = create_credits_wheal();
  current_wheal = &menu_wheal;
  
  space_id = component_system_create_entity(-1);
  component_system_set_local_transform(space_id, MatrixTranslate(0, -3, 0));
  int space_renderer_id = component_system_create_component_by_cfg(space_id, "co_renderer", 5);
  t_co_renderer* renderers = component_system_get("co_renderer", 0);
  Texture2D texture = LoadTexture("data/bg/space_2.png");
  SetTextureWrap(texture, TEXTURE_WRAP_REPEAT);
  co_renderer_set_texture(&renderers[space_renderer_id], texture);
  
  tiling_shader = LoadShader(0, TextFormat("data/shaders/glsl%i/tiling.fs", GLSL_VERSION));
  
  float tiling[2] = { 2.0f, 2.0f };
  SetShaderValue(tiling_shader, GetShaderLocation(tiling_shader, "tiling"), tiling, SHADER_UNIFORM_VEC2);
  
  float offset[2] = { 1.5f, 1.0f };
  SetShaderValue(tiling_shader, GetShaderLocation(tiling_shader, "offset"), offset, SHADER_UNIFORM_VEC2);
  
  co_renderer_set_shader(&renderers[space_renderer_id], tiling_shader);
}


void
game_fixed_update(f32 dt)
{
  // if (in_move)
  {
    collision_system_update();
  }
}


static int height = 20;
static b32 paused_from_game = false;


void
game_update(f32 dt)
{
  switch (state)
  {
  case GAME_STATE_MENU:
  {
    update_wheal(current_wheal);
    ship_system_tilt_update(dt);
    
    if (IsKeyPressed(KEY_ESCAPE) && paused_from_game)
    {
      state = GAME_STATE_GAME;
      paused_from_game = false;
    }
    
  } break;
  case GAME_STATE_GAME:
  {
    if (IsKeyPressed(KEY_ESCAPE))
    {
      state = GAME_STATE_MENU;
      paused_from_game = true;
    }
  
    if (!component_system_entity_is_enabled(ship->entity_id))
    {
      if (IsKeyPressed(KEY_SPACE))
      {
        // @todo: reset asteroids speed
        asteroid_system_reset();
      
        score = 0;
        int id = component_system_pop("co_ship");
        component_system_set_local_transform(id, MatrixTranslate(0, 1, 0));
        component_system_update_global_transform(id);
        ship->health->health = 300;
        ship->fuel = 100.0f;
        ship->health->is_dead = false;
        ship->move_pos = (Vector3){0, 1, 0};
        SetMusicVolume(music_tbl[MUSIC_GAME], 0.75f);
        component_system_set_local_transform(camera_entity_id, MatrixTranslate(0, 15, 0));
      }
    }
    else
    {
      ship_system_update(dt, &end_move);
    }

    {
      asteroid_system_update(dt, &respawn_handle);
      gun_system_update(dt);
      projectile_system_update(dt, &damage_entity);
      pickup_system_update(dt);
      particle_system_update(dt);
    }
  } break;
  }

  update_music();
}

f32 rotcircle = 0;

b32
cursor_on_target()
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
      return true;
    }
  }

  return false;
}

void
game_render(f32 dt)
{
  (void)dt;

	u64 count = 0;
  t_co_ship* ships = cast_ptr(t_co_ship) component_system_get("co_ship", &count);

  for (u64 i = 0; i < count; i++)
	{
    Matrix transform = component_system_get_global_transform(ships[i].entity_id);
    Vector3 ship_position = extract_position(&transform);

    Vector3 center = (Vector3){ships[i].move_pos.x, 0, ships[i].move_pos.z};

    f32 radius = 0.5;
    f32 h_radius = 0.75 * 0.5;
    Vector3 helpers[] = 
    {
      (Vector3) {-h_radius,0,-h_radius},
      (Vector3) {h_radius,0,h_radius},
      (Vector3) {-h_radius,0,h_radius},
      (Vector3) {h_radius,0,-h_radius}
    };

    BoundingBox bbox = {0};
    bbox.min = Vector3Add(center, helpers[0]);
    bbox.max = Vector3Add(center, helpers[1]);
    DrawLine3D(bbox.min, bbox.max, RED);
    bbox.min = Vector3Add(center, helpers[2]);
    bbox.max = Vector3Add(center, helpers[3]);
    DrawLine3D(bbox.min, bbox.max, RED);
    DrawCircle3D(center, radius, (Vector3){1,0,0}, 90, RED);

    if (ships[i].target != -1)
    {
      t_entity* ents = component_system_get_entities(0);
      if (!ents[ships[i].target].enabled)
      {
        ships[i].target = -1;
        return;
      }
    }

    if (cursor_on_target())
    {
      rotcircle = 90;
      DrawCircle3D(ship_position, 20, (Vector3){1,0,0}, rotcircle, RED);
    }
  }

  t_co_projectile* projectiles = cast_ptr(t_co_projectile) component_system_get("co_projectile", &count);
  for (u64 i = 0; i < count; i++)
  {
    if (!component_system_entity_is_enabled(projectiles[i].entity_id))
    {
      continue;
    }
    Matrix transform = component_system_get_global_transform(projectiles[i].entity_id);
    Vector3 pos = extract_position(&transform);
    DrawSphere(pos, 0.2f, GOLD);
  }
}


void
game_draw_ui()
{
  switch (state)
  {
  case GAME_STATE_MENU:
  {
    // @todo draw max score
    char str_buffer[128] = {0};
    sprintf(str_buffer, "HIGH SCORE: %09d", max_score);
    DrawText(str_buffer, GetScreenWidth() / 2 - MeasureText(str_buffer, 40) / 2, 20, 40, RAYWHITE);
    draw_wheal(current_wheal);
  } break;
  case GAME_STATE_GAME:
  {
    char hp_points_str[128] = {0};

    sprintf(hp_points_str, "%09d", score);
    DrawText(hp_points_str, GetScreenWidth() / 2 - MeasureText(hp_points_str, 40) / 2, 20, 40, RAYWHITE);

    sprintf(hp_points_str, "FUEL: %.0f", ship->fuel);
    DrawText(hp_points_str, GetScreenWidth() - MeasureText(hp_points_str, 20) - 20, 20, 20, RAYWHITE);

    sprintf(hp_points_str, "%dHP", ship_hp->health);
    DrawText(hp_points_str, 20, 20, 20, ship_hp->is_dead ? RED : RAYWHITE);
    
    if (ship && !component_system_entity_is_enabled(ship->entity_id))
    {
      const char* msg = "PRESS [SPACE] TO RESTART";
      DrawText(msg, GetScreenWidth() / 2 - MeasureText(msg, 40) / 2, GetScreenHeight() - 200, 40, RAYWHITE);
    }
  } break;
  }

  particle_system_draw();
  update_sprites();
  
  if (ship && ship->target != -1)
  {
    Matrix target_transform = component_system_get_global_transform(ship->target);
    Vector3 target_position = extract_position(&target_transform);
    Vector2 screen_pos = GetWorldToScreen(target_position, camera_system_get_camera());
    screen_pos.x -= 16;
    screen_pos.y -= 32;
    DrawTextCodepoint(GetFontDefault(), 'x', screen_pos, 64, RED);
  }
  
}


void
game_term()
{
  co_movement_term();
}


void
game_late_update(f32 dt)
{
  smooth_mover_system_update(dt);
  camera_system_update(dt);
}


void DrawTextureQuad(Texture2D texture, Vector2 tiling, Vector2 offset, Rectangle quad, Color tint)
{
  Rectangle source = { offset.x*texture.width, offset.y*texture.height, tiling.x*texture.width, tiling.y*texture.height };
  Vector2 origin = { 0.0f, 0.0f };

  DrawTexturePro(texture, source, quad, origin, 0.0f, tint);
}

static f32 alternative_offset = 0;
static f32 alternative_offset_speed = 0.1;
void
game_pre_render(f32 dt)
{
  Matrix transform = component_system_get_global_transform(camera_id);
  Vector3 pos = extract_position(&transform);
  
  f32 x_mult = 0.0114f;
  f32 y_mult = 0.0114f;

  Matrix plane_transform_prev = component_system_get_local_transform(space_id);
  Vector3 plane_prev_pos = extract_position(&plane_transform_prev);
  
  pos.y = -3;
  
  Vector3 offset = Vector3Subtract(plane_prev_pos, pos);
  
  switch (state)
  {
  case GAME_STATE_MENU:
  {
    float tiling[2] = { 3.5f, 3.5f };
    SetShaderValue(tiling_shader, GetShaderLocation(tiling_shader, "tiling"), tiling, SHADER_UNIFORM_VEC2);
  
    alternative_offset += dt;
    float offset[2] = { 0, alternative_offset * alternative_offset_speed };
    SetShaderValue(tiling_shader, GetShaderLocation(tiling_shader, "offset"), offset, SHADER_UNIFORM_VEC2);
  } break;
  case GAME_STATE_GAME:
  {
    float tiling[2] = { 2.0f, 2.0f };
    SetShaderValue(tiling_shader, GetShaderLocation(tiling_shader, "tiling"), tiling, SHADER_UNIFORM_VEC2);
  
    float offset[2] = { pos.x * x_mult, pos.z * y_mult };
    SetShaderValue(tiling_shader, GetShaderLocation(tiling_shader, "offset"), offset, SHADER_UNIFORM_VEC2);
  } break;
  }
  
  Matrix plane_transform = MatrixTranslate(pos.x,-3,pos.z);
  component_system_set_local_transform(space_id, plane_transform);
}


void
app_create(t_app* out_app)
{
  out_app->name = "Game";
  out_app->window_pos_x = 100;
  out_app->window_pos_y = 100;
  out_app->window_size_x = WINDOW_WIDTH;
  out_app->window_size_y = WINDOW_HEIGHT;
  out_app->init = &game_init;
  out_app->update = &game_update;
  out_app->pre_render = &game_pre_render;
  out_app->late_update = &game_late_update;
	out_app->fixed_update = &game_fixed_update;
  out_app->render = &game_render;
  out_app->draw_ui = &game_draw_ui;
  out_app->term = &game_term;
}
