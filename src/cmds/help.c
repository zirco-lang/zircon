#include "help.h"

int cmd_help(const cli_context_t *ctx)
{
    print_usage(ctx->prog_name);
    return EXIT_SUCCESS;
}
