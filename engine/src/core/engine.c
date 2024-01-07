#include "engine.h"

#include "event.h"

#include "core/mem.h"
#include "core/fs.h"

#include <raylib.h>
#include <raymath.h>

#include <math.h>
#include <string.h>
#include <stdio.h>

// TODO: make c file to compile to raylib
#define RAYGUI_IMPLEMENTATION
#include <raygui.h>

#include "rlgl.h"

#if defined(PLATFORM_DESKTOP)
#define GLSL_VERSION 330
#else
#define GLSL_VERSION 100
#endif

#undef RAYGUI_IMPLEMENTATION

#include <core/logger.h>
#include <containers/cvec.h>
#include <containers/cstr.h>
#include <core/asserts.h>

#include <systems/component_system.h>
#include <systems/mesh_system.h>
#include <systems/scene_system.h>
#include <systems/shaders.h>

//#define START_FULL_SCREEN

// #define RLIGHTS_IMPLEMENTATION
// #include <rlights.h>

const f64 FIXED_TIME = 1.0 / 60.0;

static t_engine engine;

static void
on_app_quit(t_event* e)
{
  (void)e;
  engine.is_running = false;
}

RenderTexture2D LoadRenderTextureDepthTex(int width, int height)
{
  RenderTexture2D target = { 0 };
  
  target.id = rlLoadFramebuffer(width, height);
  
  if (target.id > 0)
  {
    rlEnableFramebuffer(target.id);
    
    // Create color texture (default to RGBA)
    target.texture.id = rlLoadTexture(0, width, height,
                                      PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);
    target.texture.width = width;
    target.texture.height = height;
    target.texture.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    target.texture.mipmaps = 1;
    
    // Create depth texture buffer (instead of raylib default renderbuffer)
    target.depth.id = rlLoadTextureDepth(width,
                                         height,
                                         false);
    target.depth.width = width;
    target.depth.height = height;
    target.depth.format = 19;     //DEPTH_COMPONENT_24BIT?
    target.depth.mipmaps = 1;
    
    // Attach color texture and depth texture to FBO
    rlFramebufferAttach(target.id, target.texture.id,
                        RL_ATTACHMENT_COLOR_CHANNEL0,
                        RL_ATTACHMENT_TEXTURE2D, 0);
    rlFramebufferAttach(target.id, target.depth.id,
                        RL_ATTACHMENT_DEPTH,
                        RL_ATTACHMENT_TEXTURE2D, 0);
    
    // Check if fbo is complete with attachments (valid)
    if (rlFramebufferComplete(target.id))
    {
      TRACELOG(LOG_INFO, "FBO: [ID %i] Framebuffer object created successfully",
               target.id);
    }
    
    rlDisableFramebuffer();
  }
  else TRACELOG(LOG_WARNING, "FBO: Framebuffer object can not be created");
  
  return target;
}


void UnloadRenderTextureDepthTex(RenderTexture2D target)
{
  if (target.id > 0)
  {
    // Color texture attached to FBO is deleted
    rlUnloadTexture(target.texture.id);
    rlUnloadTexture(target.depth.id);
    
    // NOTE: Depth texture is automatically
    // queried and deleted before deleting framebuffer
    rlUnloadFramebuffer(target.id);
  }
}

