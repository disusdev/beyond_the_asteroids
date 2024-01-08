#include "co_renderer.h"
#include <raylib.h>
#include <raymath.h>
#include <systems/mesh_system.h>
#include <systems/shaders.h>
#include <math.h>
#include "component_system.h"
#include <core/mem.h>
#include <rlgl.h>


static cvec(t_material_desc) mats_desc = 0;
static cvec(t_co_renderer_cfg) cfgs = 0;

void
co_renderer_set_model(t_co_renderer* renderer,
                      const char* model_path)
{
  Model m;
  i32 model_id = mesh_system_get_model(model_path, &m);
  if (model_id == -1)
  {
    m = LoadModel(model_path);
    model_id = mesh_system_model_upload(model_path, &m);
  }
  
  renderer->model_id = model_id;
  
  // for (int i = 0; i < m.meshCount; i++)
  // {
  //   BoundingBox bbox = GetMeshBoundingBox(m.meshes[i]);
  //   renderer->bbox.min = Vector3Min(renderer->bbox.min, bbox.min);
  //   renderer->bbox.max = Vector3Max(renderer->bbox.max, bbox.max);
  // }

  // renderer->m = m;

  // if (m.boneCount > 0)
  // {
  //   renderer->anim = LoadModelAnimations(model_path,
  //                                        &renderer->anim_count);
  // }

#if 0
  for (int i = 0; i < m.boneCount; i++)
  {
    Matrix trans = MatrixIdentity();
    
    trans = MatrixMultiply(trans, MatrixScale(m.bindPose[i].scale.x,
                                              m.bindPose[i].scale.y,
                                              m.bindPose[i].scale.z));
    
    Quaternion q = QuaternionFromEuler(m.bindPose[i].rotation.x * DEG2RAD,
                                       m.bindPose[i].rotation.y * DEG2RAD,
                                       m.bindPose[i].rotation.z * DEG2RAD);
    
    trans = MatrixMultiply(trans, QuaternionToMatrix(q));
    
    trans = MatrixMultiply(trans, MatrixTranslate(m.bindPose[i].translation.x,
                                                  m.bindPose[i].translation.y,
                                                  m.bindPose[i].translation.z));
  }
#endif
}


void
co_renderer_set_shader(t_co_renderer* renderer, Shader shader)
{
  u64 material_count = cvec_header(renderer->materials)->size;
  for (u64 i = 0; i < material_count; i++)
  {
    renderer->materials[i].shader = shader;
  }
}


void
co_renderer_update(t_co_renderer* renderer,
                   Matrix t)
{
  (void)renderer;
  (void)t;
}


void
co_renderer_draw(t_co_renderer* renderer)
{
#if 0
  for (int i = 0; i < renderer->m.boneCount; i++)
  {
    DrawCube(renderer->anim[renderer->anim_current].framePoses[renderer->anim_frame_counter][i].translation, 0.2f, 0.2f, 0.2f, RED);
  }
#endif
  if (!component_system_get_entities(0)[renderer->entity_id].enabled)
  {
    return;
  }

  Matrix transform = component_system_get_global_transform(renderer->entity_id);

  u64 mesh_count = cvec_header(renderer->meshes)->size;
  for (u64 i = 0; i < mesh_count; i++)
  {
    DrawMesh(renderer->meshes[i],
             renderer->materials[renderer->material_map[i]],
             transform);
  }
}


