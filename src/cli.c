#include "cli.h"
#include "cmds/build_cmds.h"
#include "cmds/env_cmds.h"
#include "cmds/install_cmds.h"
#include "cmds/internal_cmds.h"
#include "cmds/self_cmds.h"
#include "cmds/toolchain_cmds.h"

static void print_usage(const char* prog) {
    printf("Zircon %s - The Zircon toolchain installer and build tool\n\n", ZIRCON_VERSION);
    printf("Usage: %s [OPTIONS] <COMMAND>\n\n", prog);
    printf("Commands:\n");
    printf("  self <SUBCOMMAND>     Commands to manage Zircon itself\n");
    printf("  build <reference>     Build a specific version of zrc\n");
    printf("  install [tag]         Install pre-built toolchains\n");
    printf("  import <archive>      Import a toolchain from an archive\n");
    printf("  switch <version>      Switch to a different toolchain version\n");
    printf("  list                  List installed toolchains\n");
    printf("  delete <version>      Delete a specific toolchain\n");
    printf("  prune [-y]            Remove unused toolchains\n");
    printf("  env [--shell SHELL]   Output shell environment configuration\n");
    printf("\nOptions:\n");
    printf("  -v, --version         Print version information\n");
    printf("  -h, --help            Print this help message\n");
}

cli_context_t* parse_args(int argc, char** argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return NULL;
    }
    
    /* Check for version flag */
    if (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0) {
        printf("zircon %s\n", ZIRCON_VERSION);
        return NULL;
    }
    
    /* Check for help flag */
    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        print_usage(argv[0]);
        return NULL;
    }
    
    cli_context_t* ctx = calloc(1, sizeof(cli_context_t));
    if (!ctx) return NULL;
    
    const char* cmd = argv[1];
    
    if (strcmp(cmd, "self") == 0) {
        ctx->command = CMD_SELF;
        if (argc < 3) {
            fprintf(stderr, "Error: 'self' requires a subcommand\n");
            free(ctx);
            return NULL;
        }
        const char* subcmd = argv[2];
        if (strcmp(subcmd, "version") == 0) {
            ctx->data.self_cmd.subcmd = SELF_CMD_VERSION;
        } else if (strcmp(subcmd, "build") == 0) {
            ctx->data.self_cmd.subcmd = SELF_CMD_BUILD;
            ctx->data.self_cmd.reference = argc > 3 ? string_duplicate(argv[3]) : string_duplicate("main");
        } else if (strcmp(subcmd, "import") == 0) {
            ctx->data.self_cmd.subcmd = SELF_CMD_IMPORT;
            if (argc < 4) {
                fprintf(stderr, "Error: 'self import' requires an archive path\n");
                free(ctx);
                return NULL;
            }
            ctx->data.self_cmd.archive = string_duplicate(argv[3]);
        } else if (strcmp(subcmd, "install") == 0) {
            ctx->data.self_cmd.subcmd = SELF_CMD_INSTALL;
            ctx->data.self_cmd.tag = argc > 3 ? string_duplicate(argv[3]) : string_duplicate("nightly");
        } else {
            fprintf(stderr, "Error: Unknown self subcommand: %s\n", subcmd);
            free(ctx);
            return NULL;
        }
    } else if (strcmp(cmd, "build") == 0) {
        ctx->command = CMD_BUILD;
        if (argc < 3) {
            fprintf(stderr, "Error: 'build' requires a reference argument\n");
            free(ctx);
            return NULL;
        }
        ctx->data.build_cmd.reference = string_duplicate(argv[2]);
        ctx->data.build_cmd.repo_url = string_duplicate("https://github.com/zirco-lang/zrc.git");
        /* TODO: Parse --zrc-repo flag */
    } else if (strcmp(cmd, "install") == 0) {
        ctx->command = CMD_INSTALL;
        ctx->data.install_cmd.tag = argc > 2 ? string_duplicate(argv[2]) : string_duplicate("nightly");
    } else if (strcmp(cmd, "import") == 0) {
        ctx->command = CMD_IMPORT;
        if (argc < 3) {
            fprintf(stderr, "Error: 'import' requires an archive path\n");
            free(ctx);
            return NULL;
        }
        ctx->data.import_cmd.archive = string_duplicate(argv[2]);
    } else if (strcmp(cmd, "switch") == 0) {
        ctx->command = CMD_SWITCH;
        if (argc < 3) {
            fprintf(stderr, "Error: 'switch' requires a version argument\n");
            free(ctx);
            return NULL;
        }
        ctx->data.switch_cmd.version = string_duplicate(argv[2]);
    } else if (strcmp(cmd, "list") == 0) {
        ctx->command = CMD_LIST;
    } else if (strcmp(cmd, "delete") == 0) {
        ctx->command = CMD_DELETE;
        if (argc < 3) {
            fprintf(stderr, "Error: 'delete' requires a version argument\n");
            free(ctx);
            return NULL;
        }
        ctx->data.delete_cmd.version = string_duplicate(argv[2]);
    } else if (strcmp(cmd, "prune") == 0) {
        ctx->command = CMD_PRUNE;
        ctx->data.prune_cmd.yes = (argc > 2 && (strcmp(argv[2], "-y") == 0 || strcmp(argv[2], "--yes") == 0));
    } else if (strcmp(cmd, "env") == 0) {
        ctx->command = CMD_ENV;
        ctx->data.env_cmd.shell = NULL;
        /* TODO: Parse --shell flag */
    } else if (strcmp(cmd, "_") == 0) {
        ctx->command = CMD_INTERNAL;
        if (argc < 3) {
            fprintf(stderr, "Error: internal command requires a subcommand\n");
            free(ctx);
            return NULL;
        }
        if (strcmp(argv[2], "bootstrap") == 0) {
            ctx->data.internal_cmd.subcmd = INTERNAL_CMD_BOOTSTRAP;
        } else {
            fprintf(stderr, "Error: Unknown internal subcommand: %s\n", argv[2]);
            free(ctx);
            return NULL;
        }
    } else {
        fprintf(stderr, "Error: Unknown command: %s\n", cmd);
        free(ctx);
        return NULL;
    }
    
    return ctx;
}

