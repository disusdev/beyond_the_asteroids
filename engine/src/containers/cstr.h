#ifndef __CSTR_H__
#define __CSTR_H__

#include <defines.h>

u64
cstr_length(const char* str);

f64
cstr_2flt(char* str, i32* out_len, char end);

i64
cstr_2int(char* str, i32* out_len);

#endif