#ifndef DEBUG_H
#define DEBUG_H

#include <stdint.h>

void debug_init(void);
void debug_log(const char* msg);
void debug_log_hex32(const char* prefix, uint32_t value);

#endif