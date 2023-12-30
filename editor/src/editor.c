#include <entry.h>
#include <raylib.h>
#include <rlgl.h>

#include <systems/component_system.h>
#include <systems/mesh_system.h>
#include <systems/scene_system.h>

#include <systems/shaders.h>

#include <raygui.h>

#include <raymath.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static int WINDOW_WIDTH = 1920;
static int WINDOW_HEIGHT = 1080;
#define RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT 24

//static Material default_cube_material;
//static Texture2D default_cube_texture;

static Mesh grid_mesh;
static Shader grid_shader;

static cvec(int) selected_entities = 0;
// int *selected_entities;

Font code_font;

typedef struct
{
  Vector3 pos;
  Vector3 rot;
  Vector3 scl;
} regular_transform;


//static int
//load_model_as_entity(int parent_id,
//                     const char* model_path,
//                     bool use_raycast_placing)
//{
//  Camera camera = camera_system_get_camera();
//
//  RayCollision coll = {0};
//  Plane plane;
//  plane.position = (Vector3){0,0,0};
//  plane.normal = (Vector3){0,-1,0};
//  if (use_raycast_placing)
//  {
//    Ray ray = GetMouseRay(GetMousePosition(), camera);
//    coll = IntersectRayPlane(ray, plane);
//  }
//  
//  if (coll.hit == false)
//  {
//    coll.point = (Vector3){ 0,0,0 };
//    coll.normal = (Vector3){ 0,1,0 };
//  }
//  
//  int ent_id = -1;
//  // t_entity* ent = 0;
//
//  ent_id = component_system_create_entity(parent_id);
//
//  Matrix transform = MatrixTranslate(coll.point.x, coll.point.y, coll.point.z);
//  component_system_set_local_transform(ent_id, transform);
//
//  co_renderer_create(ent_id, model_path, false, 0);
//  co_animation_create(ent_id);
//  
//  return ent_id;
//}


//static int
//model_import(int parent_id,
//             const char* model_path)
//{
//  return load_model_as_entity(parent_id,
//                              model_path,
//                              false);
//}


// static void
// load_all_dungeon_models(const char* path_to_models)
// {
//   // load models and create prefabs from them?
//   FilePathList glb_files = LoadDirectoryFilesEx(path_to_models, ".glb", true);

//   //for (u64 i = 0; i < glb_files.count; i++)
//   for (u64 i = 0; i < glb_files.count; i++)
//   {
//     printf("[.glb] %s\n", glb_files.paths[i]);

//     if (strstr(glb_files.paths[i], "floor_tile_small") != 0)
//     {
//       model_import(-1, glb_files.paths[i]);
//     }

//     // t_prefab* prefab = prefab_from_entity(scene_get_entity(scene_get_current(), ent_id));
//     // char prefab_path[128];
//     // sprintf(prefab_path, "data/prefabs/%s.prefab", GetFileNameWithoutExt(glb_files.paths[i]));
//     // prefab_save(prefab_path, prefab);
//   }
// }


// static int asset_scene = -1;


cvec(regular_transform) selected_entities_props;
// regular_transform* selected_entities_props;

#define STYLE_PROPS_COUNT 22

static const GuiStyleProp styleProps[STYLE_PROPS_COUNT] =
{
  { 0, 0, 0x878787ff },    // DEFAULT_BORDER_COLOR_NORMAL 
  { 0, 1, 0x2c2c2cff },    // DEFAULT_BASE_COLOR_NORMAL 
  { 0, 2, 0xc3c3c3ff },    // DEFAULT_TEXT_COLOR_NORMAL 
  { 0, 3, 0xe1e1e1ff },    // DEFAULT_BORDER_COLOR_FOCUSED 
  { 0, 4, 0x848484ff },    // DEFAULT_BASE_COLOR_FOCUSED 
  { 0, 5, 0x181818ff },    // DEFAULT_TEXT_COLOR_FOCUSED 
  { 0, 6, 0x000000ff },    // DEFAULT_BORDER_COLOR_PRESSED 
  { 0, 7, 0xefefefff },    // DEFAULT_BASE_COLOR_PRESSED 
  { 0, 8, 0x202020ff },    // DEFAULT_TEXT_COLOR_PRESSED 
  { 0, 9, 0x6a6a6aff },    // DEFAULT_BORDER_COLOR_DISABLED 
  { 0, 10, 0x818181ff },    // DEFAULT_BASE_COLOR_DISABLED 
  { 0, 11, 0x606060ff },    // DEFAULT_TEXT_COLOR_DISABLED 
  { 0, 16, 0x00000018 },    // DEFAULT_TEXT_SIZE 
  { 0, 17, 0x00000000 },    // DEFAULT_TEXT_SPACING 
  { 0, 18, 0x9d9d9dff },    // DEFAULT_LINE_COLOR 
  { 0, 19, 0x3c3c3cff },    // DEFAULT_BACKGROUND_COLOR 
  { 1, 5, 0xf7f7f7ff },    // LABEL_TEXT_COLOR_FOCUSED 
  { 1, 8, 0x898989ff },    // LABEL_TEXT_COLOR_PRESSED 
  { 4, 5, 0xb0b0b0ff },    // SLIDER_TEXT_COLOR_FOCUSED 
  { 5, 5, 0x848484ff },    // PROGRESSBAR_TEXT_COLOR_FOCUSED 
  { 9, 5, 0xf5f5f5ff },    // TEXTBOX_TEXT_COLOR_FOCUSED 
  { 10, 5, 0xf6f6f6ff },    // VALUEBOX_TEXT_COLOR_FOCUSED 
};


