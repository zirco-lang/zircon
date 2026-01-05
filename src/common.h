#ifndef ZIRCON_COMMON_H
#define ZIRCON_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <libgen.h>

/* Version information */
#define ZIRCON_VERSION "0.1.0"

/* Maximum path length */
#define MAX_PATH 4096

/* Maximum command length */
#define MAX_CMD 8192

/* Error codes */
#define ERR_OK 0
#define ERR_GENERAL 1
#define ERR_INVALID_ARG 2
#define ERR_NOT_FOUND 3
#define ERR_IO 4
#define ERR_GIT 5
#define ERR_NETWORK 6

/* Utility macros */
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/* Memory management helpers */
#define SAFE_FREE(ptr) do { if (ptr) { free(ptr); ptr = NULL; } } while(0)

/* String helpers */
char* string_duplicate(const char* str);
char* string_concat(const char* str1, const char* str2);
char* string_concat3(const char* str1, const char* str2, const char* str3);
bool string_starts_with(const char* str, const char* prefix);
bool string_ends_with(const char* str, const char* suffix);
char* string_replace_char(const char* str, char from, char to);

/* Path helpers */
char* path_join(const char* base, const char* component);
char* path_join3(const char* base, const char* comp1, const char* comp2);
bool path_exists(const char* path);
bool is_directory(const char* path);
int make_directory_recursive(const char* path);
int remove_directory_recursive(const char* path);

/* File helpers */
char* read_file_to_string(const char* path);
int write_string_to_file(const char* path, const char* content);

#endif /* ZIRCON_COMMON_H */
