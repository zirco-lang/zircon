#include "toolchains.h"
#include "paths.h"

toolchain_list_t* list_toolchains(void) {
    toolchain_list_t* list = calloc(1, sizeof(toolchain_list_t));
    if (!list) return NULL;
    
    char* toolchains_path = toolchains_dir();
    if (!toolchains_path || !path_exists(toolchains_path)) {
        free(toolchains_path);
        return list;
    }
    
    char* current_ver = get_current_toolchain();
    
    DIR* dir = opendir(toolchains_path);
    if (dir) {
        struct dirent* entry;
        size_t capacity = 16;
        list->items = malloc(capacity * sizeof(toolchain_info_t));
        
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || 
                strcmp(entry->d_name, "..") == 0 ||
                strcmp(entry->d_name, "current") == 0) {
                continue;
            }
            
            char* full_path = path_join(toolchains_path, entry->d_name);
            if (is_directory(full_path)) {
                if (list->count >= capacity) {
                    capacity *= 2;
                    list->items = realloc(list->items, capacity * sizeof(toolchain_info_t));
                }
                
                list->items[list->count].name = string_duplicate(entry->d_name);
                list->items[list->count].is_current = (current_ver && strcmp(entry->d_name, current_ver) == 0);
                list->count++;
            }
            free(full_path);
        }
        closedir(dir);
    }
    
    free(toolchains_path);
    SAFE_FREE(current_ver);
    return list;
}

void free_toolchain_list(toolchain_list_t* list) {
    if (list) {
        for (size_t i = 0; i < list->count; i++) {
            SAFE_FREE(list->items[i].name);
        }
        SAFE_FREE(list->items);
        free(list);
    }
}

char* get_current_toolchain(void) {
    char* link_path = current_toolchain_link();
    if (!link_path || !path_exists(link_path)) {
        free(link_path);
        return NULL;
    }
    
    char target[MAX_PATH];
    ssize_t len = readlink(link_path, target, sizeof(target) - 1);
    free(link_path);
    
    if (len == -1) return NULL;
    target[len] = '\0';
    
    char* base = basename(target);
    return string_duplicate(base);
}

int delete_toolchain(const char* version) {
    char* tc_dir = toolchain_dir(version);
    if (!tc_dir || !path_exists(tc_dir)) {
        fprintf(stderr, "Toolchain '%s' not found.\n", version);
        free(tc_dir);
        return -1;
    }
    
    char* current = get_current_toolchain();
    if (current && strcmp(current, version) == 0) {
        fprintf(stderr, "Cannot delete '%s' because it is the current toolchain.\n", version);
        fprintf(stderr, "Switch to another toolchain first with 'zircon switch <version>'.\n");
        SAFE_FREE(current);
        free(tc_dir);
        return -1;
    }
    SAFE_FREE(current);
    
    int result = remove_directory_recursive(tc_dir);
    free(tc_dir);
    return result;
}

char** get_prunable_toolchains(size_t* count) {
    *count = 0;
    toolchain_list_t* list = list_toolchains();
    if (!list) return NULL;
    
    char** result = malloc(list->count * sizeof(char*));
    if (!result) {
        free_toolchain_list(list);
        return NULL;
    }
    
    for (size_t i = 0; i < list->count; i++) {
        if (!list->items[i].is_current) {
            result[*count] = string_duplicate(list->items[i].name);
            (*count)++;
        }
    }
    
    free_toolchain_list(list);
    return result;
}

bool toolchain_exists(const char* version) {
    char* tc_dir = toolchain_dir(version);
    if (!tc_dir) return false;
    bool exists = path_exists(tc_dir);
    free(tc_dir);
    return exists;
}
