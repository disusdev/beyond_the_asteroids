#ifndef __FS_H__
#define __FS_H__

#include <defines.h>
#include <stdio.h>

i64 fs_getline(char** lineptr,
               i64* n,
               FILE* stream);

#endif