Vector3 start_pos = {0};
Vector3 old_pos = {0};
bool drag_mode = false;
int last_selected_axis = 0;
Plane plane[3];


static int editor_mode = 1;

// make vector of selected entities
// static int selected_entity = -1;
static bool on_panel = false;
// static char str_buffer[4096];

Vector3 props_pos = { 0 };
Vector3 props_rot = { 0 };
Vector3 props_scl = { 0 };


Vector3 props_pos_start = { 0 };
Vector3 props_rot_start = { 0 };
Vector3 props_scl_start = { 0 };


Vector3 props_pos_offset = { 0 };
Vector3 props_rot_offset = { 0 };
Vector3 props_scl_offset = { 0 };

static bool sectionActive = false;
static char* sectionNames = "Transform";

#define VALUE_TEXT_BUFFER_SIZE 20
#define BOX_COUNT 16
bool editValueBox[BOX_COUNT] = { 0 };
char valTextBox[BOX_COUNT][VALUE_TEXT_BUFFER_SIZE] = { 0 };

float xx;
float yy;
float starty = 0.0f, endy = 1.0f;

bool drop_down_active = false;
bool is_assets_loaded = false;
const char* PATH_TO_ASSETS = "data/models/dungeon/gltf";
const int ELEMENTS_IN_ROW = 10;


static void
update_props_entity(int entity_id, Vector3 pos, Vector3 rot, Vector3 scl)
{
  Matrix transform = MatrixIdentity();
    
  transform = MatrixMultiply(transform, MatrixScale(scl.x,
                                    scl.y,
                                    scl.z));
    
  Quaternion q = QuaternionFromEuler(rot.z * DEG2RAD,
                                     rot.y * DEG2RAD,
                                     rot.x * DEG2RAD);
    
  transform = MatrixMultiply(transform, QuaternionToMatrix(q));

  transform = MatrixMultiply(transform, MatrixTranslate(pos.x,
                                        pos.y,
                                        pos.z));
  
  component_system_set_local_transform(entity_id, transform);
}


// void
// asset_picker_open()
// {
//   scene_set_current(asset_scene);

//   // load all assets and position them by their bbox w and h
//   if (!is_assets_loaded)
//   {
//     FilePathList glb_files = LoadDirectoryFilesEx(PATH_TO_ASSETS, ".glb", true);
//     for (u64 i = 0; i < glb_files.count; i++)
//     {
//       printf("[.glb] %s\n", glb_files.paths[i]);
//       int ent_id = model_import(-1, glb_files.paths[i]);
//       t_scene* scene = scene_get(asset_scene);
//       Matrix* transform = entity_get_transform(asset_scene, scene_get_entity(scene, ent_id));
//       //*transform = MatrixTranslate((i % ELEMENTS_IN_ROW), 0, (i / ELEMENTS_IN_ROW));
//       *transform = MatrixTranslate((i % ELEMENTS_IN_ROW) * 8, 0, (i / ELEMENTS_IN_ROW) * 8);
//     }
//     is_assets_loaded = true;
//   }
  
//   // TODO: camera got bug when pick asset second time!
//   //       probably up vector left as < 0, 0, -1 >
//   t_co_camera* editor_camera = scene_get_current_editor_camera();
//   editor_camera->c.position = (Vector3){ 0.0f, 6.0f, 5.0f };
//   editor_camera->c.up = (Vector3){ 0.0f, 0.0f, -1.0f };
//   editor_camera->c.target = (Vector3){ 0.0f, 0.0f, 0.0f };
  
//   editor_mode = 2;
//   DisableCursor();
// }


// void
// asset_picker_close()
// {
//   scene_set_current(0);
//   editor_mode = 1;
//   EnableCursor();
// }


// void
// asset_picker_update()
// {
//   if (!asset_pick_camera)
//   {
//     outline_renderer_id = -1;
//     return;
//   }
  
//   t_scene* scene = scene_get_current();
  
//   RayCollision collision = { 0 };
//   collision.distance = FLT_MAX;
//   collision.hit = false;
  
//   t_co_camera* editor_camera = scene_get_current_editor_camera();
  
//   Vector3 pos = editor_camera->c.position;
//   Vector3 dir = Vector3Normalize(Vector3Subtract(editor_camera->c.target,
//                                                  editor_camera->c.position));
  
//   Ray ray = { pos, dir };
  
//   int entities_count = scene->entities->size;
//   for (int i = 0; i < entities_count; i++)
//   {
//     t_entity* p_ent = scene_get_entity(scene, i);
//     if (p_ent->id == -1) continue;
//     if (p_ent->component_flags & CO_TRANSFORM)
//     {
//       Matrix global_transform = entity_global_mtx(scene->id, p_ent->id);

//       if (p_ent->component_flags & CO_RENDERER)
//       {
//         int id = p_ent->renderer_id;
//         t_co_renderer* renderer = scene_get_renderer(scene, id);
        
//         //Vector3 p = (Vector3){ global_transform.m12,
//         //  global_transform.m13,
//         //  global_transform.m14 };
        
