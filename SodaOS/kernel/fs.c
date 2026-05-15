#include "fs.h"

typedef struct {
    const char* path;
    char data[256];
} FsFile;

static FsFile g_fs[] = {
    {"/home/readme.txt", "Welcome to SodaOS RAMFS."},
    {"/home/notes.txt", ""}
};

static int str_eq(const char* a, const char* b)
{
    uint32_t i = 0;
    while (a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) {
            return 0;
        }
        ++i;
    }
    return a[i] == b[i];
}

static void str_copy(char* dst, const char* src, uint32_t max)
{
    uint32_t i = 0;
    if (max == 0) {
        return;
    }
    while (i + 1 < max && src[i] != '\0') {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
}

void fs_init(void) {}

uint32_t fs_count(void)
{
    return (uint32_t)(sizeof(g_fs) / sizeof(g_fs[0]));
}

const char* fs_path_at(uint32_t index)
{
    if (index >= fs_count()) {
        return (const char*)0;
    }
    return g_fs[index].path;
}

const char* fs_read(const char* path)
{
    for (uint32_t i = 0; i < fs_count(); ++i) {
        if (str_eq(g_fs[i].path, path)) {
            return g_fs[i].data;
        }
    }
    return (const char*)0;
}

int fs_write(const char* path, const char* content)
{
    for (uint32_t i = 0; i < fs_count(); ++i) {
        if (str_eq(g_fs[i].path, path)) {
            str_copy(g_fs[i].data, content, sizeof(g_fs[i].data));
            return 1;
        }
    }
    return 0;
}