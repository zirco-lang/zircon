#include "common.h"
#include <limits.h>

/* String helpers */
char* string_duplicate(const char* str) {
    if (!str) return NULL;
    size_t len = strlen(str);
    char* result = malloc(len + 1);
    if (result) {
        memcpy(result, str, len + 1);
    }
    return result;
}

char* string_concat(const char* str1, const char* str2) {
    if (!str1 || !str2) return NULL;
    size_t len1 = strlen(str1);
    size_t len2 = strlen(str2);
    char* result = malloc(len1 + len2 + 1);
    if (result) {
        memcpy(result, str1, len1);
        memcpy(result + len1, str2, len2 + 1);
    }
    return result;
}

char* string_concat3(const char* str1, const char* str2, const char* str3) {
    if (!str1 || !str2 || !str3) return NULL;
    size_t len1 = strlen(str1);
    size_t len2 = strlen(str2);
    size_t len3 = strlen(str3);
    char* result = malloc(len1 + len2 + len3 + 1);
    if (result) {
        memcpy(result, str1, len1);
        memcpy(result + len1, str2, len2);
        memcpy(result + len1 + len2, str3, len3 + 1);
    }
    return result;
}

bool string_starts_with(const char* str, const char* prefix) {
    if (!str || !prefix) return false;
    size_t prefix_len = strlen(prefix);
    size_t str_len = strlen(str);
    if (prefix_len > str_len) return false;
    return strncmp(str, prefix, prefix_len) == 0;
}

bool string_ends_with(const char* str, const char* suffix) {
    if (!str || !suffix) return false;
    size_t suffix_len = strlen(suffix);
    size_t str_len = strlen(str);
    if (suffix_len > str_len) return false;
    return strcmp(str + str_len - suffix_len, suffix) == 0;
}

char* string_replace_char(const char* str, char from, char to) {
    if (!str) return NULL;
    char* result = string_duplicate(str);
    if (!result) return NULL;
    for (char* p = result; *p; p++) {
        if (*p == from) *p = to;
    }
    return result;
}

/* Path helpers */
char* path_join(const char* base, const char* component) {
    if (!base || !component) return NULL;
    size_t base_len = strlen(base);
    size_t comp_len = strlen(component);
    bool needs_sep = (base_len > 0 && base[base_len - 1] != '/');
    
    char* result = malloc(base_len + (needs_sep ? 1 : 0) + comp_len + 1);
    if (result) {
        memcpy(result, base, base_len);
        if (needs_sep) result[base_len] = '/';
        memcpy(result + base_len + (needs_sep ? 1 : 0), component, comp_len + 1);
    }
    return result;
}

char* path_join3(const char* base, const char* comp1, const char* comp2) {
    char* temp = path_join(base, comp1);
    if (!temp) return NULL;
    char* result = path_join(temp, comp2);
    free(temp);
    return result;
}

bool path_exists(const char* path) {
    struct stat st;
    return stat(path, &st) == 0;
}

bool is_directory(const char* path) {
    struct stat st;
    if (stat(path, &st) != 0) return false;
    return S_ISDIR(st.st_mode);
}

int make_directory_recursive(const char* path) {
    char* path_copy = string_duplicate(path);
    if (!path_copy) return -1;
    
    char* p = path_copy;
    if (*p == '/') p++;
    
    while (*p) {
        while (*p && *p != '/') p++;
        char sep = *p;
        *p = '\0';
        
        if (!path_exists(path_copy)) {
            if (mkdir(path_copy, 0755) != 0 && errno != EEXIST) {
                free(path_copy);
                return -1;
            }
        }
        
        if (sep == '\0') break;
        *p++ = sep;
    }
    
    free(path_copy);
    return 0;
}

int remove_directory_recursive(const char* path) {
    DIR* dir = opendir(path);
    if (!dir) return -1;
    
    struct dirent* entry;
    int result = 0;
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        char* full_path = path_join(path, entry->d_name);
        if (!full_path) {
            result = -1;
            break;
        }
        
        if (is_directory(full_path)) {
            if (remove_directory_recursive(full_path) != 0) result = -1;
        } else {
            if (unlink(full_path) != 0) result = -1;
        }
        
        free(full_path);
    }
    
    closedir(dir);
    
    if (result == 0) {
        result = rmdir(path);
    }
    
    return result;
}

/* File helpers */
char* read_file_to_string(const char* path) {
    FILE* file = fopen(path, "r");
    if (!file) return NULL;
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* content = malloc(size + 1);
    if (!content) {
        fclose(file);
        return NULL;
    }
    
    size_t read_size = fread(content, 1, size, file);
    content[read_size] = '\0';
    
    fclose(file);
    return content;
}

int write_string_to_file(const char* path, const char* content) {
    FILE* file = fopen(path, "w");
    if (!file) return -1;
    
    size_t len = strlen(content);
    size_t written = fwrite(content, 1, len, file);
    
    fclose(file);
    return (written == len) ? 0 : -1;
}