//         BoundingBox real_bbox = renderer->bbox;
        
//         BoundingBox bbox = { 0 };
        
//         bbox.min = (Vector3){-4.0f,-4.0f,-4.0f};
//         bbox.max = (Vector3){4.0f,4.0f,4.0f};
        
//         bbox.min = Vector3Transform(bbox.min, global_transform);// Vector3Add(bbox.min, p);
//         bbox.max = Vector3Transform(bbox.max, global_transform);// Vector3Add(bbox.max, p);

//         real_bbox.min = Vector3Transform(real_bbox.min, global_transform);// Vector3Add(bbox.min, p);
//         real_bbox.max = Vector3Transform(real_bbox.max, global_transform);// Vector3Add(bbox.max, p);
        
//         RayCollision coll = GetRayCollisionBox(ray, bbox);
        
//         if (coll.hit)
//         {
//           collision = coll;
//           outline_renderer_id = id;
//           outline_entity_id = i;
//         }
//       }
//     }
//   }
  
//   if (!collision.hit)
//   {
//     outline_renderer_id = -1;
//   }

//   if (IsMouseButtonPressed(0) && outline_renderer_id != -1)
//   {
//     // TODO: spawn entity in real scene
//     //       with selected "Prefab" (for now it is Mesh)
    
//     // TODO: store previous scene to refer
//     entity_copy_asset(0, -1, outline_entity_id, asset_scene);
    
//     asset_pick_camera = false;
//     outline_renderer_id = -1;
//     asset_picker_close();
    
//     return;
//   }
// }


static b8
gui_float(int id,
          float x, float y, float width,
          float font_size, f32* ptr_value)
{
#if defined(__APPLE__)
  if (!editValueBox[id]) gcvt(*ptr_value, 6, valTextBox[id]);
#else
  if (!editValueBox[id]) _gcvt(*ptr_value, 6, valTextBox[id]);
#endif
  if (GuiTextBox((Rectangle){ x, y, width, font_size }, valTextBox[id], VALUE_TEXT_BUFFER_SIZE, editValueBox[id]))
  {
    editValueBox[id] = !editValueBox[id];
    if (!editValueBox[id])
    {
      char *endPtr = NULL;
      double value = strtod((char *)valTextBox[id], &endPtr);
      if (endPtr != (char *)valTextBox[id])
      {
        *ptr_value = value;
        return true;
      }
    }
  }
  return false;
}


static b8
gui_vec3(int id, char* name,
         Rectangle content_rect,
         Vector2 scroll_offset,
         float margin,
         float font_size,
         Vector3* ptr_value)
{
  GuiLabel((Rectangle){ content_rect.x, content_rect.y + content_rect.height + scroll_offset.y, content_rect.width, font_size }, name);
  content_rect.height += font_size;
  return gui_float(id + 0,
                   content_rect.x,
                   content_rect.y + content_rect.height + scroll_offset.y,
                   content_rect.width / 3 - margin,
                   1.5f *font_size,
                   &ptr_value->x) ||
         gui_float(id + 1,
                   content_rect.x + content_rect.width / 3,
                   content_rect.y + content_rect.height + scroll_offset.y,
                   content_rect.width / 3,
                   1.5f *font_size,
                   &ptr_value->y) ||
         gui_float(id + 2,
                   content_rect.x + content_rect.width / 3 * 2 + margin,
                   content_rect.y + content_rect.height + scroll_offset.y,
                   content_rect.width / 3,
                   1.5f *font_size,
                   &ptr_value->z);
}


static void
load_props(int entity_id, Vector3* pos, Vector3* rot, Vector3* scl)
{
  Matrix t = component_system_get_local_transform(entity_id);
  
  pos->x = t.m12;
  pos->y = t.m13;
  pos->z = t.m14;
  t.m12 = t.m13 = t.m14 = 0;
  Vector3 s = (Vector3){ Vector3Length((Vector3){t.m0, t.m1, t.m2}),
                         Vector3Length((Vector3){t.m4, t.m5, t.m6}),
                         Vector3Length((Vector3){t.m8, t.m9, t.m10}) };
  scl->x = s.x;
  scl->y = s.y;
  scl->z = s.z;
  t.m0 /= s.x; t.m4 /= s.x; t.m8 /= s.x;
  t.m1 /= s.y; t.m5 /= s.y; t.m9 /= s.y;
  t.m2 /= s.z; t.m6 /= s.z; t.m10 /= s.z;
  Quaternion q =  QuaternionFromMatrix(t);
  Vector3 e = QuaternionToEuler(q);
  e = Vector3Scale(e, RAD2DEG);
  rot->x = e.z;
  rot->y = e.y;
  rot->z = e.x;
}


bool
find_parent(int idx)
{
  u64 selected_entities_count = cvec_header(selected_entities)->size;
  for(u64 i = 0; i < selected_entities_count; i++)
  {
    if (i == (u64) idx) continue;
    if (component_system_parent(selected_entities[idx]) == selected_entities[i])
    {
      return true;
    }
  }
  return false;
}


void
remove_entities()
{
  u64 selected_entities_count = cvec_header(selected_entities)->size;
  for(u64 i = 0; i < selected_entities_count; i++)
  {
    if (find_parent(i))
    {
      continue;
    }
    component_system_remove_entity(selected_entities[i]);
  }
  cvec_clear(selected_entities);
}


