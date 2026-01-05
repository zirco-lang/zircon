#include "internal_cmds.h"
#include "../deps.h"
#include "../paths.h"

static int cmd_bootstrap(void) {
    printf("=== Zircon Bootstrap ===\n\n");
    
    if (check_dependencies_strict() != 0) {
        return ERR_GENERAL;
    }
    
    if (ensure_directories() != 0) {
        fprintf(stderr, "Failed to create directories\n");
        return ERR_IO;
    }
    
    printf("\n✓ Bootstrap complete!\n");
    
    char* root = zircon_root();
    printf("\nZircon is installed at: %s\n", root ? root : "(unknown)");
    SAFE_FREE(root);
    
    char* bin = bin_dir();
    printf("\nNext steps:\n");
    printf("  1. Add Zircon to your PATH:\n");
    printf("     export PATH=\"%s:$PATH\"\n", bin ? bin : "(unknown)");
    printf("\n  2. Then load the environment with:\n");
    printf("     source <(zircon env)\n");
    printf("\n  3. Install a zrc version:\n");
    printf("     zircon build main\n");
    printf("     zircon build v0.1.0\n");
    printf("\n  To make these settings permanent, add to your shell profile (~/.bashrc, ~/.zshrc, etc.):\n");
    printf("     echo 'source <(%s/zircon env)' >> ~/.bashrc\n", bin ? bin : "(unknown)");
    SAFE_FREE(bin);
    
    return ERR_OK;
}

int dispatch_internal_command(cli_context_t* ctx) {
    switch (ctx->data.internal_cmd.subcmd) {
        case INTERNAL_CMD_BOOTSTRAP:
            return cmd_bootstrap();
        default:
            return ERR_INVALID_ARG;
    }
}
