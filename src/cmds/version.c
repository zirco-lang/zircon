#include "version.h"
#include "../common.h"
#include <stdio.h>

int cmd_version(const cli_context_t *ctx)
{
    (void)ctx;
    printf("Zircon v%s\n", ZIRCON_VERSION);
    return EXIT_SUCCESS;
}
