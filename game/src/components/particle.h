#ifndef __PARTICLE_H__
#define __PARTICLE_H__

#include <defines.h>

#define PARTICLE_COUNT 192

typedef struct
{
  Vector3 position;
  Color color;
  float alpha;
  float size;
  b32 enable;
} t_particle;

typedef struct
{
  i32 entity_id;
  t_particle particles[PARTICLE_COUNT];
} t_co_particle_system;

static Texture2D particle_texture;

i32
co_particle_system_create(i32 entity_id, i32 cfg_id)
{
  t_co_particle_system particle_system = {0};
  particle_system.entity_id = entity_id;
  
  for (u64 i = 0; i < PARTICLE_COUNT; i++)
  {
    particle_system.particles[i].color = YELLOW;
    particle_system.particles[i].alpha = 1.0f;
    particle_system.particles[i].size = 0.15f;
    particle_system.particles[i].enable = false;
  }
  
  return component_system_insert(entity_id, "co_particle_system", &particle_system);
}


void
particle_system_init()
{
  particle_texture = LoadTexture("data/sprites/particle.png");
  component_system_add_component("co_particle_system",
                                 sizeof(t_co_particle_system),
                                 4, 0, &co_particle_system_create);
}


void
particle_system_update(f32 dt)
{
  u64 count = 0;
  t_co_particle_system* particles = component_system_get("co_particle_system", &count);
  for (u64 i = 0; i < count; i++)
  {
    Matrix transform = component_system_get_global_transform(particles[i].entity_id);
    Vector3 pos = extract_position(&transform);
    
    for (u64 j = 0; j < PARTICLE_COUNT; j++)
    {
      if (!particles[i].particles[j].enable)
      {
        particles[i].particles[j].enable = true;
        particles[i].particles[j].alpha = 1.0f;
        particles[i].particles[j].size = 0.15f;
        particles[i].particles[j].position = pos;
        j = PARTICLE_COUNT;
      }
    }
  }
  
  for (u64 i = 0; i < count; i++)
  {
    for (u64 j = 0; j < PARTICLE_COUNT; j++)
    {
      particles[i].particles[j].alpha -= dt * 2.0f;
      particles[i].particles[j].size -= dt * 0.15f;
      if (particles[i].particles[j].alpha <= 0)
      {
        particles[i].particles[j].enable = false;
      }
    }
  }
}


void
particle_system_draw()
{
  BeginBlendMode(BLEND_ALPHA);
  
  u64 count = 0;
  t_co_particle_system* particles = component_system_get("co_particle_system", &count);
  for (u64 i = 0; i < count; i++)
  {
    for (u64 j = 0; j < PARTICLE_COUNT; j++)
    {
      if (!particles[i].particles[j].enable) continue;
      
      Vector2 screen_pos = GetWorldToScreen(particles[i].particles[j].position, camera_system_get_camera());
      
      DrawTexturePro(particle_texture, (Rectangle){ 0.0f, 0.0f,
                                                    (float)particle_texture.width,
                                                    (float)particle_texture.height },
                                       (Rectangle){ screen_pos.x,
                                                    screen_pos.y,
                                                    particle_texture.width*particles[i].particles[j].size,
                                                    particle_texture.height*particles[i].particles[j].size },
                                       (Vector2){ (float)(particle_texture.width*particles[i].particles[j].size/2.0f),
                                                  (float)(particle_texture.height*particles[i].particles[j].size/2.0f) },
                                       0, Fade(particles[i].particles[j].color, particles[i].particles[j].alpha));
    }
  }
  
  EndBlendMode();
}


#endif