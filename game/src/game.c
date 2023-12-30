#include <entry.h>
#include <raylib.h>

#include <systems/component_system.h>
#include <systems/scene_system.h>
#include <systems/shaders.h>

#include <raygui.h>
#include <rlgl.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static int WINDOW_WIDTH = 1920;
static int WINDOW_HEIGHT = 1080;


#include "components/text_3d.h"
#include "components/movement.h"
#include "components/smooth_mover.h"
#include "components/collision.h"
#include "components/health.h"
#include "components/ship.h"
#include "components/asteroid.h"
#include "components/gun.h"
#include "components/projectile.h"

#include <containers/ctbl.h>


typedef struct
{
  Vector3 pos;
  i32 id;
  i32 frame;
} t_sprite_anim;


typedef enum
{
  GAME_STATE_MENU = 0,
  GAME_STATE_GAME,
  GAME_STATE_PAUSE
} e_game_state;

e_game_state state = GAME_STATE_MENU;


static Mesh grid_mesh;
static Shader grid_shader;
#define EXPLOSION_COUNT 4
static Texture2D explosion[EXPLOSION_COUNT];
i32 explosion_frame = 0;
f32 explosion_time = 0;

static b32 in_move = false;

static Texture2D background;

static cvec(t_sprite_anim) animations;

t_co_ship* ship = 0;

static i32 score = 0;


void
load_textures()
{
  background = LoadTexture("data/bg/space.png");
  SetTextureWrap(background, TEXTURE_WRAP_REPEAT);

  explosion[0] = LoadTexture("data/sprites/fire/full_explosion_0.png");
  explosion[1] = LoadTexture("data/sprites/fire/full_explosion_1.png");
  explosion[2] = LoadTexture("data/sprites/fire/full_explosion_2.png");
  explosion[3] = LoadTexture("data/sprites/fire/full_explosion_3.png");
}


typedef enum
{
  MUSIC_GAME,
  MUSIC_SHIP_MOVE
} e_music;


typedef enum
{
  SFX_COLLISION,
  SFX_SHIP_FIRE,
  SFX_SHIP_LUNCH
} e_sfx;


const char* music_game_path = "data/audio/cosmohunter_loop.wav";
const char* music_ship_move_path = "data/audio/space_ship_move_loop.wav";

const char* sfx_collision_path = "data/audio/space_collision.wav";
const char* sfx_ship_fire_path = "data/audio/space_ship_fire.wav";
const char* sfx_ship_lunch_path = "data/audio/space_ship_lunch.wav";


Music music_tbl[2] = {0};
Sound sfx_tbl[3] = {0};


void
play_once(e_sfx sfx)
{
  // RayPlaySound(sfx_tbl[sfx]);
  PlaySound(sfx_tbl[sfx]);
}


void
play_music(e_music music)
{
  //if (!IsMusicStreamPlaying(music_tbl[music]))
  PlayMusicStream(music_tbl[music]);
}


void
stop_music(e_music music)
{
  //if (IsMusicStreamPlaying(music_tbl[music]))
  StopMusicStream(music_tbl[music]);
}


void
update_music()
{
  for (u64 i = 0; i < 2; i++)
  {
    UpdateMusicStream(music_tbl[i]);
  }
}


static float exponent = 1.0f;                 // Audio exponentiation value
static float averageVolume[400] = { 0.0f };   // Average volume history

void ProcessAudio(void *buffer, unsigned int frames)
{
    float *samples = (float *)buffer;   // Samples internally stored as <float>s
    float average = 0.0f;               // Temporary average volume

    for (unsigned int frame = 0; frame < frames; frame++)
    {
        float *left = &samples[frame * 2 + 0], *right = &samples[frame * 2 + 1];

        *left = powf(fabsf(*left), exponent) * ( (*left < 0.0f)? -1.0f : 1.0f );
        *right = powf(fabsf(*right), exponent) * ( (*right < 0.0f)? -1.0f : 1.0f );

        average += fabsf(*left) / frames;   // accumulating average volume
        average += fabsf(*right) / frames;
    }

    // Moving history to the left
    for (int i = 0; i < 399; i++) averageVolume[i] = averageVolume[i + 1];

    averageVolume[399] = average;         // Adding last average value
}


