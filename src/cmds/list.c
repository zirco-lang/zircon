#include "list.h"
#include "../toolchains.h"
#include "../common.h"
#include <stdio.h>

int cmd_list(const cli_context_t *ctx)
{
    (void)ctx;
    toolchain_list_t *list = list_toolchains();
    if (!list)
    {
        fprintf(stderr, "Failed to list toolchains\n");
        return EXIT_GENERAL;
    }

    if (list->count == 0)
    {
        printf("No toolchains installed.\n");
        printf("Try installing one with 'zircon install <tag>'\n");
        free_toolchain_list(list);
        return EXIT_SUCCESS;
    }
    printf("Installed toolchains:\n");
    for (size_t i = 0; i < list->count; i++)
    {
        printf(" - %s%s\n", list->items[i].name,
               list->items[i].is_current ? " (active)" : "");
    }
    free_toolchain_list(list);
    return EXIT_SUCCESS;
}
