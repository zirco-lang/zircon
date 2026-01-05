#pragma once
#include "../common.h"
#include "../cli.h"

char *extract_version_from_filename(const char *path);
char *compute_archive_hash(const char *path);
int extract_archive(const char *archive_path, const char *dest_dir);
int cmd_import(const cli_context_t *ctx);
