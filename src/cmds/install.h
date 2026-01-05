#pragma once
#include "../common.h"
#include "../cli.h"

int download_file(const char *url, const char *dest_path);
int cmd_install(const cli_context_t *ctx);
