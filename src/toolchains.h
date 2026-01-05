#ifndef ZIRCON_TOOLCHAINS_H
#define ZIRCON_TOOLCHAINS_H

#include "common.h"

typedef struct {
    char* name;
    bool is_current;
} toolchain_info_t;

typedef struct {
    toolchain_info_t* items;
    size_t count;
} toolchain_list_t;

/* List all installed toolchains */
toolchain_list_t* list_toolchains(void);

/* Free toolchain list */
void free_toolchain_list(toolchain_list_t* list);

/* Get the currently active toolchain name */
char* get_current_toolchain(void);

/* Delete a specific toolchain */
int delete_toolchain(const char* version);

/* Get list of toolchains that can be pruned */
char** get_prunable_toolchains(size_t* count);

/* Check if a toolchain exists */
bool toolchain_exists(const char* version);

#endif /* ZIRCON_TOOLCHAINS_H */