void
update_selected_props()
{
  u64 selected_entities_count = cvec_header(selected_entities)->size;
  for (u64 i = 0; i < selected_entities_count; i++)
  {
    regular_transform r_transform = {0};
    load_props(selected_entities[i], &r_transform.pos, &r_transform.rot, &r_transform.scl);
    selected_entities_props[i] = r_transform;
  }
}


void
update_props_entities(bool set_as_is)
{
  props_pos_offset = Vector3Subtract(props_pos, props_pos_start);
  props_rot_offset = Vector3Subtract(props_rot, props_rot_start);
  props_scl_offset = Vector3Subtract(props_scl, props_scl_start);

  u64 selected_entities_count = cvec_header(selected_entities)->size;
  for(u64 i = 0; i < selected_entities_count; i++)
  {
    if (find_parent(i))
    {
      continue;
    }
    Vector3 dst_pos = selected_entities_props[i].pos;
    Vector3 dst_rot = selected_entities_props[i].rot;
    Vector3 dst_scl = selected_entities_props[i].scl;
    if (set_as_is)
    {
      dst_pos = props_pos;
      dst_rot = props_rot;
      dst_scl = props_scl;
    }
    else
    {
      dst_pos = Vector3Add(dst_pos, props_pos_offset);
      dst_rot = Vector3Add(dst_rot, props_rot_offset);
      dst_scl = Vector3Add(dst_scl, props_scl_offset);
    }
    update_props_entity(selected_entities[i], dst_pos, dst_rot, dst_scl);
  }
}


static char writable_model_name[128] = {0};
static bool textBoxEditMode = false;


static void
entity_panel_open(int entity_id)
{
  load_props(entity_id, &props_pos, &props_rot, &props_scl);
  props_pos_start = props_pos;
  props_rot_start = props_rot;
  props_scl_start = props_scl;

  // load_props(entity_id, &props_pos, &props_rot, &props_scl);
  // selected_entities_props = cvec_push(selected_entities_props, &r_transform);

  t_entity* entities = cast_ptr(t_entity) component_system_get_entities(0);
  t_co_renderer* renderer = cast_ptr(t_co_renderer) component_system_at("co_renderer", (u64) ctbl_get(entities[entity_id].components, "co_renderer"));

  if (renderer != 0)
  {
    const char* model_name = GetFileNameWithoutExt(mesh_system_get_src_path(renderer->model_id));
    memcpy(writable_model_name, model_name, 128);
  }
}


const char* MODELS_PATH = "data/models/dungeon/gltf";

static const char*
fs_find_model_by_name(const char* model_name)
{
  FilePathList glb_files = LoadDirectoryFilesEx(MODELS_PATH, ".glb", true);

  for (u64 i = 0; i < glb_files.count; i++)
  {
    if (strcmp(GetFileNameWithoutExt(glb_files.paths[i]), model_name) == 0)
    {
      printf("[.glb] %s\n", glb_files.paths[i]);
      return glb_files.paths[i];
    }
  }

  return 0;
}