void
engine_create(t_app* app)
{
  engine.app = app;
  
  logger_init();
  
  event_init();
  event_register(on_app_quit, APP_QUIT);
  
  // input_init();
  // renderer_init();

  shaders_init();
  mesh_system_init();
  component_system_init();
  scene_system_init();

  // SetTraceLogLevel(RL_LOG_ERROR);
  InitWindow(app->window_size_x, app->window_size_y, app->name);
  
  //SetConfigFlags(FLAG_WINDOW_UNDECORATED);

#ifdef START_FULL_SCREEN
  SetWindowState(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI | FLAG_MSAA_4X_HINT);
  ToggleFullscreen();
#else
  SetWindowState(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI | FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
#endif

  SetTargetFPS(60);
}

// void
// engine_push_bone(t_anim_bone* anim_bone)
// {
//   scene_push_anim_bone(scene_get_current(), anim_bone);
// }

b8
engine_run()
{
  engine.is_running = true;
  
  f64 timer = 0.0;
  f64 current = GetTime();
  f64 accumulator = 0.0;
  f64 fresh = 0.0;
  f64 delta = 0.0;


  //Shader shader = LoadShader(0,
  //                           TextFormat("data/shaders/glsl%i/write_depth.fs",
  //                                      GLSL_VERSION));
  
  // Shader light_shader = LoadShader(TextFormat("data/shaders/glsl%i/lighting.vs", GLSL_VERSION),
  //                                  TextFormat("data/shaders/glsl%i/lighting.fs", GLSL_VERSION));
  // light_shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(light_shader, "viewPos");

  // int ambientLoc = GetShaderLocation(light_shader, "ambient");
  // SetShaderValue(light_shader, ambientLoc, (float[4]){ 4.0f, 4.0f, 4.0f, 4.0f }, SHADER_UNIFORM_VEC4);

  // Light lights[MAX_LIGHTS] = { 0 };
  // Color light_color = {0};
  // light_color.r = 0x2b;// 2b5699
  // light_color.g = 0x56;
  // light_color.b = 0x99;
  // light_color.a = 100;
  // lights[0] = CreateLight(LIGHT_POINT, (Vector3){ 3, 15, -3 }, Vector3Zero(), light_color, light_shader);
  // light_color.r = 0xd3;// d38f40 
  // light_color.g = 0xf4;
  // light_color.b = 0x40;
  // light_color.a = 0xd0;
  // lights[1] = CreateLight(LIGHT_POINT, (Vector3){ -3, 3, 3 }, Vector3Zero(), light_color, light_shader);
  //lights[1] = CreateLight(LIGHT_POINT, (Vector3){ 2, 1, 2 }, Vector3Zero(), RED, light_shader);
  //lights[2] = CreateLight(LIGHT_POINT, (Vector3){ -2, 1, 2 }, Vector3Zero(), GREEN, light_shader);
  //lights[3] = CreateLight(LIGHT_POINT, (Vector3){ 2, 1, -2 }, Vector3Zero(), BLUE, light_shader);
  
  RenderTexture2D target, gizmo_target;
  
  int display = GetCurrentMonitor();      
  if (!IsWindowFullscreen())
  {
    SetWindowSize(engine.app->window_size_x, engine.app->window_size_y);
  
    target = LoadRenderTextureDepthTex(engine.app->window_size_x, 
                                       engine.app->window_size_y);
    gizmo_target = LoadRenderTextureDepthTex(engine.app->window_size_x, 
                                             engine.app->window_size_y);
  }
  else
  {
    i32 width = GetMonitorWidth(display);
    i32 height = GetMonitorHeight(display);
    printf("%d, %d\n", width, height);
    SetWindowSize(width, height);
  
    target = LoadRenderTextureDepthTex(width, 
                                       height);
    gizmo_target = LoadRenderTextureDepthTex(width, 
                                             height);
  }

  engine.app->init();

  // {
  //   t_scene* scene = scene_get_current();
  //   for (u64 i = 0; i < scene->renderers->size; i++)
  //   {
  //     t_co_renderer* renderer = scene_get_renderer(scene, i);
  //     renderer_set_shader(renderer, light_shader);
  //   }
  // }

  // float runTime = 0.0f;
  
  SetExitKey(KEY_F12);

  while(engine.is_running)
  {
    // input_pre_update();
    
    // platform_pump_msg();
    if (WindowShouldClose())
    {
      engine.is_running = false;
    }
    
    // input_update();
    
    if (!engine.is_running) break;
    
    
    if (IsWindowResized() && !IsWindowFullscreen())
    {
      engine.app->window_size_x = GetScreenWidth();
      engine.app->window_size_y = GetScreenHeight();
      
      UnloadRenderTextureDepthTex(target);
      UnloadRenderTextureDepthTex(gizmo_target);
      
      target = LoadRenderTextureDepthTex(engine.app->window_size_x, 
                                         engine.app->window_size_y);
      gizmo_target = LoadRenderTextureDepthTex(engine.app->window_size_x, 
                                               engine.app->window_size_y);
    }
    
    if (IsKeyPressed(KEY_ENTER) && (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)))
    {
      ToggleFullscreen();
      int display = GetCurrentMonitor();      
      if (!IsWindowFullscreen())
      {
        SetWindowSize(engine.app->window_size_x, engine.app->window_size_y);
        UnloadRenderTextureDepthTex(target);
        UnloadRenderTextureDepthTex(gizmo_target);
      
        target = LoadRenderTextureDepthTex(engine.app->window_size_x, 
                                           engine.app->window_size_y);
        gizmo_target = LoadRenderTextureDepthTex(engine.app->window_size_x, 
                                                 engine.app->window_size_y);
      }
      else
      {
        i32 width = GetMonitorWidth(display);
        i32 height = GetMonitorHeight(display);
        SetWindowSize(width, height);
        UnloadRenderTextureDepthTex(target);
        UnloadRenderTextureDepthTex(gizmo_target);
      
        target = LoadRenderTextureDepthTex(width, 
                                           height);
        gizmo_target = LoadRenderTextureDepthTex(width, 
                                                 height);
      }
    }
    
    fresh = GetTime();
    delta = fresh - current;

    current = fresh;
    accumulator += delta;

    Camera camera = camera_system_get_camera();

    engine.app->update(delta);

    // float cameraPos[3] = { camera.position.x, camera.position.y, camera.position.z };
    // SetShaderValue(light_shader, light_shader.locs[SHADER_LOC_VECTOR_VIEW], cameraPos, SHADER_UNIFORM_VEC3);

    // @todo implement lights
    // update_lights();

    // for (int i = 0; i < MAX_LIGHTS; i++) UpdateLightValues(light_shader, lights[i]);

    // update_animations();

    // for (u64 i = 0; i < scene->anim_bones->size; i++)
    // {
    //   t_anim_bone* bone_anim = scene_get_anim_bone(scene, i);
    //   ModelAnimation anim = bone_anim->renderer->anim[bone_anim->renderer->anim_current];
    //   t_scene* scene = scene_get_current();
    //   bone_update(scene->id,
    //               scene_get_entity(scene,
    //                                bone_anim->ent_id),
    //               anim, bone_anim->bone_id,
    //               bone_anim->renderer->anim_frame_counter);
    // }
    
    while (accumulator >= FIXED_TIME)
    {
      if (engine.app->fixed_update)
        engine.app->fixed_update(FIXED_TIME);

      accumulator -= FIXED_TIME;
      timer += FIXED_TIME;
    }
    
    // collect render group
    // update_renderers();

    t_event event = {0};
    event.ctx.f32[0] = delta;
    event_dispatch_ext(&event, APP_UPDATE);

    component_system_update();

    engine.app->late_update(delta);

    // render
    BeginTextureMode(target);

    if (engine.app->pre_render != 0)
    {
      ClearBackground(BLANK);
      engine.app->pre_render(delta);
    }
    
    ClearBackground(BLANK);


    {
      BeginMode3D(camera);

      u64 renderers_count = 0;
      t_co_renderer* renderers = component_system_get("co_renderer", &renderers_count);
      
      for (u64 i = 0; i < renderers_count; i++)
      {
        co_renderer_draw(&renderers[i]);
      }
      
      engine.app->render(delta);

      t_event event = {0};
      event.ctx.f32[0] = delta;
      event_dispatch_ext(&event, APP_RENDER);

      // for (int i = 0; i < MAX_LIGHTS; i++)
      // {
      //     if (lights[i].enabled) DrawSphereEx(lights[i].position, 0.2f, 8, 8, lights[i].color);
      //     else DrawSphereWires(lights[i].position, 0.2f, 8, 8, ColorAlpha(lights[i].color, 0.3f));
      // }

      EndMode3D();
    }

    engine.app->draw_ui();

    EndTextureMode();
    
    if (engine.app->draw_gizmo)
    {
    BeginTextureMode(gizmo_target);
    
    ClearBackground(BLANK);
    

    {      
      BeginMode3D(camera);
      
      engine.app->draw_gizmo(delta);
      
      EndMode3D();
    }
    
    EndTextureMode();
    }


    // TODO: darray of targets to draw, then draw commands compositor

    int screenWidth = engine.app->window_size_x;
    int screenHeight = engine.app->window_size_y;

    BeginDrawing();
    ClearBackground(RAYWHITE);//GetColor(0x2c2c3cff)
    DrawTextureRec(target.texture,
                   (Rectangle){0, 0, screenWidth, -screenHeight},
                   (Vector2){0,0}, WHITE);
    if (engine.app->draw_gizmo)
    {
    DrawTextureRec(gizmo_target.texture,
                   (Rectangle){0, 0, screenWidth, -screenHeight},
                   (Vector2){0,0}, WHITE);
    }
    EndDrawing();
    
    // platform_swap_buffers();
  }
  
  engine.app->term();

  scene_system_term();
  component_system_term();
  
  mesh_system_clear();
  
  UnloadRenderTextureDepthTex(gizmo_target);
  UnloadRenderTextureDepthTex(target);
  
  RaylibCloseWindow();
  
  logger_term();
  
  return 0;
}
