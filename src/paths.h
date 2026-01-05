#pragma once
#include <stdbool.h>

char *get_zircon_root_dir(void);
char *get_toolchains_dir(void);
char *get_toolchain_dir(const char *toolchain_name);
char *get_current_toolchain_link_path(void);
char *get_current_toolchain_environment_script_path(void);
char *get_self_dir(void);
char *get_self_bin_dir(void);
char *get_self_binary_path(void);
int ensure_directories(void);

char *path_join(const char *base, const char *component);
char *path_join3(const char *part1, const char *part2, const char *part3);
bool path_exists(const char *path);
bool is_directory(const char *path);
int mkdir_recursive(const char *path);
int rm_recursive(const char *path);
char *read_file_to_string(const char *path);
int write_string_to_file(const char *path, const char *content);
bool string_starts_with(const char *str, const char *prefix);
