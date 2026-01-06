#include "switch.h"
#include "../toolchains.h"
#include "../common.h"
#include "../paths.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int cmd_switch(const cli_context_t *ctx)
{
    char *tc_dir = get_toolchain_dir(ctx->data.switch_toolchain.toolchain_name);
    if (!tc_dir)
        return EXIT_GENERAL;

    if (!path_exists(tc_dir))
    {
        fprintf(stderr, "Error: Toolchain %s does not exist.\n", ctx->data.switch_toolchain.toolchain_name);
        free(tc_dir);
        return EXIT_GENERAL;
    }

    char *link_path = get_current_toolchain_link_path();
    if (!link_path)
    {
        free(tc_dir);
        return EXIT_GENERAL;
    }

    // Remove existing symlink if it exists
    unlink(link_path);
    if (symlink(tc_dir, link_path) != 0)
    {
        fprintf(stderr, "Error: Failed to create symlink for current toolchain.\n");
        free(tc_dir);
        free(link_path);
        return EXIT_GENERAL;
    }

    printf("✓ Switched to toolchain %s.\n", ctx->data.switch_toolchain.toolchain_name);
    free(tc_dir);
    free(link_path);
    return EXIT_SUCCESS;
}