void
co_renderer_draw_wires(t_co_renderer* renderer, Color tint)
{
  Matrix transform = component_system_get_global_transform(renderer->entity_id);
  u64 mesh_count = cvec_header(renderer->meshes)->size;
  for (u64 i = 0; i < mesh_count; i++)
  {
    Material* material = &renderer->materials[renderer->material_map[i]];

    Color color = material->maps[MATERIAL_MAP_DIFFUSE].color;// m.materials[renderer->m.meshMaterial[i]].maps[MATERIAL_MAP_DIFFUSE].color;
    Color colorTint = WHITE;
    colorTint.r = (unsigned char)((((float)color.r/255.0f)*((float)tint.r/255.0f))*255.0f);
    colorTint.g = (unsigned char)((((float)color.g/255.0f)*((float)tint.g/255.0f))*255.0f);
    colorTint.b = (unsigned char)((((float)color.b/255.0f)*((float)tint.b/255.0f))*255.0f);
    colorTint.a = (unsigned char)((((float)color.a/255.0f)*((float)tint.a/255.0f))*255.0f);

    material->maps[MATERIAL_MAP_DIFFUSE].color = colorTint;
    DrawMesh(renderer->meshes[i], *material, transform);
    material->maps[MATERIAL_MAP_DIFFUSE].color = color;
  }
}


static void
DrawBoundingBoxPosition(Vector3 pos, BoundingBox box, Color color)
{
  Vector3 size = { 0 };
  
  size.x = fabsf(box.max.x - box.min.x);
  size.y = fabsf(box.max.y - box.min.y);
  size.z = fabsf(box.max.z - box.min.z);
  
  Vector3 center = { box.min.x + size.x/2.0f + pos.x, box.min.y + size.y/2.0f + pos.y, box.min.z + size.z/2.0f + + pos.z };
  
  DrawCubeWires(center, size.x, size.y, size.z, color);
}


void
co_renderer_draw_bbox(t_co_renderer* renderer)
{
  Matrix transform = component_system_get_global_transform(renderer->entity_id);
  Vector3 p = (Vector3){ transform.m12,
                         transform.m13,
                         transform.m14 };
  
  u64 mesh_count = cvec_header(renderer->meshes)->size;
  for (u64 i = 0; i < mesh_count; i++)
  {
    DrawBoundingBoxPosition(p, GetMeshBoundingBox(renderer->meshes[i]), GREEN);
  }
}


int
co_renderer_create(int entity_id, const char* model_path, bool is_static, Material* mat)
{
  t_co_renderer renderer = {0};
  Model cached_model;
  renderer.model_id = mesh_system_get_model(model_path, &cached_model);
  renderer.entity_id = entity_id;
  renderer.is_static = is_static;
  
  if (is_static)
  {
    //renderer.m = cached_model;
  }
  else
  {
    // @fix Hack, should make proper copy, or some instanced magic with animation!
    //renderer.m = LoadModel(model_path);
  }

  u64 meshes_size = sizeof(Mesh) * cached_model.meshCount;
  renderer.meshes = cvec_create(sizeof(Mesh), cached_model.meshCount, true);
  mem_copy(renderer.meshes, cached_model.meshes, meshes_size);

  u64 materials_size = sizeof(Material) * cached_model.materialCount;
  renderer.materials = cvec_create(sizeof(Material), cached_model.materialCount, true);
  mem_copy(renderer.materials, cached_model.materials, materials_size);

  if (mat)
  {
    for (u64 i = 0; i < materials_size; i++)
    {
      renderer.materials[i] = *mat;
    }
  }

  renderer.material_map = cvec_create(sizeof(i32), cached_model.meshCount, true);
  mem_copy(renderer.material_map, cached_model.meshMaterial, sizeof(i32) * cached_model.meshCount);

  return component_system_insert(entity_id, "co_renderer", &renderer);
}


void
co_renderer_set_texture(t_co_renderer* renderer, Texture2D texture)
{
  u64 mesh_count = cvec_header(renderer->meshes)->size;
  for (u64 i = 0; i < mesh_count; i++)
  {
    SetMaterialTexture(&renderer->materials[renderer->material_map[i]], MATERIAL_MAP_DIFFUSE, texture);
  }
}


