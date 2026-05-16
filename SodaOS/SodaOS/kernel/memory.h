#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

void memory_init(void);
void* kmalloc(uint32_t size);
uint32_t memory_used(void);
uint32_t memory_free(void);
uint32_t memory_alloc_count(void);

#endif