#ifndef ZIRCON_PATHS_H
#define ZIRCON_PATHS_H

#include "common.h"

/* Get the Zircon root directory (default: ~/.zircon) */
char* zircon_root(void);

/* Get the sources directory */
char* sources_dir(void);

/* Get the zirco-lang organization directory */
char* zirco_lang_dir(void);

/* Get the zrc source directory */
char* zrc_source_dir(void);

/* Get the zircon source directory (for self updates) */
char* zircon_source_dir(void);

/* Get the toolchains directory */
char* toolchains_dir(void);

/* Get a specific toolchain directory */
char* toolchain_dir(const char* version);

/* Get the current toolchain symlink path */
char* current_toolchain_link(void);

/* Get the env.sh script path in the current toolchain */
char* current_toolchain_env_sh(void);

/* Get the bin directory */
char* bin_dir(void);

/* Get the zircon binary link in root bin */
char* zircon_binary_link(void);

/* Get the self bin directory */
char* self_bin_dir(void);

/* Get the zircon binary path in self */
char* self_zircon_binary(void);

/* Ensure all necessary directories exist */
int ensure_directories(void);

/* Create a symlink */
int create_link(const char* src, const char* dst);

#endif /* ZIRCON_PATHS_H */
