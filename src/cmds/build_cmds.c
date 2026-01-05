#include "build_cmds.h"
#include "../deps.h"
#include "../git_utils.h"
#include "../paths.h"

static int run_build_hook(const char* source_dir, const char* toolchain_dir) {
    char hook_path[MAX_PATH];
    snprintf(hook_path, sizeof(hook_path), "%s/hooks/zircon.sh", source_dir);
    
    if (!path_exists(hook_path)) {
        fprintf(stderr, "Hook script not found at %s.\n", hook_path);
        fprintf(stderr, "This version of zrc may not support zircon hooks.\n");
        return -1;
    }
    
    printf("Running zrc build hook...\n");
    
    char cmd[MAX_CMD];
    snprintf(cmd, sizeof(cmd), 
             "cd '%s' && ZIRCON_TOOLCHAIN_DIR='%s' bash '%s'",
             source_dir, toolchain_dir, hook_path);
    
    int status = system(cmd);
    if (status != 0) {
        fprintf(stderr, "Hook script failed (exit code: %d)\n", status);
        return -1;
    }
    
    return 0;
}

int dispatch_build_command(cli_context_t* ctx) {
    /* Check dependencies first */
    if (check_dependencies_strict() != 0) {
        return ERR_GENERAL;
    }
    
    /* Ensure directories exist */
    if (ensure_directories() != 0) {
        fprintf(stderr, "Failed to create directories\n");
        return ERR_IO;
    }
    
    char* source_dir = zrc_source_dir();
    if (!source_dir) return ERR_GENERAL;
    
    /* Clone or open repository */
    git_repository* repo = NULL;
    if (clone_or_open(ctx->data.build_cmd.repo_url, source_dir, &repo) != 0) {
        fprintf(stderr, "Failed to clone/open repository\n");
        free(source_dir);
        return ERR_GIT;
    }
    
    /* Fetch latest changes */
    if (fetch_repo(repo) != 0) {
        fprintf(stderr, "Failed to fetch repository\n");
        git_repository_free(repo);
        free(source_dir);
        return ERR_GIT;
    }
    
    /* Checkout the requested reference */
    if (checkout_ref(repo, ctx->data.build_cmd.reference) != 0) {
        fprintf(stderr, "Failed to checkout reference: %s\n", ctx->data.build_cmd.reference);
        git_repository_free(repo);
        free(source_dir);
        return ERR_GIT;
    }
    
    /* Get commit SHA for version naming */
    char* commit_sha = get_current_commit_short(repo);
    if (!commit_sha) {
        fprintf(stderr, "Failed to get commit SHA\n");
        git_repository_free(repo);
        free(source_dir);
        return ERR_GIT;
    }
    
    /* Determine reference type and create appropriate version name */
    ref_info_t* ref_info = determine_ref_type(repo, ctx->data.build_cmd.reference);
    if (!ref_info) {
        fprintf(stderr, "Failed to determine reference type\n");
        free(commit_sha);
        git_repository_free(repo);
        free(source_dir);
        return ERR_GIT;
    }
    
    char version[MAX_PATH];
    if (ref_info->type == REF_TYPE_TAG) {
        snprintf(version, sizeof(version), "%s", ref_info->name);
    } else if (ref_info->type == REF_TYPE_BRANCH) {
        char* safe_branch = string_replace_char(ref_info->name, '/', '-');
        snprintf(version, sizeof(version), "%s@%s", safe_branch, commit_sha);
        free(safe_branch);
    } else {
        snprintf(version, sizeof(version), "%s", ref_info->name);
    }
    
    printf("Building version: %s\n", version);
    
    free_ref_info(ref_info);
    free(commit_sha);
    git_repository_free(repo);
    
    /* Create toolchain directory */
    char* tc_dir = toolchain_dir(version);
    if (!tc_dir) {
        free(source_dir);
        return ERR_GENERAL;
    }
    
    if (make_directory_recursive(tc_dir) != 0) {
        fprintf(stderr, "Failed to create toolchain directory\n");
        free(tc_dir);
        free(source_dir);
        return ERR_IO;
    }
    
    /* Execute the hook script */
    if (run_build_hook(source_dir, tc_dir) != 0) {
        free(tc_dir);
        free(source_dir);
        return ERR_GENERAL;
    }
    
    /* Update current symlink */
    char* current_link = current_toolchain_link();
    if (current_link) {
        create_link(tc_dir, current_link);
        free(current_link);
    }
    
    printf("\n✓ Successfully built and installed zrc %s\n", version);
    printf("  Toolchain location: %s\n", tc_dir);
    printf("\nTo use zrc, run:\n");
    printf("  source <(zircon env)\n");
    
    free(tc_dir);
    free(source_dir);
    
    return ERR_OK;
}