// void
// co_renderer_set_texcoord(t_co_renderer* renderer, Vector2 offset, Vector2 scale)
// {
//   // int textureTilingLoc = GetShaderLocation(shader, "tiling");
//   u64 mesh_count = cvec_header(renderer->meshes)->size;
//   for (u64 i = 0; i < mesh_count; i++)
//   {
//     for (u64 j = 0; j < vertexCount; j++)
//     {
//       renderer->meshes[i].texcoords[j]
//     }
//   }
// }


int
co_renderer_create_by_cfg(int entity_id, int cfg_id)
{
  ctbl_insert(component_system_get_entities(0)[entity_id].components_cfgs, "co_renderer", (void*)(u64)cfg_id);

  if (cfgs[cfg_id].mat_desc_id != -1)
  {
    Material material = LoadMaterialDefault();
    t_material_desc* mat_desc = renderer_system_get_material_desc(cfgs[cfg_id].mat_desc_id);
    if (mat_desc->vs || mat_desc->fs)
    {
      Shader shader = LoadShaderFromMemory(shaders_get(mat_desc->vs), shaders_get(mat_desc->fs));
      material.shader = shader;
    }

    for (i32 i = 0; i < MATERIAL_MAP_COUNT; i++)
    {
      if (mat_desc->textures[i].path)
      {
        Texture2D tex = LoadTexture(mat_desc->textures[i].path);
        SetTextureFilter(tex, mat_desc->textures[i].filter);
        SetTextureWrap(tex, mat_desc->textures[i].wrap);
        material.maps[i].texture = tex;
      }
    }

    return co_renderer_create(entity_id, cfgs[cfg_id].model_path, cfgs[cfg_id].is_static, &material);
  }

  return co_renderer_create(entity_id, cfgs[cfg_id].model_path, cfgs[cfg_id].is_static, 0);
}


int
renderer_system_add_cfg(t_co_renderer_cfg cfg)
{
  int id = cvec_header(cfgs)->size;
  cfgs = cvec_push(cfgs, cast_ptr(u8) &cfg);
  return id;
}


t_co_renderer_cfg
renderer_system_get_cfg(int id)
{
  return cfgs[id];
}


int
renderer_system_add_material_desc(t_material_desc desc)
{
  int id = cvec_header(mats_desc)->size;
  mats_desc = cvec_push(mats_desc, cast_ptr(u8) & desc);
  return id;
}


t_material_desc*
renderer_system_get_material_desc(int id)
{
  return &mats_desc[id];
}


int
co_renderer_duplicate(int ent_id, int comp_id)
{
  t_co_renderer* renderers = component_system_get("co_renderer", 0);

  int new_comp_id = co_renderer_create(ent_id, mesh_system_get_src_path(renderers[comp_id].model_id), renderers[comp_id].is_static, 0);

  return new_comp_id;
}


int
renderer_system_create_component(int ent_id, int cfg_id)
{
  if (cfg_id == -1)
  {
    return -1;
  }
  return co_renderer_create_by_cfg(ent_id, cfg_id);
}


void
renderer_system_init()
{
  cfgs = cvec_create(sizeof(t_co_renderer_cfg), 16, false);
  mats_desc = cvec_create(sizeof(t_material_desc), 16, false);
  component_system_add_component("co_renderer",
                                 sizeof(t_co_renderer),
                                 1024,
                                 &co_renderer_duplicate,
                                 &renderer_system_create_component);
}


