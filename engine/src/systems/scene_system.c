#include "scene_system.h"
#include <stdio.h>
#include <string.h>
#include <core/mem.h>
#include <containers/cstr.h>

void
scene_system_switch(int scene_id)
{
  component_system_switch_state(scene_id);
}


u8*
load_raw_binary(const char* path)
{
  FILE* stream = fopen(path, "rb");

  fseek(stream, 0, SEEK_END);
  u64 file_size = ftell(stream);
  fseek(stream, 0, SEEK_SET);

  u8* buffer = mem_alloc(file_size, MEM_TAG_NONE);
  fread(buffer, file_size, 1, stream);

  fclose(stream);

  return buffer;
}


int
scene_system_load(const char* path)
{
  int state_id = component_system_add_new_state();
  component_system_switch_state(state_id);

  char bin_path[128] = {0};
  strcat(bin_path, path);
  strcat(bin_path, ".bscn");
  u8* raw = load_raw_binary(bin_path);

  u8* ptr = raw;

  struct props
  {
    u64 entities_count;
  };

  struct props* p = (struct props*) ptr;
  ptr += sizeof(struct props);

  for (u64 i = 0; i < p->entities_count; i++)
  {
    // @fix add name?
    // u8 *name = ptr;
    ptr += 128;

    Matrix *transform = (Matrix*)ptr;
    ptr += sizeof(Matrix);

    t_hierarchy *hierarchy = (t_hierarchy*)ptr;
    ptr += sizeof(t_hierarchy);

    u64 components_count = *(u64*) ptr;
    ptr += sizeof(u64);
    
    u8** components_arr = mem_calloc(components_count, sizeof(char*), MEM_TAG_NONE);
    for (u64 j = 0; j < components_count; j++)
    {
      u64 str_len = *(u64*) ptr;
      ptr += sizeof(u64);

      components_arr[j] = ptr;
      ptr += str_len + 1;
    }

    u64 cfg_count = *(u64*) ptr;
    ptr += sizeof(u64);

    struct cfg
    {
      u8* key;
      u64 id;
    };

    struct cfg* cfg_arr = mem_calloc(components_count, sizeof(struct cfg), MEM_TAG_NONE);
    for (u64 j = 0; j < cfg_count; j++)
    {
      u64 str_len = *(u64*) ptr;
      ptr += sizeof(u64);

      cfg_arr[j].key = ptr;
      ptr += str_len + 1;

      cfg_arr[j].id = *(u64*) ptr;
      ptr += sizeof(u64);
    }

    int entity_id = component_system_create_entity(hierarchy->parent_id);
    component_system_set_local_transform(entity_id, *transform);

    for (u64 j = 0; j < cfg_count; j++)
    {
      component_system_create_component_by_cfg(entity_id, (const char*)cfg_arr[j].key, cfg_arr[j].id);
    }

    for (u64 j = 0; j < components_count; j++)
    {
      component_system_create_component(entity_id, (const char*)components_arr[j]);
    }

    mem_free(components_arr, sizeof(components_arr), MEM_TAG_NONE);
    mem_free(cfg_arr, sizeof(cfg_arr), MEM_TAG_NONE);
  }

  mem_free(raw, sizeof(raw), MEM_TAG_NONE);
  return state_id;
}


