#include "paths.h"
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

char *get_zircon_root_dir(void)
{
    char *prefix = getenv("ZIRCON_PREFIX");
    if (prefix)
        return strdup(prefix);

    char *home = getenv("HOME");
    if (!home)
    {
        struct passwd *pw = getpwuid(getuid());
        if (pw)
            home = pw->pw_dir;
    }

    if (home)
        return path_join(home, ".zircon");

    return strdup(".zircon");
}

char *get_toolchains_dir(void)
{
    char *root = get_zircon_root_dir();
    if (!root)
        return NULL;
    char *toolchains_dir = path_join(root, "toolchains");
    free(root);
    return toolchains_dir;
}

char *get_toolchain_dir(const char *toolchain_name)
{
    char *toolchains_dir = get_toolchains_dir();
    if (!toolchains_dir)
        return NULL;
    char *toolchain_dir = path_join(toolchains_dir, toolchain_name);
    free(toolchains_dir);
    return toolchain_dir;
}

char *get_current_toolchain_link_path(void)
{
    char *root = get_toolchains_dir();
    if (!root)
        return NULL;
    char *link_path = path_join(root, "current");
    free(root);
    return link_path;
}

char *get_current_toolchain_environment_script_path(void)
{
    char *root = get_toolchains_dir();
    if (!root)
        return NULL;
    char *script_path = path_join3(root, "current", "env.sh");
    free(root);
    return script_path;
}

char *get_self_dir(void)
{
    char *root = get_zircon_root_dir();
    if (!root)
        return NULL;
    char *self_dir = path_join(root, "self");
    free(root);
    return self_dir;
}

char *get_self_bin_dir(void)
{
    char *self_dir = get_self_dir();
    if (!self_dir)
        return NULL;
    char *bin_dir = path_join(self_dir, "bin");
    free(self_dir);
    return bin_dir;
}

char *get_self_binary_path(void)
{
    char *bin_dir = get_self_bin_dir();
    if (!bin_dir)
        return NULL;
    char *binary_path = path_join(bin_dir, "zircon");
    free(bin_dir);
    return binary_path;
}

int ensure_directories(void)
{
    char *root = get_zircon_root_dir();
    if (!root)
        return -1;
    int res = mkdir_recursive(root);
    free(root);
    if (res != 0)
        return -1;

    char *toolchains_dir = get_toolchains_dir();
    if (!toolchains_dir)
        return -1;
    res = mkdir_recursive(toolchains_dir);
    free(toolchains_dir);
    if (res != 0)
        return -1;

    char *self_dir = get_self_dir();
    if (!self_dir)
        return -1;
    res = mkdir_recursive(self_dir);
    free(self_dir);
    if (res != 0)
        return -1;

    char *bin_dir = get_self_bin_dir();
    if (!bin_dir)
        return -1;
    res = mkdir_recursive(bin_dir);
    free(bin_dir);
    if (res != 0)
        return -1;

    return 0;
}

char *path_join(const char *base, const char *component)
{
    if (!base || !component)
        return NULL;
    size_t base_len = strlen(base);
    size_t comp_len = strlen(component);
    bool needs_sep = (base_len > 0 && base[base_len - 1] != '/');

    char *result = malloc(base_len + comp_len + (needs_sep ? 1 : 0) + 1);
    if (!result)
        return NULL;

    memcpy(result, base, base_len);
    if (needs_sep)
        result[base_len] = '/';
    memcpy(result + base_len + (needs_sep ? 1 : 0), component, comp_len + 1);

    return result;
}

char *path_join3(const char *part1, const char *part2, const char *part3)
{
    char *temp = path_join(part1, part2);
    if (!temp)
        return NULL;
    char *result = path_join(temp, part3);
    free(temp);
    return result;
}

bool path_exists(const char *path)
{
    struct stat st;
    return (stat(path, &st) == 0);
}

bool is_directory(const char *path)
{
    struct stat st;
    if (stat(path, &st) != 0)
        return false;
    return S_ISDIR(st.st_mode);
}

int mkdir_recursive(const char *path)
{
    char *path_copy = strdup(path);
    if (!path_copy)
        return -1;

    char *p = path_copy;
    if (p[0] == '/')
        p++;

    for (; *p; p++)
    {
        if (*p == '/')
        {
            *p = '\0';
            if (!path_exists(path_copy))
            {
                if (mkdir(path_copy, 0755) != 0)
                {
                    free(path_copy);
                    return -1;
                }
            }
            *p = '/';
        }
    }

    if (!path_exists(path_copy))
    {
        if (mkdir(path_copy, 0755) != 0)
        {
            free(path_copy);
            return -1;
        }
    }

    free(path_copy);
    return 0;
}

int rm_recursive(const char *path)
{
    struct stat st;
    if (stat(path, &st) != 0)
        return -1;

    if (S_ISDIR(st.st_mode))
    {
        DIR *dir = opendir(path);
        if (!dir)
            return -1;

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL)
        {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            char *child_path = path_join(path, entry->d_name);
            if (!child_path)
            {
                closedir(dir);
                return -1;
            }

            if (rm_recursive(child_path) != 0)
            {
                free(child_path);
                closedir(dir);
                return -1;
            }
            free(child_path);
        }
        closedir(dir);
        if (rmdir(path) != 0)
            return -1;
    }
    else
    {
        if (remove(path) != 0)
            return -1;
    }

    return 0;
}

char *read_file_to_string(const char *path)
{
    FILE *file = fopen(path, "r");
    if (!file)
        return NULL;

    fseek(file, 0, SEEK_END);
    size_t length = (size_t)ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = malloc(length + 1);
    if (!buffer)
    {
        fclose(file);
        return NULL;
    }

    fread(buffer, 1, length, file);
    buffer[length] = '\0';

    fclose(file);
    return buffer;
}

int write_string_to_file(const char *path, const char *content)
{
    FILE *file = fopen(path, "w");
    if (!file)
        return -1;
    fwrite(content, 1, strlen(content), file);
    fclose(file);
    return 0;
}

bool string_starts_with(const char *str, const char *prefix)
{
    return strncmp(str, prefix, strlen(prefix)) == 0;
}