static void
draw_entity_panel(int entity_id)
{
  u64 selected_entities_count = cvec_header(selected_entities)->size;
  if (selected_entities_count == 0) return;

  char ent_name[128];
  sprintf(ent_name, "entity_%d", entity_id);
  DrawTextEx(code_font, ent_name, (Vector2){ 20.0f, 20.0f }, (float)code_font.baseSize, 2, BLACK);
  
  // DrawText(ent_name, 20, 20, 20, BLACK);

  /* sprintf(ent_name, "xx: [%f]", xx); */
  /* DrawText(ent_name, 20, 40, 20, BLACK); */

  float fontSize = GuiGetStyle(DEFAULT, TEXT_SIZE);
  const float margin = 12;
  Vector2 scrollOffset = (Vector2){ 0, 0 };
  Rectangle contentRect = (Rectangle){ 0, 0, 0, 0 };
  Rectangle settingsRect = (Rectangle){ WINDOW_WIDTH - WINDOW_WIDTH/4, 0, WINDOW_WIDTH/4, WINDOW_HEIGHT };

  on_panel = CheckCollisionPointRec(GetMousePosition(), settingsRect);

  GuiScrollPanel(settingsRect, 0, contentRect, &scrollOffset, NULL);

  BeginScissorMode(settingsRect.x, settingsRect.y+RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT, settingsRect.width, settingsRect.height);
  {
    contentRect = (Rectangle){ settingsRect.x + margin, RAYGUI_WINDOWBOX_STATUSBAR_HEIGHT+margin, settingsRect.width - 2*margin - GuiGetStyle(LISTVIEW, SCROLLBAR_WIDTH), 0 };

    // Help button
    // if (GuiButton((Rectangle){ contentRect.x, contentRect.y + contentRect.height + scrollOffset.y, contentRect.width, 1.5*fontSize }, GuiIconText(showHelp? ICON_EYE_ON : ICON_EYE_OFF, "Curve controls help"))) showHelp = !showHelp;

    // contentRect.height += 1.5*fontSize + margin;

    // float dropdown_height = contentRect.height;

    t_entity* entities = cast_ptr(t_entity) component_system_get_entities(0);
    u64 renderer_id = (u64) ctbl_get(entities[entity_id].components, "co_renderer");
    t_co_renderer* renderer = cast_ptr(t_co_renderer) component_system_at("co_renderer", renderer_id);

    if (renderer != 0)
    {
      // str_buffer[0] = '\0';
      // for (int i = 0; i < renderer->anim_count; i++)
      // {
      //   if (i != 0) strncat(str_buffer, ";", 1);
      //   strncat(str_buffer, renderer->anim[i].name, strlen(renderer->anim[i].name));
      // }

      // contentRect.height += 1.5 * fontSize + margin;

      if (GuiTextBox((Rectangle) { contentRect.x, contentRect.y + contentRect.height + scrollOffset.y, contentRect.width, fontSize * 2 },
          writable_model_name, 128, textBoxEditMode))
      {
        textBoxEditMode = !textBoxEditMode;

        if (!textBoxEditMode)
        {
          const char* model_path = fs_find_model_by_name(writable_model_name);
          if (!model_path)
          {
            const char* model_name = GetFileNameWithoutExt(mesh_system_get_src_path(renderer->model_id));
            memcpy(writable_model_name, model_name, 128);
          }
          else
          {
            // renderer_set_model(renderer,
            //                    model_path);
          }
        }
      }

      contentRect.height += 1.5 * fontSize + margin;


      // if (renderer->anim)
      // {
      //   renderer->anim_frame_time = MIN(renderer->anim_frame_time, renderer->anim[renderer->anim_current].frameCount);

      //   GuiSlider((Rectangle) { contentRect.x, contentRect.y + contentRect.height + scrollOffset.y, contentRect.width / 2, fontSize }, NULL, TextFormat("Animation Time: %.2fs", renderer->anim_frame_time), & renderer->anim_frame_time, 1, renderer->anim[renderer->anim_current].frameCount);
      //   contentRect.height += fontSize + margin;

      //   renderer->anim_frame_counter = (int)renderer->anim_frame_time;

      //   if (renderer->anim_frame_counter >= renderer->anim[renderer->anim_current].frameCount) renderer->anim_frame_counter = 0;
      // }
    }


    {
      Rectangle headerRect = (Rectangle){ contentRect.x, contentRect.y + contentRect.height+scrollOffset.y, contentRect.width, 1.5f*fontSize };
      GuiStatusBar(headerRect, NULL);

      if (GuiLabelButton(headerRect, GuiIconText(sectionActive ? ICON_ARROW_DOWN_FILL : ICON_ARROW_RIGHT_FILL, sectionNames))) sectionActive = !sectionActive;

      contentRect.height += 1.5f*fontSize + margin;

      if (sectionActive)
      {
        b8 update_transform = gui_vec3(0, "Position",
                                       contentRect,
                                       scrollOffset,
                                       margin,
                                       fontSize,
                                       &props_pos);
        contentRect.height += 1.5f * fontSize + margin + fontSize;
        update_transform |= gui_vec3(3, "Rotation",
                                     contentRect,
                                     scrollOffset,
                                     margin,
                                     fontSize,
                                     &props_rot);
        contentRect.height += 1.5f * fontSize + margin + fontSize;
        update_transform |= gui_vec3(6, "Scale",
                                     contentRect,
                                     scrollOffset,
                                     margin,
                                     fontSize,
                                     &props_scl);
        contentRect.height += 1.5f * fontSize + margin + fontSize;

        if (update_transform)
        {
          update_props_entities(true);
          update_selected_props();
        }
      }
    }


    contentRect.height += margin;

    // if (renderer != 0 && renderer->anim)
    // {
    //   if (GuiDropdownBox((Rectangle) { contentRect.x + margin, contentRect.y + dropdown_height + scrollOffset.y, contentRect.width - margin * 2, fontSize }, str_buffer, & renderer->anim_current, drop_down_active))
    //   {
    //     drop_down_active = !drop_down_active;
    //   }
    // }
  }
  EndScissorMode();

  DrawRectangleGradientH(settingsRect.x - 12, 0, 12, settingsRect.height, BLANK, (Color){ 0, 0, 0, 100 });
}


