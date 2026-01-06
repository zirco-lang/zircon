#include "self_import.h"
#include "import.h"
#include "../paths.h"
#include "../toolchains.h"
#include <stdio.h>
#include <archive.h>
#include <archive_entry.h>
#include <stdlib.h>
#include <string.h>

int cmd_self_import(const cli_context_t *ctx)
{
    printf("Importing Zircon from archive...\n");

    /* Verify archive exists */
    if (!path_exists(ctx->data.self_import.archive_path))
    {
        fprintf(stderr, "Archive not found: %s\n", ctx->data.self_import.archive_path);
        return EXIT_GENERAL;
    }

    /* Ensure directories exist */
    if (ensure_directories() != 0)
    {
        fprintf(stderr, "Failed to create directories\n");
        return EXIT_GENERAL;
    }

    char *root = get_zircon_root_dir();
    if (!root)
        return EXIT_GENERAL;

    char self_dir[MAX_PATH];
    snprintf(self_dir, sizeof(self_dir), "%s/self", root);
    free(root);

    /* Remove existing self directory if it exists */
    if (path_exists(self_dir))
    {
        if (rm_recursive(self_dir) != 0)
        {
            fprintf(stderr, "Failed to remove existing self directory\n");
            return EXIT_GENERAL;
        }
    }

    /* Create self directory */
    if (mkdir_recursive(self_dir) != 0)
    {
        fprintf(stderr, "Failed to create self directory\n");
        return EXIT_GENERAL;
    }

    /* Extract archive to self directory */
    printf("Extracting archive...\n");
    if (extract_archive(ctx->data.self_import.archive_path, self_dir) != 0)
    {
        fprintf(stderr, "Failed to extract archive\n");
        return EXIT_GENERAL;
    }

    /* Validate that bin directory exists */
    char self_bin_dir[MAX_PATH];
    snprintf(self_bin_dir, sizeof(self_bin_dir), "%s/bin", self_dir);

    if (!path_exists(self_bin_dir) || !is_directory(self_bin_dir))
    {
        fprintf(stderr, "Invalid archive structure: missing 'bin' directory at %s\n", self_bin_dir);
        return EXIT_GENERAL;
    }

    /* Find zircon binary in self/bin */
    char zircon_binary[MAX_PATH];
    snprintf(zircon_binary, sizeof(zircon_binary), "%s/bin/zircon", self_dir);

    if (!path_exists(zircon_binary))
    {
        fprintf(stderr, "Zircon binary not found in archive at %s\n", zircon_binary);
        return EXIT_GENERAL;
    }

/* Make executable on Unix */
#ifdef __unix__
    chmod(zircon_binary, 0755);
#endif

    printf("✓ Zircon imported successfully!\n");
    printf("  Location: %s\n", self_dir);

    return EXIT_SUCCESS;
}
