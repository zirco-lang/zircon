#ifndef ZIRCON_DEPS_H
#define ZIRCON_DEPS_H

#include "common.h"

/* Check if LLVM 20 is installed */
int check_llvm(char** version_out);

/* Check if clang is installed */
int check_clang(char** version_out);

/* Check dependencies and return error if missing (strict mode for bootstrap and build) */
int check_dependencies_strict(void);

#endif /* ZIRCON_DEPS_H */
