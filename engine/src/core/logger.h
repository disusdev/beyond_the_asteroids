#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <defines.h>

#include <containers/cstr.h>

// TODO: it is broke, fix logger in some near future!

void
logger_init();

void
logger_term();

void
logger_log_args(const char* type,
                const char* str, ...);

#define LOG_DEBUG(format, ...) do {\
logger_log_args("[DEBUG] ", format, ##__VA_ARGS__); } while (0)
#define LOG_EMPTY(format, ...) do {\
logger_log_args("", format, ##__VA_ARGS__); } while (0)
#define LOG_WARN(format, ...) do {\
logger_log_args("[WARN] ", format, ##__VA_ARGS__); } while (0)
#define LOG_ERROR(format, ...) do {\
logger_log_args("[ERROR] ", format, ##__VA_ARGS__); } while (0)

#endif
