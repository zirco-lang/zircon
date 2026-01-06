#include "prune.h"
#include "../toolchains.h"
#include "../common.h"
#include <stdio.h>
#include <stdlib.h>

int cmd_prune(const cli_context_t *ctx)
{
    size_t count = 0;
    char **to_prune = get_prunable_toolchains(&count);
    if (count == 0)
    {
        printf("No unused toolchains to prune.\n");
        return EXIT_SUCCESS;
    }

    if (!to_prune)
    {
        fprintf(stderr, "Failed to get list of toolchains to prune\n");
        return EXIT_GENERAL;
    }

    printf("The following toolchains will be removed:\n");
    for (size_t i = 0; i < count; i++)
    {
        printf(" - %s\n", to_prune[i]);
    }

    char *current = get_current_toolchain();
    if (current)
    {
        printf("Active toolchain %s will be kept.\n", current);
        free(current);
    }

    if (!ctx->data.prune.yes)
    {
        printf("\nProceed with deletion? (y/N): ");
        char response[10];
        if (fgets(response, sizeof(response), stdin) == NULL ||
            (response[0] != 'y' && response[0] != 'Y'))
        {
            printf("Aborting prune operation.\n");
            for (size_t i = 0; i < count; i++)
                free(to_prune[i]);

            free(to_prune);
            return EXIT_SUCCESS;
        }
    }

    for (size_t i = 0; i < count; i++)
    {
        if (delete_toolchain(to_prune[i]) == 0)
            printf("Deleted toolchain: %s\n", to_prune[i]);
        else
            fprintf(stderr, "Failed to delete toolchain: %s\n", to_prune[i]);
        free(to_prune[i]);
    }

    free(to_prune);
    return EXIT_SUCCESS;
}
