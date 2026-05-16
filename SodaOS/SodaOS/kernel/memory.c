#include "memory.h"

#define HEAP_SIZE 16384

static uint8_t g_heap[HEAP_SIZE];
static uint32_t g_heap_used = 0;
static uint32_t g_alloc_count = 0;

void memory_init(void)
{
    g_heap_used = 0;
    g_alloc_count = 0;
}

void* kmalloc(uint32_t size)
{
    uint32_t aligned = (size + 7U) & ~7U;
    void* ptr;
    if (aligned == 0 || g_heap_used + aligned > HEAP_SIZE) {
        return (void*)0;
    }
    ptr = &g_heap[g_heap_used];
    g_heap_used += aligned;
    g_alloc_count++;
    return ptr;
}

uint32_t memory_used(void) { return g_heap_used; }
uint32_t memory_free(void) { return HEAP_SIZE - g_heap_used; }
uint32_t memory_alloc_count(void) { return g_alloc_count; }