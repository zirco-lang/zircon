#include "cli.h"
#include "common.h"
#include "paths.h"
#include <stdio.h>

int main(int argc, char **argv)
{
    cli_context_t *ctx = parse_cli_args(argc, argv);
    if (!ctx)
        return EXIT_CLI_ERROR;

    if (ensure_directories() != 0)
    {
        fprintf(stderr, "Failed to create necessary directories\n");
        free_cli_context(ctx);
        return EXIT_GENERAL;
    }

    int exit = dispatch_command(ctx);
    free_cli_context(ctx);

    return exit;
}