void
scene_system_save(const char* path)
{
  char txt_path[128] = {0};
  strcat(txt_path, path);
  strcat(txt_path, ".scn");
  FILE* scene_file = fopen(txt_path, "w+");

  u64 entities_count = 0;
  t_entity* entities = component_system_get_entities(&entities_count);

  fprintf(scene_file, "+ properties\n"
                      "|\n"
                      "|-> entities_count %lld\n"
                      "|\n"
                      "+ properties\n\n", entities_count);

  for (u64 i = 0; i < entities_count; i++)
  {
    fprintf(scene_file, "+ entity %s\n"
                        "|\n", entities[i].name);

    Matrix transform = component_system_get_local_transform(i);
    fprintf(scene_file, "|-> transform %f %f %f %f\n"
                        "|             %f %f %f %f\n"
                        "|             %f %f %f %f\n"
                        "|             %f %f %f %f\n"
                        "|\n",
                        transform.m0, transform.m4, transform.m8, transform.m12,
                        transform.m1, transform.m5, transform.m9, transform.m13,
                        transform.m2, transform.m6, transform.m10, transform.m14,
                        transform.m3, transform.m7, transform.m11, transform.m15);

    t_hierarchy hierarchy = component_system_get_hierarchy(i);
    fprintf(scene_file, "|-> hierarchy %d %d %d\n"
                        "|             %d %d %d\n"
                        "|\n",
                        hierarchy.id, hierarchy.parent_id, hierarchy.left_sibling,
                        hierarchy.right_sibling, hierarchy.first_child, hierarchy.depth);

    u64 components_count = ctbl_size(entities[i].components);
    fprintf(scene_file, "|-+ components %lld\n", components_count);
    {
      void** components_arr = ctbl_to_array(entities[i].components, 0);
      for (u64 j = 0; j < components_count; j++)
      {
        const char* component_key = components_arr[j * 2 + 0];
        fprintf(scene_file, "| |-> %s\n", component_key);
      }
      mem_free(components_arr, sizeof(components_arr), MEM_TAG_NONE);
    }
    fprintf(scene_file, "|\n");

    u64 cfg_count = ctbl_size(entities[i].components_cfgs);
    fprintf(scene_file, "|-+ cfgs %lld\n", cfg_count);
    {
      void** cfg_arr = ctbl_to_array(entities[i].components_cfgs, 0);
      for (u64 j = 0; j < cfg_count; j++)
      {
        const char* cfg_key = cfg_arr[j * 2 + 0];
        u64 cfg_id = (u64)cfg_arr[j * 2 + 1];
        fprintf(scene_file, "| |-> %s %lld\n", cfg_key, cfg_id);
      }
      mem_free(cfg_arr, sizeof(cfg_arr), MEM_TAG_NONE);
    }

    fprintf(scene_file, "|\n"
                        "+ entity %s\n\n", entities[i].name);
  }
  
  fclose(scene_file);

  char bin_path[128] = {0};
  strcat(bin_path, path);
  strcat(bin_path, ".bscn");
  FILE* scene_binary_file = fopen(bin_path, "wb+");

  struct props
  {
    u64 entities_count;
  };

  struct props p = { entities_count };
  fwrite(&p, sizeof(struct props), 1, scene_binary_file);

  for (u64 i = 0; i < entities_count; i++)
  {
    fwrite(entities[i].name, 127, 1, scene_binary_file);
    fwrite("\0", 1, 1, scene_binary_file);

    Matrix transform = component_system_get_local_transform(i);
    fwrite(&transform, sizeof(Matrix), 1, scene_binary_file);

    t_hierarchy hierarchy = component_system_get_hierarchy(i);
    fwrite(&hierarchy, sizeof(t_hierarchy), 1, scene_binary_file);

    u64 components_count = ctbl_size(entities[i].components);
    fwrite(&components_count, sizeof(u64), 1, scene_binary_file);
    
    {
      void** components_arr = ctbl_to_array(entities[i].components, 0);
      for (u64 j = 0; j < components_count; j++)
      {
        const char* component_key = components_arr[j * 2 + 0];
        u64 str_len = strlen(component_key);
        fwrite(&str_len, sizeof(u64), 1, scene_binary_file);
        fwrite(component_key, strlen(component_key), 1, scene_binary_file);
        fwrite("\0", 1, 1, scene_binary_file);
      }
      mem_free(components_arr, sizeof(components_arr), MEM_TAG_NONE);
    }

    u64 cfg_count = ctbl_size(entities[i].components_cfgs);
    fwrite(&cfg_count, sizeof(u64), 1, scene_binary_file);
    {
      void** cfg_arr = ctbl_to_array(entities[i].components_cfgs, 0);
      for (u64 j = 0; j < cfg_count; j++)
      {
        const char* cfg_key = cfg_arr[j * 2 + 0];
        u64 cfg_id = (u64)cfg_arr[j * 2 + 1];
        u64 str_len = strlen(cfg_key);
        fwrite(&str_len, sizeof(u64), 1, scene_binary_file);
        fwrite(cfg_key, strlen(cfg_key), 1, scene_binary_file);
        fwrite("\0", 1, 1, scene_binary_file);
        fwrite(&cfg_id, sizeof(u64), 1, scene_binary_file);
      }
      mem_free(cfg_arr, sizeof(cfg_arr), MEM_TAG_NONE);
    }
  }
  
  fclose(scene_binary_file);
}


void
scene_system_init()
{

}


void
scene_system_term()
{

}