void
draw_gizmo()
{
  RayCollision collision = { 0 };
  collision.distance = FLT_MAX;
  collision.hit = false;
  
  Camera camera = camera_system_get_camera();
  
  Ray ray = GetMouseRay(GetMousePosition(), camera);
  
  bool gizmo_hit = drag_mode;
  int selected_axis = last_selected_axis;
  
  u64 selected_entities_count = cvec_header(selected_entities)->size;
  if (selected_entities_count > 0)
  {
    Matrix t = component_system_get_global_transform(selected_entities[0]);
    
    Vector3 pos = (Vector3){t.m12, t.m13, t.m14};
    float dist = Vector3Length(Vector3Subtract(pos, ray.position));
    
    float scale = 0.04f * (dist + 1);
    
    float radius = 0.2f * scale;
    int slices = 16;
    float length = 5.0f * scale;
    
    Vector3 min = Vector3Zero();
    Vector3 max = Vector3Zero();
    BoundingBox bbox = {0};
    float distance[3] = {FLT_MAX, FLT_MAX, FLT_MAX};
    
    min.x = -(radius * 2);
    min.y = -(radius * 2);
    min.z = - length - (length/4);
    max.x = (radius * 2);
    max.y = (radius * 2);
    max.z = (radius * 2);

    min = Vector3RotateByQuaternion(min, QuaternionFromMatrix(t));
    max = Vector3RotateByQuaternion(max, QuaternionFromMatrix(t));

    bbox.min = Vector3Add(pos, min);
    bbox.max = Vector3Add(pos, max);

    
    RayCollision coll = GetRayCollisionBox(ray, bbox);
    if (coll.hit)
    {
      distance[0] = coll.distance;
      plane[0].position = coll.point;
      plane[0].normal = (Vector3){1,0,0};
      if (!IntersectRayPlane(ray, plane[0]).hit)
      {
        plane[0].normal = (Vector3){-1,0,0};
      }
      coll.hit = false;
      gizmo_hit = true;
    }
    
    min.x = -(radius * 2);
    min.y = -(radius * 2);
    min.z = -(radius * 2);
    max.x = (radius * 2);
    max.y = length + (length/4);
    max.z = (radius * 2);

    min = Vector3RotateByQuaternion(min, QuaternionFromMatrix(t));
    max = Vector3RotateByQuaternion(max, QuaternionFromMatrix(t));

    bbox.min = Vector3Add(pos, min);
    bbox.max = Vector3Add(pos, max);

    
    coll = GetRayCollisionBox(ray, bbox);
    if (coll.hit)
    {
      distance[1] = coll.distance;
      plane[1].position = coll.point;
      plane[1].normal = (Vector3){1,0,0};
      if (!IntersectRayPlane(ray, plane[1]).hit)
      {
        plane[1].normal = (Vector3){-1,0,0};
      }
      coll.hit = false;
      gizmo_hit = true;
    }
    
    min.x = -(radius * 2);
    min.y = -(radius * 2);
    min.z = -(radius * 2);
    max.x = length + (length/4);
    max.y = (radius * 2);
    max.z = (radius * 2);

    min = Vector3RotateByQuaternion(min, QuaternionFromMatrix(t));
    max = Vector3RotateByQuaternion(max, QuaternionFromMatrix(t));
    
    bbox.min = Vector3Add(pos, min);
    bbox.max = Vector3Add(pos, max);

    
    coll = GetRayCollisionBox(ray, bbox);
    if (coll.hit)
    {
      distance[2] = coll.distance;
      plane[2].position = coll.point;
      plane[2].normal = (Vector3){0,0,1};
      if (!IntersectRayPlane(ray, plane[2]).hit)
      {
        plane[2].normal = (Vector3){0,0,-1};
      }
      coll.hit = false;
      gizmo_hit = true;
    }
    
    for (int i = 1; i < 3; i++)
    {
      if (distance[selected_axis] > distance[i])
      {
        selected_axis = i;
      }
    }
    
    Color colors[3] = {GREEN, BLUE, RED};
    
    if (gizmo_hit)
    {
      colors[selected_axis].a = 0x88;
    }
    
    Vector3 end = Vector3Add(pos, (Vector3){0.0f, 0.0f, -length});
    DrawCylinderEx((Vector3){t.m12, t.m13, t.m14},
                   end, radius, radius, slices, colors[0]);
    DrawCylinderEx(end,
                   Vector3Add(end, (Vector3){0.0f, 0.0f, -length/4}),
                   radius * 2, 0, slices, colors[0]);
    end = Vector3Add(pos, (Vector3){0.0f, length, 0.0f});
    DrawCylinderEx((Vector3){t.m12, t.m13, t.m14},
                   end, radius, radius, slices, colors[1]);
    DrawCylinderEx(end, Vector3Add(end, (Vector3){0.0f, length/4, 0.0f}),
                   radius * 2, 0, slices, colors[1]);
    end = Vector3Add(pos, (Vector3){length, 0.0f, 0.0f});
    DrawCylinderEx((Vector3){t.m12, t.m13, t.m14},
                   end, radius, radius, slices, colors[2]);
    DrawCylinderEx(end, Vector3Add(end, (Vector3){length/4, 0.0f, 0.0f}),
                   radius * 2, 0, slices, colors[2]);
    
    DrawSphere(pos, radius * 2, YELLOW);
    
    if (gizmo_hit && IsMouseButtonPressed(0))
    {
      props_pos = selected_entities_props[0].pos;
      props_rot = selected_entities_props[0].rot;
      props_scl = selected_entities_props[0].scl;

      props_pos_start = props_pos;
      props_rot_start = props_rot;
      props_scl_start = props_scl;

      start_pos = IntersectRayPlane(ray, plane[selected_axis]).point;
      old_pos = props_pos;
      drag_mode = true;
      last_selected_axis = selected_axis;

    }
    
    if (drag_mode)
    {// zyx
      Vector3 pos = IntersectRayPlane(ray, plane[last_selected_axis]).point;
      Vector3 offset = Vector3Subtract(pos, start_pos);

      bool use_grid = false;
      float gridCube = 1;
      if (IsKeyDown(KEY_LEFT_CONTROL))
      {
        use_grid = true;
      }
      
      switch (last_selected_axis)
      {
        case 0:
        {
          offset.x = 0;
          offset.y = 0;
          if (use_grid)
            offset.z = round(offset.z / gridCube) * gridCube;
        } break;
        case 1:
        {
          offset.x = 0;
          if (use_grid)
            offset.y = round(offset.y / gridCube) * gridCube;
          offset.z = 0;
        } break;
        case 2:
        {
          if (use_grid)
            offset.x = round(offset.x / gridCube) * gridCube;
          offset.y = 0;
          offset.z = 0;
        } break;
      }
      
      props_pos = Vector3Add(old_pos, offset);
      DrawLine3D(start_pos, Vector3Add(start_pos, offset), YELLOW);

      update_props_entities(false);
    }


    if (IsMouseButtonReleased(0))
    {
      update_selected_props();
      drag_mode = false;
      last_selected_axis = 0;
    }    
  }
  
  if (!drag_mode && IsMouseButtonPressed(0) && editor_mode != 2)
  {
    u64 entities_count = 0;
    t_entity* entities = component_system_get_entities(&entities_count);

    int selected_entity = -1;
    for (u64 i = 0; i < entities_count; i++)
    {
      selected_entities_count = cvec_header(selected_entities)->size;
      if (selected_entities_count > 0 && (u64)selected_entities[0] == i)
      {
        continue;
      }
      if (!ctbl_exist(entities[i].components, "co_renderer")) continue;

      Matrix t = component_system_get_global_transform(i);
      t_co_renderer* renderer = cast_ptr(t_co_renderer) component_system_at("co_renderer", (u64) ctbl_get(entities[i].components, "co_renderer"));
      if (renderer != 0)
      {
        u64 meshes_count = cvec_header(renderer->meshes)->size;
        for (u64 m = 0; m < meshes_count; m++)
        {
          RayCollision meshHitInfo = GetRayCollisionMesh(ray, renderer->meshes[m], t);
          if (meshHitInfo.hit)
          {
            if ((!collision.hit) || (collision.distance > meshHitInfo.distance))
            {
              collision = meshHitInfo;
              selected_entity = i;
            }
          }
        }
      }
    }
    
    if (selected_entity != -1)
    {
      if (IsKeyDown(KEY_LEFT_CONTROL))
      {
        selected_entities = cvec_push(selected_entities, (u8*) &selected_entity);

        regular_transform r_transform = {0};
        load_props(selected_entity, &r_transform.pos, &r_transform.rot, &r_transform.scl);
        selected_entities_props = cvec_push(selected_entities_props, (u8*) &r_transform);
      }
      else
      {
        cvec_clear(selected_entities);
        
        entity_panel_open(selected_entity);
        selected_entities = cvec_push(selected_entities, (u8*) &selected_entity);

        cvec_clear(selected_entities_props);

        regular_transform r_transform = {0};
        load_props(selected_entity, &r_transform.pos, &r_transform.rot, &r_transform.scl);
        selected_entities_props = cvec_push(selected_entities_props, (u8*) &r_transform);
      }
    }
  }
  
  if (!drag_mode && !collision.hit && IsMouseButtonPressed(0) && !on_panel && !gizmo_hit)
  {
    cvec_clear(selected_entities);
  }
}