void
load_audio()
{
  InitAudioDevice();

  AttachAudioMixedProcessor(ProcessAudio);

  music_tbl[MUSIC_GAME] = LoadMusicStream(music_game_path);
  music_tbl[MUSIC_GAME].looping = true;
  SetMusicVolume(music_tbl[MUSIC_GAME], 0.75f);
  music_tbl[MUSIC_SHIP_MOVE] = LoadMusicStream(music_ship_move_path);
  music_tbl[MUSIC_SHIP_MOVE].looping = true;
  SetMusicVolume(music_tbl[MUSIC_SHIP_MOVE], 1.5f);

  sfx_tbl[SFX_COLLISION] = LoadSound(sfx_collision_path);
  sfx_tbl[SFX_SHIP_FIRE] = LoadSound(sfx_ship_fire_path);
  sfx_tbl[SFX_SHIP_LUNCH] = LoadSound(sfx_ship_lunch_path);
  SetSoundVolume(sfx_tbl[SFX_SHIP_LUNCH], 0.5f);

  play_music(MUSIC_GAME);
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


void
add_animation(int entity_id, Vector3 pos)
{
  t_entity* entities = component_system_get_entities(0);
  t_sprite_anim anim = {0};
  anim.pos = pos;
  anim.frame = 0;

  if (ctbl_exist(entities[entity_id].components, "co_asteroid"))
  {
    anim.id = 0;
    if (ship->target == entity_id)
    {
      ship->target = -1;
    }
  }
  else
  if (ctbl_exist(entities[entity_id].components, "co_ship"))
  {
    anim.id = 3;
  }

  Vector2 screen_pos = GetWorldToScreen(pos, camera_system_get_camera());

  if (screen_pos.x < 0 || screen_pos.x > GetScreenWidth() ||
      screen_pos.y < 0 || screen_pos.y > GetScreenHeight())
  {
    return;
  }

  play_once(SFX_COLLISION);

  animations = cvec_push(animations, &anim);
}


static void
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
collision_handle(t_co_collision* c1, t_co_collision* c2)//int e1, int e2)
{
  if (health_system_on_health_change(c1->entity_id, -100))
  {
    Matrix transform = component_system_get_global_transform(c1->entity_id);
    add_animation(c1->entity_id, extract_position(&transform));

    create_asteroid();
  }

  if (health_system_on_health_change(c2->entity_id, -100) && c1->entity_id != c2->entity_id)
  {
    Matrix transform = component_system_get_global_transform(c2->entity_id);
    add_animation(c2->entity_id, extract_position(&transform));

    create_asteroid();
  }
}


b32
damage_entity(i32 entity_id, i32 dmg)
{
  if (health_system_on_health_change(entity_id, -dmg))
  {
    Matrix transform = component_system_get_global_transform(entity_id);
    add_animation(entity_id, extract_position(&transform));
    create_asteroid();
    score++;
    return true;
  }
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

  DrawTextureRec(explosion[anim->id], rect, (Vector2) {pos.x - size / 2, pos.y - size / 2}, WHITE);

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
    i32 proj_comp_id = component_system_create_component(proj_id, "co_projectile");
  }

  component_system_set_local_transform(proj_id, MatrixTranslate(from.x, 1, from.z));
  component_system_update_global_transform(proj_id);
  
  t_co_projectile* projectile = component_system_entity_get_component(proj_id, "co_projectile");
  projectile->ship = ship;
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
} t_buttons_wheal;


t_buttons_wheal menu_wheal;
t_buttons_wheal options_wheal;

t_buttons_wheal* current_wheal;

i32 camera_entity_id;

void
menu_play()
{
  {// @todo should make mover when starting the game
    t_smooth_mover* mover = component_system_create_component_ptr(camera_entity_id, "co_smooth_mover");
    mover->target_id = ship->entity_id;
    // component_system_set_local_transform(camera_entity_id, MatrixTranslate(0, 60, 0));
    t_co_camera* cameras = component_system_get("co_camera", 0);
    mover->camera = &cameras[camera_id];
  }

  state = GAME_STATE_GAME;
}

void
menu_options()
{
  current_wheal = &options_wheal;
}

void
menu_exit()
{
  event_dispatch(APP_QUIT);
}

t_buttons_wheal
create_menu_wheal()
{
  t_buttons_wheal wheal =
  {
    { "Play", "Options", "Exit" },
    { &menu_play, &menu_options, &menu_exit },
    3, 0, - 45 * DEG2RAD
  };
  
  return wheal;
}


