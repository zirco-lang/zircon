#include "toolchains.h"
#include "common.h"
#include "paths.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <libgen.h>
#include <unistd.h>
#include <dirent.h>

toolchain_list_t *list_toolchains(void)
{
    toolchain_list_t *list = calloc(1, sizeof(toolchain_list_t));
    if (!list)
        return NULL;

    char *toolchains_path = get_toolchains_dir();
    if (!toolchains_path || !path_exists(toolchains_path))
    {
        free(toolchains_path);
        return list;
    }

    char *current_ver = get_current_toolchain();

    DIR *dir = opendir(toolchains_path);
    if (dir)
    {
        struct dirent *entry;
        size_t capacity = 16;
        list->items = malloc(capacity * sizeof(toolchain_info_t));

        while ((entry = readdir(dir)) != NULL)
        {
            if (strcmp(entry->d_name, ".") == 0 ||
                strcmp(entry->d_name, "..") == 0 ||
                strcmp(entry->d_name, "current") == 0)
            {
                continue;
            }

            char *full_path = path_join(toolchains_path, entry->d_name);
            if (is_directory(full_path))
            {
                if (list->count >= capacity)
                {
                    capacity *= 2;
                    list->items = realloc(list->items, capacity * sizeof(toolchain_info_t));
                    if (!list->items)
                    {
                        free(full_path);
                        closedir(dir);
                        free_toolchain_list(list);
                        return NULL;
                    }
                }

                list->items[list->count].name = strdup(entry->d_name);
                list->items[list->count].is_current = (current_ver && strcmp(entry->d_name, current_ver) == 0);
                list->count++;
            }
            free(full_path);
        }
        closedir(dir);
    }

    free(toolchains_path);
    if (current_ver)
        free(current_ver);
    return list;
}

void free_toolchain_list(toolchain_list_t *list)
{
    if (!list)
        return;

    for (size_t i = 0; i < list->count; i++)
    {
        free(list->items[i].name);
    }
    free(list->items);
    free(list);
}

char *get_current_toolchain(void)
{
    char *link_path = get_current_toolchain_link_path();
    if (!link_path || !path_exists(link_path))
    {
        free(link_path);
        return NULL;
    }

    char target[MAX_PATH];
    ssize_t len = readlink(link_path, target, sizeof(target) - 1);
    free(link_path);

    if (len == -1)
        return NULL;
    target[len] = '\0';

    char *base = basename(target);
    return strdup(base);
}

int delete_toolchain(const char *toolchain_name)
{
    char *toolchain_path = path_join(get_toolchains_dir(), toolchain_name);
    if (!toolchain_path || !path_exists(toolchain_path))
    {
        free(toolchain_path);
        return -1;
    }

    // Recursively delete the toolchain directory
    int result = rm_recursive(toolchain_path);
    free(toolchain_path);
    return result;
}

char **get_prunable_toolchains(size_t *out_count)
{
    size_t count = 0;
    char **prunable = NULL;

    toolchain_list_t *list = list_toolchains();
    if (!list)
        return NULL;

    char *current = get_current_toolchain();

    for (size_t i = 0; i < list->count; i++)
    {
        if (current && strcmp(list->items[i].name, current) == 0)
            continue;

        prunable = realloc(prunable, (count + 1) * sizeof(char *));
        prunable[count] = strdup(list->items[i].name);
        count++;
    }

    free_toolchain_list(list);
    if (current)
        free(current);

    *out_count = count;
    return prunable;
}

bool toolchain_exists(const char *toolchain_name)
{
    char *toolchain_path = path_join(get_toolchains_dir(), toolchain_name);
    if (!toolchain_path)
        return false;

    bool exists = path_exists(toolchain_path) && is_directory(toolchain_path);
    free(toolchain_path);
    return exists;
}