static int
create_player(int parent)
{
  int ent_id = -1;

  ent_id = component_system_create_entity(parent);

  Matrix transform = MatrixTranslate(0, 0, 0);
  component_system_set_local_transform(ent_id, transform);

  co_renderer_create_by_cfg(ent_id, 0);
  co_animation_create(ent_id);
  
  return ent_id;
}


void
editor_init()
{
  renderer_system_init();
  animation_system_init();
  camera_system_init();

  cvec(u8) ip_vs = shader_load("data/shaders/glsl330/infinite_plane.vs");
  cvec(u8) ip_fs = shader_load("data/shaders/glsl330/infinite_plane.fs");
  grid_shader = LoadShaderFromMemory(ip_vs, ip_fs);
  grid_mesh = GenMeshPlane(1, 1, 1, 1);

  renderer_system_add_cfg((t_co_renderer_cfg){ 0, "data/models/characters/body/Rogue_Naked.glb", false, -1 });
  renderer_system_add_cfg((t_co_renderer_cfg){ 1, "data/models/characters/armory/crossbow_2handed.gltf", false, -1 });
  camera_system_add_cfg((t_co_camera_cfg){ 85.0f, 0, 0});

  code_font = LoadFontEx("data/fonts/comic-code-regular.ttf", 24, 0, 250);
  SetTextureFilter(code_font.texture, TEXTURE_FILTER_TRILINEAR);

  GuiSetFont(code_font);

  for (int i = 0; i < STYLE_PROPS_COUNT; i++)
  {
    GuiSetStyle(styleProps[i].controlId, styleProps[i].propertyId, styleProps[i].propertyValue);
  }

  selected_entities = cvec_create(sizeof(int), 128, false);

  selected_entities_props = cvec_create(sizeof(regular_transform), 128, false);

  int editor_camera_entity_id = component_system_create_entity(-1);
  component_system_set_local_transform(editor_camera_entity_id, MatrixIdentity());
  component_system_create_component_by_cfg(editor_camera_entity_id, "co_camera", 0);

  // test for on camera object sync
  // int cross_entity_id = component_system_create_entity(editor_camera_entity_id);
  // component_system_set_local_transform(cross_entity_id, MatrixTranslate(-0.3, -0.3, 0.4));
  // component_system_create_component_by_cfg(cross_entity_id, "co_renderer", 1);
}


void
editor_term()
{

}


static Vector2 mouse_pos = {0};


