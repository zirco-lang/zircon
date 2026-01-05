#ifndef ZIRCON_CLI_H
#define ZIRCON_CLI_H

#include "common.h"

/* Command types */
typedef enum {
    CMD_SELF,
    CMD_INSTALL,
    CMD_IMPORT,
    CMD_SWITCH,
    CMD_LIST,
    CMD_DELETE,
    CMD_PRUNE,
    CMD_ENV,
    CMD_INTERNAL
} command_type_t;

typedef enum {
    SELF_CMD_VERSION,
    SELF_CMD_BUILD,
    SELF_CMD_IMPORT,
    SELF_CMD_INSTALL
} self_command_type_t;

typedef enum {
    INTERNAL_CMD_BOOTSTRAP
} internal_command_type_t;

/* CLI context */
typedef struct {
    command_type_t command;
    union {
        struct {
            self_command_type_t subcmd;
            char* reference;
            char* archive;
            char* tag;
        } self_cmd;
        struct {
            char* tag;
        } install_cmd;
        struct {
            char* archive;
        } import_cmd;
        struct {
            char* version;
        } switch_cmd;
        struct {
            char* version;
        } delete_cmd;
        struct {
            bool yes;
        } prune_cmd;
        struct {
            char* shell;
        } env_cmd;
        struct {
            internal_command_type_t subcmd;
        } internal_cmd;
    } data;
} cli_context_t;

/* Parse command line arguments */
cli_context_t* parse_args(int argc, char** argv);

/* Free CLI context */
void free_cli_context(cli_context_t* ctx);

/* Dispatch command */
int dispatch_command(cli_context_t* ctx);

#endif /* ZIRCON_CLI_H */
