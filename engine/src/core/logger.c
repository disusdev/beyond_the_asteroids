#include "logger.h"

#include <containers/cvec.h>
#include <containers/cstr.h>

#include <stdio.h>
#include <stdarg.h>

// TODO: it is broke, fix logger in some near future!

#define DEFAULT_BUFFER_SIZE 1024

typedef struct
{
  cvec(u8) buffer;
} logger_state;

static logger_state state;

void
logger_init()
{
  state.buffer = cvec_create(sizeof(u8), DEFAULT_BUFFER_SIZE, false);
}

void
logger_term()
{
  cvec_destroy(state.buffer);
}

void
logger_log_args(const char* type,
                const char* str, ...)
{
  va_list arg_ptr;
  va_start(arg_ptr, str);
  u64 buffer_size = snprintf(NULL, 0, str, arg_ptr);
  state.buffer = cvec_resize(state.buffer, buffer_size + 1);
  i32 written = vsnprintf(cast_ptr(i8) state.buffer, buffer_size, str, arg_ptr);
  char* str_ptr = cvec_at(state.buffer, written);
  *str_ptr = 0;
  va_end(arg_ptr);
  printf("%s%s\n", type, (char*)state.buffer);
}
