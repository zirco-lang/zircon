#include "toolchain_cmds.h"
#include "../toolchains.h"
#include "../paths.h"
#include <openssl/sha.h>
#include <archive.h>
#include <archive_entry.h>

/* Extract version name from archive filename */
static char* extract_version_from_filename(const char* path) {
    const char* filename = strrchr(path, '/');
    if (filename) {
        filename++; /* Skip the '/' */
    } else {
        filename = path;
    }
    
    /* Remove common extensions */
    char* name = string_duplicate(filename);
    if (!name) return NULL;
    
    /* Trim extensions */
    char* ext = strstr(name, ".tar.gz");
    if (ext) {
        *ext = '\0';
    } else if ((ext = strstr(name, ".tgz"))) {
        *ext = '\0';
    } else if ((ext = strstr(name, ".tar"))) {
        *ext = '\0';
    } else if ((ext = strstr(name, ".zip"))) {
        *ext = '\0';
    }
    
    if (name[0] == '\0') {
        free(name);
        return NULL;
    }
    
    return name;
}

/* Compute a short SHA256 hash of the archive file for uniqueness */
static char* compute_archive_hash(const char* path) {
    FILE* file = fopen(path, "rb");
    if (!file) {
        return NULL;
    }
    
    SHA256_CTX sha256;
    if (!SHA256_Init(&sha256)) {
        fclose(file);
        return NULL;
    }
    
    unsigned char buffer[8192];
    size_t bytes_read;
    
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        SHA256_Update(&sha256, buffer, bytes_read);
    }
    
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &sha256);
    
    fclose(file);
    
    /* Convert to hex and return first 8 characters */
    char* hex_hash = malloc(9); /* 8 chars + null terminator */
    if (!hex_hash) return NULL;
    
    for (int i = 0; i < 4; i++) {
        sprintf(hex_hash + (i * 2), "%02x", hash[i]);
    }
    hex_hash[8] = '\0';
    
    return hex_hash;
}

/* Extract archive to destination directory */
static int extract_archive(const char* archive_path, const char* dest_dir) {
    struct archive* a = archive_read_new();
    struct archive* ext = archive_write_disk_new();
    struct archive_entry* entry;
    int r;
    
    /* Support various formats */
    archive_read_support_filter_gzip(a);
    archive_read_support_filter_bzip2(a);
    archive_read_support_format_tar(a);
    archive_read_support_format_zip(a);
    
    archive_write_disk_set_options(ext, 
        ARCHIVE_EXTRACT_TIME | 
        ARCHIVE_EXTRACT_PERM | 
        ARCHIVE_EXTRACT_ACL | 
        ARCHIVE_EXTRACT_FFLAGS);
    
    if ((r = archive_read_open_filename(a, archive_path, 10240))) {
        fprintf(stderr, "Failed to open archive: %s\n", archive_error_string(a));
        archive_read_free(a);
        archive_write_free(ext);
        return -1;
    }
    
    while ((r = archive_read_next_header(a, &entry)) == ARCHIVE_OK) {
        const char* current_file = archive_entry_pathname(entry);
        char full_path[MAX_PATH];
        snprintf(full_path, sizeof(full_path), "%s/%s", dest_dir, current_file);
        
        archive_entry_set_pathname(entry, full_path);
        
        if ((r = archive_write_header(ext, entry)) != ARCHIVE_OK) {
            fprintf(stderr, "Failed to write header: %s\n", archive_error_string(ext));
        } else {
            const void* buff;
            size_t size;
            int64_t offset;
            
            while ((r = archive_read_data_block(a, &buff, &size, &offset)) == ARCHIVE_OK) {
                if (archive_write_data_block(ext, buff, size, offset) != ARCHIVE_OK) {
                    fprintf(stderr, "Failed to write data: %s\n", archive_error_string(ext));
                    break;
                }
            }
            
            if (r != ARCHIVE_EOF) {
                fprintf(stderr, "Error reading archive: %s\n", archive_error_string(a));
            }
        }
        
        archive_write_finish_entry(ext);
    }
    
    archive_read_close(a);
    archive_read_free(a);
    archive_write_close(ext);
    archive_write_free(ext);
    
    return 0;
}

