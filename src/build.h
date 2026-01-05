#ifndef ZIRCON_BUILD_H
#define ZIRCON_BUILD_H

#include "common.h"

/* Build a Rust project using cargo */
int build_rust_project(const char* source_dir);

/* Check if cargo is available */
int check_cargo(void);

#endif /* ZIRCON_BUILD_H */
