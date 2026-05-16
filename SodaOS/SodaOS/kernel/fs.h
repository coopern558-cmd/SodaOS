#ifndef FS_H
#define FS_H

#include <stdint.h>

void fs_init(void);
const char* fs_read(const char* path);
int fs_write(const char* path, const char* content);
const char* fs_path_at(uint32_t index);
uint32_t fs_count(void);

#endif