void free_cli_context(cli_context_t* ctx) {
    if (!ctx) return;
    
    switch (ctx->command) {
        case CMD_SELF:
            SAFE_FREE(ctx->data.self_cmd.reference);
            SAFE_FREE(ctx->data.self_cmd.archive);
            SAFE_FREE(ctx->data.self_cmd.tag);
            break;
        case CMD_BUILD:
            SAFE_FREE(ctx->data.build_cmd.reference);
            SAFE_FREE(ctx->data.build_cmd.repo_url);
            break;
        case CMD_INSTALL:
            SAFE_FREE(ctx->data.install_cmd.tag);
            break;
        case CMD_IMPORT:
            SAFE_FREE(ctx->data.import_cmd.archive);
            break;
        case CMD_SWITCH:
            SAFE_FREE(ctx->data.switch_cmd.version);
            break;
        case CMD_DELETE:
            SAFE_FREE(ctx->data.delete_cmd.version);
            break;
        case CMD_ENV:
            SAFE_FREE(ctx->data.env_cmd.shell);
            break;
        default:
            break;
    }
    
    free(ctx);
}

int dispatch_command(cli_context_t* ctx) {
    if (!ctx) return ERR_INVALID_ARG;
    
    switch (ctx->command) {
        case CMD_SELF:
            return dispatch_self_command(ctx);
        case CMD_BUILD:
            return dispatch_build_command(ctx);
        case CMD_INSTALL:
            return dispatch_install_command(ctx);
        case CMD_IMPORT:
            return dispatch_import_command(ctx);
        case CMD_SWITCH:
            return dispatch_switch_command(ctx);
        case CMD_LIST:
            return dispatch_list_command(ctx);
        case CMD_DELETE:
            return dispatch_delete_command(ctx);
        case CMD_PRUNE:
            return dispatch_prune_command(ctx);
        case CMD_ENV:
            return dispatch_env_command(ctx);
        case CMD_INTERNAL:
            return dispatch_internal_command(ctx);
        default:
            fprintf(stderr, "Error: Unknown command type\n");
            return ERR_INVALID_ARG;
    }
}
