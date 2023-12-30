#ifndef __SCENE_SYSTEM_H__
#define __SCENE_SYSTEM_H__

#include <defines.h>
#include "component_system.h"


void
scene_system_switch(int scene_id);


int
scene_system_load(const char* path);


void
scene_system_save(const char* path);


void
scene_system_init();


void
scene_system_term();


#endif