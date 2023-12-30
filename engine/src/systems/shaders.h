#ifndef __SHADERS_H__
#define __SHADERS_H__

#include <defines.h>
#include <containers/cvec.h>


void
shaders_init();


void
shaders_term();


cvec(u8) shader_load(const char* path);


const char*
shaders_get(const char* key);


void
shaders_set(const char* path, cvec(u8) src);


#endif