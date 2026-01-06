#include "delete.h"
#include "../toolchains.h"
#include "../common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int cmd_delete(const cli_context_t *ctx)
{
    char *current = get_current_toolchain();
    if (current && strcmp(current, ctx->data.delete_toolchain.toolchain_name) == 0)
    {
        fprintf(stderr, "Error: Cannot delete the currently active toolchain '%s'. Please switch to a different toolchain before deleting.\n", current);
        free(current);
        return EXIT_GENERAL;
    }

    printf("Deleting toolchain %s\n", ctx->data.delete_toolchain.toolchain_name);

    if (delete_toolchain(ctx->data.delete_toolchain.toolchain_name) != 0)
    {
        fprintf(stderr, "Error: Failed to delete toolchain %s\n", ctx->data.delete_toolchain.toolchain_name);
        return EXIT_GENERAL;
    }

    printf("✓ Toolchain %s deleted.\n", ctx->data.delete_toolchain.toolchain_name);
    return EXIT_SUCCESS;
}
