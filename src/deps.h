#pragma once
#include "common.h"

int check_llvm(char **version_out);
int check_clang(char **version_out);
int check_dependencies_strict(void);
