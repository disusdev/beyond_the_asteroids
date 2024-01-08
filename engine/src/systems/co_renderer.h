#ifndef CO_RENDERER_H
#define CO_RENDERER_H

#include <defines.h>
#include <raylib.h>
#include <containers/cvec.h>
#include <containers/ctbl.h>

#define MATERIAL_MAP_COUNT 11

typedef struct
{
  const char* path;
  TextureFilter filter;
  TextureWrap wrap;
} t_material_texture;


typedef struct
{
  int id;
  const char* vs;
  const char* fs;
  t_material_texture textures[MATERIAL_MAP_COUNT];
} t_material_desc;


typedef struct
{
  int id;
  char model_path[128];
  bool is_static;
  int mat_desc_id;
} t_co_renderer_cfg;


typedef struct
{
  // TODO: could be a pointer to the model, or mesh
  // Model m;

  cvec(Mesh) meshes;
  cvec(Material) materials;
  cvec(i32) material_map;

  i32 model_id;
  i32 entity_id;
  bool is_static;

  // TODO: it is only used in asset selector scene,
  //       should be moved
  // BoundingBox bbox;
  
  Vector2 tiling;

} t_co_renderer;


int
renderer_system_add_cfg(t_co_renderer_cfg cfg);


t_co_renderer_cfg
renderer_system_get_cfg(int id);


void
co_renderer_set_texture(t_co_renderer* renderer, Texture2D texture);


int
renderer_system_add_material_desc(t_material_desc desc);


t_material_desc*
renderer_system_get_material_desc(int id);


void
co_renderer_set_model(t_co_renderer* renderer,
                      const char* model_path);


void
co_renderer_set_model_by_cfg(t_co_renderer* renderer,
                             i32 cfg_id);


void
co_renderer_set_shader(t_co_renderer* renderer,
                       Shader shader);


void
co_renderer_update(t_co_renderer* renderer,
                   Matrix t);


void
co_renderer_draw(t_co_renderer* renderer);


void
co_renderer_draw_wires(t_co_renderer* renderer, Color tint);


void
co_renderer_draw_bbox(t_co_renderer* renderer);


int
co_renderer_create(int entity_id, const char* model_path, bool is_static, Material* mat);


int
co_renderer_create_by_cfg(int entity_id, int cfg_id);


int
co_renderer_duplicate(int ent_id, int comp_id);


void
renderer_system_init();


int
renderer_system_create_component(int ent_id, int cfg_id);


void renderer_system_draw_simple(Mesh mesh, Shader shader);


#endif