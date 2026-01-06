#include "common.h"
#include "cli.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmds/self_install.h"
#include "cmds/self_import.h"
#include "cmds/install.h"
#include "cmds/import.h"
#include "cmds/switch.h"
#include "cmds/list.h"
#include "cmds/delete.h"
#include "cmds/prune.h"
#include "cmds/env.h"
#include "cmds/help.h"
#include "cmds/version.h"

void print_usage(const char *prog_name)
{
    printf("Zircon v%s - The Zircon toolchain manager\n", ZIRCON_VERSION);
    printf("Usage: %s [options] <command>\n", prog_name);
    printf("Commands:\n");
    printf("  self-install [tag]    Install a specific version of Zircon itself\n");
    printf("  self-import <archive> Import a Zircon release from an archive\n");
    printf("  install [tag]         Install a specific version of Zirco\n");
    printf("  import <archive>      Import a toolchain from an archive\n");
    printf("  switch <toolchain>    Switch to a different toolchain\n");
    printf("  list                  List installed toolchains\n");
    printf("  delete <toolchain>    Delete an installed toolchain\n");
    printf("  prune [-y]            Remove unused toolchains\n");
    printf("  env                   Print environment setup commands\n");
    printf("  help                  Show this help message\n");
    printf("  version               Show version information\n");
}

cli_context_t *
parse_cli_args(int argc, char **argv)
{
    if (argc < 2)
    {
        print_usage(argv[0]);
        return NULL;
    }

    cli_context_t *ctx = calloc(1, sizeof(cli_context_t));
    if (!ctx)
        return NULL;

    ctx->prog_name = strdup(argv[0]);
    if (!ctx->prog_name)
    {
        free(ctx);
        return NULL;
    }
    const char *cmd = argv[1];

    if (strcmp(cmd, "self-install") == 0)
    {
        ctx->command = CMD_SELF_INSTALL;
        if (argc >= 3)
        {
            ctx->data.self_install.tag = strdup(argv[2]);
        }
        else
        {
            ctx->data.self_install.tag = strdup(ZIRCON_SELFINSTALL_DEFAULT_TAG);
        }
    }
    else if (strcmp(cmd, "self-import") == 0)
    {
        ctx->command = CMD_SELF_IMPORT;
        if (argc >= 3)
        {
            ctx->data.self_import.archive_path = strdup(argv[2]);
        }
        else
        {
            fprintf(stderr, "import command requires an archive path\n");
            print_usage(argv[0]);
            free_cli_context(ctx);
            return NULL;
        }
    }
    else if (strcmp(cmd, "install") == 0)
    {
        ctx->command = CMD_INSTALL;
        if (argc >= 3)
        {
            ctx->data.install.tag = strdup(argv[2]);
        }
        else
        {
            ctx->data.install.tag = strdup(ZIRCON_INSTALL_DEFAULT_TAG);
        }
    }
    else if (strcmp(cmd, "import") == 0)
    {
        ctx->command = CMD_IMPORT;
        if (argc >= 3)
        {
            ctx->data.import.archive_path = strdup(argv[2]);
        }
        else
        {
            fprintf(stderr, "import command requires an archive path\n");
            print_usage(argv[0]);
            free_cli_context(ctx);
            return NULL;
        }
    }
    else if (strcmp(cmd, "switch") == 0)
    {
        ctx->command = CMD_SWITCH;
        if (argc >= 3)
        {
            ctx->data.switch_toolchain.toolchain_name = strdup(argv[2]);
        }
        else
        {
            fprintf(stderr, "switch command requires a toolchain name\n");
            print_usage(argv[0]);
            free_cli_context(ctx);
            return NULL;
        }
    }
    else if (strcmp(cmd, "list") == 0)
    {
        ctx->command = CMD_LIST;
    }
    else if (strcmp(cmd, "delete") == 0)
    {
        ctx->command = CMD_DELETE;
        if (argc >= 3)
        {
            ctx->data.delete_toolchain.toolchain_name = strdup(argv[2]);
        }
        else
        {
            fprintf(stderr, "delete command requires a toolchain name\n");
            print_usage(argv[0]);
            free_cli_context(ctx);
            return NULL;
        }
    }
    else if (strcmp(cmd, "prune") == 0)
    {
        ctx->command = CMD_PRUNE;
        if (argc >= 3 && strcmp(argv[2], "-y") == 0)
        {
            ctx->data.prune.yes = true;
        }
    }
    else if (strcmp(cmd, "env") == 0)
    {
        ctx->command = CMD_ENV;
    }
    else if (strcmp(cmd, "help") == 0)
    {
        ctx->command = CMD_HELP;
    }
    else if (strcmp(cmd, "version") == 0)
    {
        ctx->command = CMD_VERSION;
    }
    else
    {
        fprintf(stderr, "Unknown command\n");
        print_usage(argv[0]);
        free_cli_context(ctx);
        return NULL;
    }

    return ctx;
}

void free_cli_context(cli_context_t *ctx)
{
    if (!ctx)
        return;

    free(ctx->prog_name);

    switch (ctx->command)
    {
    case CMD_SELF_INSTALL:
        free(ctx->data.self_install.tag);
        break;
    case CMD_INSTALL:
        free(ctx->data.install.tag);
        break;
    case CMD_IMPORT:
        free(ctx->data.import.archive_path);
        break;
    case CMD_SWITCH:
        free(ctx->data.switch_toolchain.toolchain_name);
        break;
    case CMD_DELETE:
        free(ctx->data.delete_toolchain.toolchain_name);
        break;
    default:
        break;
    }

    free(ctx);
}

int dispatch_command(const cli_context_t *ctx)
{
    switch (ctx->command)
    {
    case CMD_SELF_INSTALL:
        return cmd_self_install(ctx);
    case CMD_SELF_IMPORT:
        return cmd_self_import(ctx);
    case CMD_INSTALL:
        return cmd_install(ctx);
    case CMD_IMPORT:
        return cmd_import(ctx);
    case CMD_SWITCH:
        return cmd_switch(ctx);
    case CMD_LIST:
        return cmd_list(ctx);
    case CMD_DELETE:
        return cmd_delete(ctx);
    case CMD_PRUNE:
        return cmd_prune(ctx);
    case CMD_ENV:
        return cmd_env(ctx);
    case CMD_HELP:
        return cmd_help(ctx);
    case CMD_VERSION:
        return cmd_version(ctx);
    default:
        fprintf(stderr, "Unknown command type\n");
        return EXIT_CLI_ERROR;
    }
}