/* Validate that the extracted toolchain has the expected structure */
static int validate_toolchain_structure(const char* toolchain_dir) {
    /* Check for bin directory */
    char bin_path[MAX_PATH];
    snprintf(bin_path, sizeof(bin_path), "%s/bin", toolchain_dir);
    
    if (!path_exists(bin_path) || !is_directory(bin_path)) {
        fprintf(stderr, "Invalid toolchain structure: missing 'bin' directory at %s\n", bin_path);
        return -1;
    }
    
    /* Check for include directory (optional but expected) */
    char include_path[MAX_PATH];
    snprintf(include_path, sizeof(include_path), "%s/include", toolchain_dir);
    
    if (!path_exists(include_path)) {
        fprintf(stderr, "Warning: 'include' directory not found in toolchain\n");
    }
    
    return 0;
}

int dispatch_switch_command(cli_context_t* ctx) {
    char* tc_dir = toolchain_dir(ctx->data.switch_cmd.version);
    if (!tc_dir) return ERR_GENERAL;
    
    if (!toolchain_exists(ctx->data.switch_cmd.version)) {
        fprintf(stderr, "Toolchain '%s' not found at %s\n", 
                ctx->data.switch_cmd.version, tc_dir);
        fprintf(stderr, "Use 'zircon install %s' to install it.\n", 
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
    /* Verify archive exists */
    if (!path_exists(ctx->data.import_cmd.archive)) {
        fprintf(stderr, "Archive not found: %s\n", ctx->data.import_cmd.archive);
        return ERR_NOT_FOUND;
    }
    
    /* Compute hash of the archive */
    char* hash = compute_archive_hash(ctx->data.import_cmd.archive);
    if (!hash) {
        fprintf(stderr, "Failed to compute archive hash\n");
        return ERR_GENERAL;
    }
    
    /* Extract base name from archive filename and append hash */
    char* base_name = extract_version_from_filename(ctx->data.import_cmd.archive);
    if (!base_name) {
        fprintf(stderr, "Could not determine version name from archive filename\n");
        free(hash);
        return ERR_GENERAL;
    }
    
    /* Create version string: base_name-hash */
    char version[MAX_PATH];
    snprintf(version, sizeof(version), "%s-%s", base_name, hash);
    
    printf("Importing toolchain: %s\n", version);
    
    free(base_name);
    free(hash);
    
    /* Check if toolchain already exists */
    if (toolchain_exists(version)) {
        fprintf(stderr, "Toolchain '%s' already exists.\n", version);
        fprintf(stderr, "Use 'zircon delete %s' to remove it first.\n", version);
        return ERR_GENERAL;
    }
    
    /* Ensure directories exist */
    if (ensure_directories() != 0) {
        fprintf(stderr, "Failed to create directories\n");
        return ERR_IO;
    }
    
    /* Create toolchain directory */
    char* tc_dir = toolchain_dir(version);
    if (!tc_dir) {
        return ERR_GENERAL;
    }
    
    if (make_directory_recursive(tc_dir) != 0) {
        fprintf(stderr, "Failed to create toolchain directory\n");
        free(tc_dir);
        return ERR_IO;
    }
    
    /* Extract archive */
    printf("Extracting archive...\n");
    if (extract_archive(ctx->data.import_cmd.archive, tc_dir) != 0) {
        fprintf(stderr, "Failed to extract archive\n");
        remove_directory_recursive(tc_dir);
        free(tc_dir);
        return ERR_GENERAL;
    }
    
    /* Validate toolchain structure */
    if (validate_toolchain_structure(tc_dir) != 0) {
        remove_directory_recursive(tc_dir);
        free(tc_dir);
        return ERR_GENERAL;
    }
    
    printf("✓ Successfully imported toolchain: %s\n", version);
    printf("  Toolchain location: %s\n", tc_dir);
    
    /* Always set as current */
    char* current_link = current_toolchain_link();
    if (current_link) {
        create_link(tc_dir, current_link);
        free(current_link);
        printf("✓ Set as current toolchain\n");
    }
    
    printf("\nTo use this toolchain, run:\n");
    printf("  source <(zircon env)\n");
    
    free(tc_dir);
    return ERR_OK;
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
