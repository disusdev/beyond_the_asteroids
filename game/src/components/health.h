#ifndef __HEALTH_H__
#define __HEALTH_H__


#include <defines.h>
#include <systems/component_system.h>


typedef struct
{
  int entity_id;
  int health;
  b32 is_dead;
} t_co_health;


int
co_health_create(int entity_id, int cfg)
{
  t_co_health health = {0};
  health.entity_id = entity_id;
  health.health = 100;
  health.is_dead = false;
  return component_system_insert(entity_id, "co_health", &health);
}


void
health_system_init()
{
  component_system_add_component("co_health", sizeof(t_co_health), 128, 0, &co_health_create);
}


b32
health_system_on_health_change(int entity_id,
                               int health_offset)
{
  int hp_id = (u64) ctbl_get(component_system_get_entities(0)[entity_id].components, "co_health");
  t_co_health* hps = cast_ptr(t_co_health) component_system_get("co_health", 0);
  if (hps[hp_id].is_dead) return false;
  hps[hp_id].health += health_offset;
  if (hps[hp_id].health <= 0)
  {
    hps[hp_id].is_dead = true;
    component_system_push(entity_id);
    // component_system_remove_entity(entity_id);
    return true;
  }
  return false;
}


#endif