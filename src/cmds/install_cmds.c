#include "install_cmds.h"
#include "toolchain_cmds.h"

int dispatch_install_command(cli_context_t* ctx) {
    printf("Installing %s release...\n", ctx->data.install_cmd.tag);
    printf("Note: Pre-built installation not yet fully implemented in C version\n");
    printf("Please use 'zircon build <version>' instead\n");
    return ERR_GENERAL;
}