void
editor_update(f32 dt)
{
  (void)dt;
  if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) &&
      editor_mode == 1)
  {
    editor_mode = 0;
    mouse_pos = GetMousePosition();
    DisableCursor();
  }
  
  if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT) &&
      editor_mode == 0)
  {
    editor_mode = 1;
    EnableCursor();
    SetMousePosition(mouse_pos.x, mouse_pos.y);
  }

  if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_S))
  {
    scene_system_save("data/scenes/test_game_0");
  }

  if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_O))
  {
    scene_system_load("data/scenes/test_game_0");
    return;
  }

  if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_E))
  {
    u64 selected_entities_count = cvec_header(selected_entities)->size;
    int ent_id = create_player(selected_entities_count > 0 ? selected_entities[selected_entities_count - 1] : -1);

    cvec_clear(selected_entities);
    selected_entities = cvec_push(selected_entities, (u8*) &ent_id);

    cvec_clear(selected_entities_props);

    regular_transform r_transform = {0};
    load_props(ent_id, &r_transform.pos, &r_transform.rot, &r_transform.scl);
    selected_entities_props = cvec_push(selected_entities_props, (u8*) &r_transform);
    
    entity_panel_open(ent_id);

#if 0
      {
        t_anim_bone anim_bone = bone_create(scene_get_current()->id,
                                            13, ent_id,
                                            renderer);
        engine_push_bone(&anim_bone);
        
        int dagger_id = model_import(anim_bone.ent_id,
                                     "data/models/characters/armory/dagger.gltf");

        //selected_entity = dagger_id;
        
        // TODO: "code to write a code" Make to one function, transform_set_rotation_y(180.0f);
        //       To much of a hastle cosed by geting components from enity.
        //       What can we make with it? Maybe chache all data in one place.
        t_entity* dagger = scene_get_entity(scene_get_current(),
                                            dagger_id);
        Matrix* transform = entity_get_transform(scene_get_current()->id,
                                                         dagger);
        *transform = MatrixRotateY(180.0f * DEG2RAD);
      }
#endif
  }

  if (IsKeyPressed(KEY_DELETE))
  {
    remove_entities();
  }

  if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_D))
  {
    u64 selected_entities_count = cvec_header(selected_entities)->size;
    if (selected_entities_count == 1)
    {
      selected_entities[0] = component_system_duplicate_entity(selected_entities[0]);
      regular_transform r_transform = {0};
      load_props(selected_entities[0], &r_transform.pos, &r_transform.rot, &r_transform.scl);
      selected_entities_props[0] = r_transform;
    }
  }


  // asset_picker_update();

  // if (IsKeyPressed(KEY_SPACE) && editor_mode > 0)
  // {
  //   asset_pick_camera = !asset_pick_camera;
  //   if (asset_pick_camera)
  //   {
  //     asset_picker_open();
  //   }
  //   else
  //   {
  //     asset_picker_close();
  //   }
  // }
}


void
editor_late_update(f32 dt)
{
  if (editor_mode == 0 ||
      editor_mode == 2)
  {
    camera_system_update(dt);
  }
}


static f32 r_color_time = 0.0f;


void
editor_render(f32 dt)
{
  t_entity* entities = component_system_get_entities(0);
  t_co_renderer* renderers = component_system_get("co_renderer", 0);
  u64 selected_entities_count = cvec_header(selected_entities)->size;
  for (u64 i = 0; i < selected_entities_count; i++)
  {
    int renderer_id = -1;
    if (ctbl_exist(entities[selected_entities[i]].components, "co_renderer"))
    {
      renderer_id = (u64) ctbl_get(entities[selected_entities[i]].components, "co_renderer");
    }

    if (renderer_id == -1) continue;

    Color color;
    r_color_time += TAU * dt;

    int red = (int)(127.5 * (1 + sin(r_color_time)));
    int green = (int)(127.5 * (1 + sin(r_color_time + TAU / 3)));
    int blue = (int)(127.5 * (1 + sin(r_color_time + 2 * TAU / 3)));

    color.r = red;
    color.g = green;
    color.b = blue;
    color.a = 255;

    rlEnableWireMode();
    co_renderer_draw_wires(&renderers[renderer_id], color);
    rlDisableWireMode();
  }

  rlDisableBackfaceCulling();
  renderer_system_draw_simple(grid_mesh, grid_shader);
  rlEnableBackfaceCulling();
}


void
editor_draw_gizmo(f32 dt)
{
  (void)dt;
  if (IsKeyDown(KEY_LEFT_SHIFT)) return;
  
  draw_gizmo();
  // draw_hierarchy();
}


void
editor_draw_ui()
{
  // TODO: should draw panels, mb from post render
  u64 selected_entities_count = cvec_header(selected_entities)->size;
  if (selected_entities_count > 0)
  {
    draw_entity_panel(selected_entities[0]);
  }
}


void
app_create(t_app* out_app)
{
  out_app->name = "Editor";
  out_app->window_pos_x = 100;
  out_app->window_pos_y = 100;
  out_app->window_size_x = WINDOW_WIDTH;
  out_app->window_size_y = WINDOW_HEIGHT;
  out_app->init = &editor_init;
  out_app->update = &editor_update;
  out_app->late_update = &editor_late_update;
  out_app->render = &editor_render;
  out_app->draw_ui = &editor_draw_ui;
  out_app->term = &editor_term;
  out_app->draw_gizmo = &editor_draw_gizmo;
}
