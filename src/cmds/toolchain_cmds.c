#include "toolchain_cmds.h"
#include "../toolchains.h"
#include "../paths.h"

int dispatch_switch_command(cli_context_t* ctx) {
    char* tc_dir = toolchain_dir(ctx->data.switch_cmd.version);
    if (!tc_dir) return ERR_GENERAL;
    
    if (!toolchain_exists(ctx->data.switch_cmd.version)) {
        fprintf(stderr, "Toolchain '%s' not found at %s\n", 
                ctx->data.switch_cmd.version, tc_dir);
        fprintf(stderr, "Use 'zircon build %s' to install it.\n", 
                ctx->data.switch_cmd.version);
        free(tc_dir);
        return ERR_NOT_FOUND;
    }
    
    char* current_link = current_toolchain_link();
    if (current_link) {
        create_link(tc_dir, current_link);
        free(current_link);
    }
    
    printf("✓ Switched to toolchain: %s\n", ctx->data.switch_cmd.version);
    
    free(tc_dir);
    return ERR_OK;
}

int dispatch_import_command(cli_context_t* ctx) {
    printf("Importing toolchain from archive: %s\n", ctx->data.import_cmd.archive);
    printf("Note: Archive import not yet fully implemented in C version\n");
    return ERR_GENERAL;
}

int dispatch_list_command(cli_context_t* ctx) {
    (void)ctx;
    toolchain_list_t* list = list_toolchains();
    if (!list) {
        fprintf(stderr, "Failed to list toolchains\n");
        return ERR_GENERAL;
    }
    
    if (list->count == 0) {
        printf("No toolchains installed.\n");
    } else {
        printf("Installed toolchains:\n");
        for (size_t i = 0; i < list->count; i++) {
            if (list->items[i].is_current) {
                printf("  %s (current)\n", list->items[i].name);
            } else {
                printf("  %s\n", list->items[i].name);
            }
        }
    }
    
    free_toolchain_list(list);
    return ERR_OK;
}

int dispatch_delete_command(cli_context_t* ctx) {
    printf("Deleting toolchain: %s\n", ctx->data.delete_cmd.version);
    
    if (delete_toolchain(ctx->data.delete_cmd.version) != 0) {
        return ERR_GENERAL;
    }
    
    printf("✓ Toolchain '%s' deleted\n", ctx->data.delete_cmd.version);
    return ERR_OK;
}

int dispatch_prune_command(cli_context_t* ctx) {
    size_t count = 0;
    char** to_prune = get_prunable_toolchains(&count);
    
    if (count == 0) {
        printf("No unused toolchains to prune.\n");
        return ERR_OK;
    }
    
    printf("Toolchains to be deleted:\n");
    for (size_t i = 0; i < count; i++) {
        printf("  %s\n", to_prune[i]);
    }
    
    char* current = get_current_toolchain();
    if (current) {
        printf("\nCurrent toolchain '%s' will be kept.\n", current);
        free(current);
    }
    
    if (!ctx->data.prune_cmd.yes) {
        printf("\nProceed with deletion? (y/N): ");
        char response[10];
        if (fgets(response, sizeof(response), stdin)) {
            if (response[0] != 'y' && response[0] != 'Y') {
                printf("Cancelled.\n");
                for (size_t i = 0; i < count; i++) {
                    free(to_prune[i]);
                }
                free(to_prune);
                return ERR_OK;
            }
        }
    }
    
    printf("\nDeleting toolchains...\n");
    for (size_t i = 0; i < count; i++) {
        delete_toolchain(to_prune[i]);
        printf("  ✓ Deleted %s\n", to_prune[i]);
        free(to_prune[i]);
    }
    free(to_prune);
    
    printf("\n✓ Pruned %zu toolchain(s)\n", count);
    return ERR_OK;
}
