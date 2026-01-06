#pragma once
#include <stdbool.h>

typedef enum
{
    CMD_SELF_INSTALL,
    CMD_SELF_IMPORT,
    CMD_INSTALL,
    CMD_IMPORT,
    CMD_SWITCH,
    CMD_LIST,
    CMD_DELETE,
    CMD_PRUNE,
    CMD_ENV,
    CMD_HELP,
    CMD_VERSION
} command_type_t;

typedef struct
{
    command_type_t command;
    char *prog_name;
    union
    {
        struct
        {
            char *tag;
        } self_install;

        struct
        {
            char *archive_path;
        } self_import;

        struct
        {
            char *tag;
        } install;

        struct
        {
            char *archive_path;
        } import;

        struct
        {
            char *toolchain_name;
        } switch_toolchain;

        struct
        {
            char *toolchain_name;
        } delete_toolchain;

        struct
        {
            bool yes;
        } prune;
    } data;
} cli_context_t;

void print_usage(const char *prog_name);
cli_context_t *parse_cli_args(int argc, char **argv);
void free_cli_context(cli_context_t *ctx);
int dispatch_command(const cli_context_t *ctx);