void renderer_system_draw_simple(Mesh mesh, Shader shader)
{
#if defined(GRAPHICS_API_OPENGL_33) || defined(GRAPHICS_API_OPENGL_ES2)
    rlEnableShader(shader.id);

    Matrix matModel = MatrixIdentity();
    Matrix matView = rlGetMatrixModelview();
    Matrix matModelView = MatrixIdentity();
    Matrix matProjection = rlGetMatrixProjection();

    if (shader.locs[SHADER_LOC_MATRIX_VIEW] != -1) rlSetUniformMatrix(shader.locs[SHADER_LOC_MATRIX_VIEW], matView);
    if (shader.locs[SHADER_LOC_MATRIX_PROJECTION] != -1) rlSetUniformMatrix(shader.locs[SHADER_LOC_MATRIX_PROJECTION], matProjection);

    if (shader.locs[SHADER_LOC_MATRIX_MODEL] != -1) rlSetUniformMatrix(shader.locs[SHADER_LOC_MATRIX_MODEL], matModel);

    matModel = MatrixMultiply(matModel, rlGetMatrixTransform());

    matModelView = MatrixMultiply(matModel, matView);

    if (shader.locs[SHADER_LOC_MATRIX_NORMAL] != -1) rlSetUniformMatrix(shader.locs[SHADER_LOC_MATRIX_NORMAL], MatrixTranspose(MatrixInvert(matModel)));

    if (!rlEnableVertexArray(mesh.vaoId))
    {
        rlEnableVertexBuffer(mesh.vboId[0]);
        rlSetVertexAttribute(shader.locs[SHADER_LOC_VERTEX_POSITION], 3, RL_FLOAT, 0, 0, 0);
        rlEnableVertexAttribute(shader.locs[SHADER_LOC_VERTEX_POSITION]);

        if (mesh.indices != 0) rlEnableVertexBufferElement(mesh.vboId[6]);
    }

    if (mesh.vboId[3] == 0) rlDisableVertexAttribute(shader.locs[SHADER_LOC_VERTEX_COLOR]);

    int eyeCount = 1;
    if (rlIsStereoRenderEnabled()) eyeCount = 2;

    for (int eye = 0; eye < eyeCount; eye++)
    {
        Matrix matModelViewProjection = MatrixIdentity();
        if (eyeCount == 1) matModelViewProjection = MatrixMultiply(matModelView, matProjection);
        else
        {
            rlViewport(eye*rlGetFramebufferWidth()/2, 0, rlGetFramebufferWidth()/2, rlGetFramebufferHeight());
            matModelViewProjection = MatrixMultiply(MatrixMultiply(matModelView, rlGetMatrixViewOffsetStereo(eye)), rlGetMatrixProjectionStereo(eye));
        }

        rlSetUniformMatrix(shader.locs[SHADER_LOC_MATRIX_MVP], matModelViewProjection);

        if (mesh.indices != 0) rlDrawVertexArrayElements(0, mesh.triangleCount*3, 0);
        else rlDrawVertexArray(0, mesh.vertexCount);
    }

    rlDisableVertexArray();
    rlDisableVertexBuffer();
    rlDisableVertexBufferElement();

    rlDisableShader();

    rlSetMatrixModelview(matView);
    rlSetMatrixProjection(matProjection);
#endif
}


void
co_renderer_set_model_by_cfg(t_co_renderer* renderer,
                             i32 cfg_id)
{
  Model cached_model;
  renderer->model_id = mesh_system_get_model(cfgs[cfg_id].model_path, &cached_model);

  u64 meshes_size = sizeof(Mesh) * cached_model.meshCount;
  cvec_destroy(renderer->meshes);
  // cvec_clear(renderer->meshes);
  renderer->meshes = cvec_create(sizeof(Mesh), cached_model.meshCount, true);
  mem_copy(renderer->meshes, cached_model.meshes, meshes_size);

  u64 materials_size = sizeof(Material) * cached_model.materialCount;
  cvec_destroy(renderer->materials);
  // cvec_clear(renderer->materials);
  renderer->materials = cvec_create(sizeof(Material), cached_model.materialCount, true);
  mem_copy(renderer->materials, cached_model.materials, materials_size);

  cvec_destroy(renderer->material_map);
  // cvec_clear(renderer->material_map);
  renderer->material_map = cvec_create(sizeof(i32), cached_model.meshCount, true);
  mem_copy(renderer->material_map, cached_model.meshMaterial, sizeof(i32) * cached_model.meshCount);
}                             
