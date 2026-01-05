#include "self_cmds.h"
#include "../build.h"
#include "../git_utils.h"
#include "../paths.h"

static void cmd_version(void) {
    printf("zircon %s\n", ZIRCON_VERSION);
}

static int cmd_self_build(const char* reference) {
    printf("Building Zircon from '%s'...\n", reference);
    
    char* zircon_src = zircon_source_dir();
    if (!zircon_src) return ERR_GENERAL;
    
    git_repository* repo = NULL;
    if (clone_or_open("https://github.com/zirco-lang/zircon.git", zircon_src, &repo) != 0) {
        fprintf(stderr, "Failed to clone/open Zircon repository\n");
        free(zircon_src);
        return ERR_GIT;
    }
    
    fetch_repo(repo);
    checkout_ref(repo, reference);
    git_repository_free(repo);
    
    printf("Building Zircon...\n");
    check_cargo();
    build_rust_project(zircon_src);
    
    printf("✓ Zircon built successfully from '%s'!\n", reference);
    
    free(zircon_src);
    return ERR_OK;
}

int dispatch_self_command(cli_context_t* ctx) {
    switch (ctx->data.self_cmd.subcmd) {
        case SELF_CMD_VERSION:
            cmd_version();
            return ERR_OK;
        case SELF_CMD_BUILD:
            return cmd_self_build(ctx->data.self_cmd.reference);
        case SELF_CMD_IMPORT:
            printf("Note: Self import not yet fully implemented in C version\n");
            return ERR_GENERAL;
        case SELF_CMD_INSTALL:
            printf("Note: Self install not yet fully implemented in C version\n");
            return ERR_GENERAL;
        default:
            return ERR_INVALID_ARG;
    }
}