b32 sfx_mute = false;
void
options_sfx()
{
  sfx_mute = !sfx_mute;
  options_wheal.options_labels[0] = sfx_mute ? "Unmute Music" : "Mute Music";
  for (u64 i = 0; i < 3; i++)
  {
    SetSoundVolume(sfx_tbl[i], !sfx_mute);
  }
}

b32 music_mute = false;
void
options_music()
{
  music_mute = !music_mute;
  options_wheal.options_labels[1] = music_mute ? "Unmute Sfx" : "Mute Sfx";
  for (u64 i = 0; i < 2; i++)
  {
    SetMusicVolume(music_tbl[i], !music_mute);
  }
}

void
options_return()
{
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
  
  return wheal;
}


void
update_wheal(t_buttons_wheal* wheal)
{
  if (IsKeyPressed(KEY_ENTER))
  {
    wheal->options_callbacks[wheal->current]();
  }
  if (IsKeyPressed(KEY_UP))
  {
    wheal->current = MAX(0, wheal->current - 1);
  }
  else
  if (IsKeyPressed(KEY_DOWN))
  {
    wheal->current = MIN(wheal->count - 1, wheal->current + 1);
  }
}


void
draw_wheal(t_buttons_wheal* wheal)
{
  Vector2 origin = (Vector2){ GetScreenWidth() - 20, GetScreenHeight() / 2 };
  f32 start_x = origin.x;
  f32 start_y = origin.y - 64 - (60 + 32) * wheal->current;

  for (u32 i = 0; i < wheal->count; i++)
  {
    f32 font_size = 64 * (wheal->current == i ? 2 : 1);
    f32 text_size = MeasureText(wheal->options_labels[i], font_size);
    Color color = (wheal->current == i ? GOLD : RAYWHITE);
    if (IsKeyDown(KEY_ENTER) && wheal->current == i)
    {
      color = GREEN;
    }
    DrawText(wheal->options_labels[i], start_x - text_size - (wheal->current == i ? 40 : 0), start_y, font_size, color);
    if (wheal->current == i)
    {
      DrawText(">", start_x - text_size - 128, start_y, 128, GREEN);
    }
    start_y += font_size / 2 + 60;
  }
}


