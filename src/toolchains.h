#pragma once
#include <stdbool.h>
#include <stddef.h>

typedef struct
{
    char *name;
    bool is_current;
} toolchain_info_t;

typedef struct
{
    toolchain_info_t *items;
    size_t count;
} toolchain_list_t;

toolchain_list_t *list_toolchains(void);
void free_toolchain_list(toolchain_list_t *list);

char *get_current_toolchain(void);
int delete_toolchain(const char *toolchain_name);
char **get_prunable_toolchains(size_t *out_count);
bool toolchain_exists(const char *toolchain_name);
