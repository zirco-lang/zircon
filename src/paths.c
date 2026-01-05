#include "paths.h"
#include <pwd.h>

char* zircon_root(void) {
    char* prefix = getenv("ZIRCON_PREFIX");
    if (prefix) {
        return string_duplicate(prefix);
    }
    
    char* home = getenv("HOME");
    if (!home) {
        struct passwd* pw = getpwuid(getuid());
        if (pw) home = pw->pw_dir;
    }
    
    if (home) {
        return path_join(home, ".zircon");
    }
    
    return string_duplicate(".zircon");
}

char* sources_dir(void) {
    char* root = zircon_root();
    if (!root) return NULL;
    char* result = path_join(root, "sources");
    free(root);
    return result;
}

char* zirco_lang_dir(void) {
    char* sources = sources_dir();
    if (!sources) return NULL;
    char* result = path_join(sources, "zirco-lang");
    free(sources);
    return result;
}

char* zrc_source_dir(void) {
    char* zirco = zirco_lang_dir();
    if (!zirco) return NULL;
    char* result = path_join(zirco, "zrc");
    free(zirco);
    return result;
}

char* zircon_source_dir(void) {
    char* zirco = zirco_lang_dir();
    if (!zirco) return NULL;
    char* result = path_join(zirco, "zircon");
    free(zirco);
    return result;
}

char* toolchains_dir(void) {
    char* root = zircon_root();
    if (!root) return NULL;
    char* result = path_join(root, "toolchains");
    free(root);
    return result;
}

char* toolchain_dir(const char* version) {
    char* toolchains = toolchains_dir();
    if (!toolchains) return NULL;
    char* result = path_join(toolchains, version);
    free(toolchains);
    return result;
}

char* current_toolchain_link(void) {
    char* toolchains = toolchains_dir();
    if (!toolchains) return NULL;
    char* result = path_join(toolchains, "current");
    free(toolchains);
    return result;
}

char* current_toolchain_env_sh(void) {
    char* current = current_toolchain_link();
    if (!current) return NULL;
    char* result = path_join(current, "env.sh");
    free(current);
    return result;
}

char* bin_dir(void) {
    char* root = zircon_root();
    if (!root) return NULL;
    char* result = path_join(root, "bin");
    free(root);
    return result;
}

char* zircon_binary_link(void) {
    char* bin = bin_dir();
    if (!bin) return NULL;
    char* result = path_join(bin, "zircon");
    free(bin);
    return result;
}

char* self_bin_dir(void) {
    char* zircon_src = zircon_source_dir();
    if (!zircon_src) return NULL;
    char* target = path_join(zircon_src, "target");
    free(zircon_src);
    if (!target) return NULL;
    char* result = path_join(target, "release");
    free(target);
    return result;
}

char* self_zircon_binary(void) {
    char* self_bin = self_bin_dir();
    if (!self_bin) return NULL;
    char* result = path_join(self_bin, "zircon");
    free(self_bin);
    return result;
}

int ensure_directories(void) {
    int result = 0;
    
    char* sources = sources_dir();
    if (sources) {
        result |= make_directory_recursive(sources);
        free(sources);
    }
    
    char* zirco = zirco_lang_dir();
    if (zirco) {
        result |= make_directory_recursive(zirco);
        free(zirco);
    }
    
    char* toolchains = toolchains_dir();
    if (toolchains) {
        result |= make_directory_recursive(toolchains);
        free(toolchains);
    }
    
    char* self_bin = self_bin_dir();
    if (self_bin) {
        result |= make_directory_recursive(self_bin);
        free(self_bin);
    }
    
    char* bin = bin_dir();
    if (bin) {
        result |= make_directory_recursive(bin);
        free(bin);
    }
    
    return result;
}

int create_link(const char* src, const char* dst) {
    /* Remove existing link if present */
    if (path_exists(dst)) {
        if (is_directory(dst)) {
            char link_target[MAX_PATH];
            ssize_t len = readlink(dst, link_target, sizeof(link_target) - 1);
            
            if (len == -1) {
                /* Not a symlink, remove directory */
                remove_directory_recursive(dst);
            } else {
                /* Is a symlink */
                unlink(dst);
            }
        } else {
            unlink(dst);
        }
    }
    
    return symlink(src, dst);
}