void
game_init()
{
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

  load_textures();
  animations = cvec_create(sizeof(t_sprite_anim), 32, false);

  load_audio();

  renderer_system_add_cfg((t_co_renderer_cfg){ 0, "data/models/characters/body/Rogue_Naked.glb", false, -1 });
  renderer_system_add_cfg((t_co_renderer_cfg){ 1, "data/models/characters/armory/crossbow_2handed.gltf", false, -1 });
	renderer_system_add_cfg((t_co_renderer_cfg){ 2, "data/models/asteroids/asteroid_0.glb", false, -1 });
  renderer_system_add_cfg((t_co_renderer_cfg){ 3, "data/models/ship/bob.gltf", false, -1 });

  camera_system_add_cfg((t_co_camera_cfg){ 0, 85.0f, CAMERA_PERSPECTIVE, 2});
  camera_system_add_cfg((t_co_camera_cfg){ 1, 45.0f, CAMERA_PERSPECTIVE, 1});

  cvec(u8) ip_vs = shader_load("data/shaders/glsl330/infinite_plane.vs");
  cvec(u8) ip_fs = shader_load("data/shaders/glsl330/infinite_plane.fs");
  grid_shader = LoadShaderFromMemory(ip_vs, ip_fs);
  grid_mesh = GenMeshPlane(1, 1, 1, 1);

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
  current_wheal = &menu_wheal;
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


void
game_update(f32 dt)
{
  switch (state)
  {
  case GAME_STATE_MENU:
  {
    update_wheal(current_wheal);
  } break;
  case GAME_STATE_GAME:
  {
    if (!component_system_entity_is_enabled(ship->entity_id))
    {
      if (IsKeyPressed(KEY_SPACE))
      {
        int id = component_system_pop("co_ship");
        component_system_set_local_transform(id, MatrixTranslate(0, 1, 0));
        component_system_update_global_transform(id);
        ship->health->health = 300;
        ship->health->is_dead = false;
        ship->move_pos = (Vector3){0, 1, 0};
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
    }
  } break;
  }

  update_music();
}


void
game_render(f32 dt)
{
  (void)dt;

  // for (u32 i = 0; i < menu_wheal.count; i++)
  // {
  //   Matrix transform = component_system_get_global_transform(menu_wheal.options_entities[i]);
  //   Vector3 pos = extract_position(&transform);
  //   DrawSphere(pos, 1.0f, GOLD);
  // }

	u64 count = 0;
  t_co_ship* ships = cast_ptr(t_co_ship) component_system_get("co_ship", &count);

  for (u64 i = 0; i < count; i++)
	{
    Matrix transform = component_system_get_global_transform(ships[i].entity_id);
    Vector3 ship_position = extract_position(&transform);

    DrawSphere((Vector3){ships[i].move_pos.x, 0, ships[i].move_pos.z}, 0.3f, MAGENTA);

    if (ships[i].target != -1)
    {
      t_entity* ents = component_system_get_entities(0);
      if (!ents[ships[i].target].enabled)
      {
        ships[i].target = -1;
        return;
      }
      Matrix target_transform = component_system_get_global_transform(ships[i].target);
      Vector3 target_position = extract_position(&target_transform);
      DrawLine3D(ship_position, target_position, RED);
    }
  }

  // if (!in_move)
  // {
	//   for (u64 i = 0; i < count; i++)
	//   {
  //     Matrix transform = component_system_get_global_transform(ships[i].entity_id);
  //     Vector3 ship_position = extract_position(&transform);
  //     u64 points_count = cvec_header(ships[i].points)->size;
  //     for (u64 j = 0; j < points_count; j++)
  //     {
  //       DrawSphere(ships[i].points[j], 0.1f, MAGENTA);
  //     }

  //     if (ships[i].target != -1)
  //     {
  //       t_entity* ents = component_system_get_entities(0);
  //       if (!ents[ships[i].target].enabled)
  //       {
  //         ships[i].target = -1;
  //         return;
  //       }
  //       Matrix target_transform = component_system_get_global_transform(ships[i].target);
  //       Vector3 target_position = extract_position(&target_transform);
  //       DrawLine3D(ship_position, target_position, RED);
  //     }
	//   }
  // }

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
  

  // u64 colliders_count = 0;
  // t_co_collision* colliders = cast_ptr(t_co_collision) component_system_get("co_collision", &colliders_count);
	// for (u64 i = 0; i < colliders_count; i++)
	// {
  //   Matrix transform = component_system_get_global_transform(colliders[i].entity_id);
  //   Vector3 pos = extract_position(&transform);
  //   Vector3 forward = (Vector3) {transform.m8, transform.m9, transform.m10};
  //   forward = Vector3Scale(forward, colliders[i].offset.z);
  //   Vector3 right = (Vector3) {transform.m0, transform.m1, transform.m2};
  //   right = Vector3Scale(right, colliders[i].offset.x);
  //   DrawSphereWires(Vector3Add(Vector3Add(pos, forward), right), colliders[i].radius, 16, 16, GREEN);
	// }


	// rlDisableBackfaceCulling();
  // renderer_system_draw_simple(grid_mesh, grid_shader);
  // rlEnableBackfaceCulling();
}


void
game_draw_ui()
{
  switch (state)
  {
  case GAME_STATE_MENU:
  {
    draw_wheal(current_wheal);
  } break;
  case GAME_STATE_GAME:
  {
    char hp_points_str[128] = {0};

    sprintf(hp_points_str, "%09d", score);
    DrawText(hp_points_str, GetScreenWidth() / 2 - MeasureText(hp_points_str, 40) / 2, 20, 40, RAYWHITE);

    int fps = GetFPS();
    sprintf(hp_points_str, "%d", fps);
    DrawText(hp_points_str, 20, 20, 20, fps < 30 ? RED : (fps < 60 ? YELLOW : GREEN));

    sprintf(hp_points_str, "%dHP", ship_hp->health);
    DrawText(hp_points_str, 20, 45, 20, ship_hp->is_dead ? RED : RAYWHITE);

    Camera camera = camera_system_get_camera();
    sprintf(hp_points_str, "[ %f %f %f ]", camera.position.x, camera.position.y, camera.position.z);
    DrawText(hp_points_str, 20, 70, 20, RAYWHITE);
  } break;
  }



  update_sprites();
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


void
game_pre_render(f32 dt)
{
  Matrix transform = component_system_get_global_transform(camera_id);
  Vector3 pos = extract_position(&transform);

  Vector2 offset = (Vector2) {-pos.x/GetScreenWidth()*20, -pos.z/GetScreenHeight()*20};
  Rectangle dest = { 0, 0, GetScreenWidth(), GetScreenHeight() };

  DrawTextureQuad(background, (Vector2) {1,1}, offset, dest, WHITE);
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
