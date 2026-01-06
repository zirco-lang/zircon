#include "import.h"
#include "../paths.h"
#include "../toolchains.h"
#include <stdio.h>
#include <archive.h>
#include <archive_entry.h>
#include <openssl/evp.h>
#include <stdlib.h>
#include <string.h>

/* Extract version name from archive filename */
char *extract_version_from_filename(const char *path)
{
    const char *filename = strrchr(path, '/');
    if (filename)
    {
        filename++; /* Skip the '/' */
    }
    else
    {
        filename = path;
    }

    /* Remove common extensions */
    char *name = strdup(filename);
    if (!name)
        return NULL;

    /* Trim extensions */
    char *ext = strstr(name, ".tar.gz");
    if (ext)
    {
        *ext = '\0';
    }
    else if ((ext = strstr(name, ".tgz")))
    {
        *ext = '\0';
    }
    else if ((ext = strstr(name, ".tar")))
    {
        *ext = '\0';
    }
    else if ((ext = strstr(name, ".zip")))
    {
        *ext = '\0';
    }

    if (name[0] == '\0')
    {
        free(name);
        return NULL;
    }

    return name;
}

/* Compute a short SHA256 hash of the archive file for uniqueness */
char *compute_archive_hash(const char *path)
{
    FILE *file = fopen(path, "rb");
    if (!file)
    {
        return NULL;
    }

    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if (!mdctx)
    {
        fclose(file);
        return NULL;
    }

    if (EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL) != 1)
    {
        EVP_MD_CTX_free(mdctx);
        fclose(file);
        return NULL;
    }

    unsigned char buffer[8192];
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0)
    {
        if (EVP_DigestUpdate(mdctx, buffer, bytes_read) != 1)
        {
            EVP_MD_CTX_free(mdctx);
            fclose(file);
            return NULL;
        }
    }

    unsigned char hash[32];
    unsigned int hash_len;
    if (EVP_DigestFinal_ex(mdctx, hash, &hash_len) != 1)
    {
        EVP_MD_CTX_free(mdctx);
        fclose(file);
        return NULL;
    }

    EVP_MD_CTX_free(mdctx);
    fclose(file);

    /* Convert to hex and return first 8 characters */
    char *hex_hash = malloc(9); /* 8 chars + null terminator */
    if (!hex_hash)
        return NULL;

    for (int i = 0; i < 4; i++)
    {
        sprintf(hex_hash + (i * 2), "%02x", hash[i]);
    }
    hex_hash[8] = '\0';

    return hex_hash;
}

/* Extract archive to destination directory */
int extract_archive(const char *archive_path, const char *dest_dir)
{
    struct archive *a = archive_read_new();
    struct archive *ext = archive_write_disk_new();
    struct archive_entry *entry;
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

    if ((r = archive_read_open_filename(a, archive_path, 10240)))
    {
        fprintf(stderr, "Failed to open archive: %s\n", archive_error_string(a));
        archive_read_free(a);
        archive_write_free(ext);
        return -1;
    }

    while ((r = archive_read_next_header(a, &entry)) == ARCHIVE_OK)
    {
        const char *current_file = archive_entry_pathname(entry);
        char full_path[MAX_PATH];
        snprintf(full_path, sizeof(full_path), "%s/%s", dest_dir, current_file);

        archive_entry_set_pathname(entry, full_path);

        if ((r = archive_write_header(ext, entry)) != ARCHIVE_OK)
        {
            fprintf(stderr, "Failed to write header: %s\n", archive_error_string(ext));
        }
        else
        {
            const void *buff;
            size_t size;
            int64_t offset;

            while ((r = archive_read_data_block(a, &buff, &size, &offset)) == ARCHIVE_OK)
            {
                if (archive_write_data_block(ext, buff, size, offset) != ARCHIVE_OK)
                {
                    fprintf(stderr, "Failed to write data: %s\n", archive_error_string(ext));
                    break;
                }
            }

            if (r != ARCHIVE_EOF)
            {
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

int cmd_import(const cli_context_t *ctx)
{
    if (!path_exists(ctx->data.import.archive_path))
    {
        fprintf(stderr, "Archive not found: %s\n", ctx->data.import.archive_path);
        return EXIT_GENERAL;
    }

    /* Compute hash of the archive */
    char *hash = compute_archive_hash(ctx->data.import.archive_path);
    if (!hash)
    {
        fprintf(stderr, "Failed to compute archive hash\n");
        return EXIT_GENERAL;
    }

    /* Extract base name from archive filename and append hash */
    char *base_name = extract_version_from_filename(ctx->data.import.archive_path);
    if (!base_name)
    {
        fprintf(stderr, "Could not determine version name from archive filename\n");
        free(hash);
        return EXIT_GENERAL;
    }

    /* Create version string: base_name-hash */
    char version[MAX_PATH];
    snprintf(version, sizeof(version), "%s-%s", base_name, hash);

    printf("Importing toolchain: %s\n", version);

    free(base_name);
    free(hash);

    /* Check if toolchain already exists */
    if (toolchain_exists(version))
    {
        fprintf(stderr, "Toolchain '%s' already exists.\n", version);
        fprintf(stderr, "Use 'zircon delete %s' to remove it first.\n", version);
        return EXIT_GENERAL;
    }

    /* Ensure directories exist */
    if (ensure_directories() != 0)
    {
        fprintf(stderr, "Failed to create directories\n");
        return EXIT_GENERAL;
    }

    /* Create toolchain directory */
    char *tc_dir = get_toolchain_dir(version);
    if (!tc_dir)
    {
        return EXIT_GENERAL;
    }

    if (mkdir_recursive(tc_dir) != 0)
    {
        fprintf(stderr, "Failed to create toolchain directory\n");
        free(tc_dir);
        return EXIT_GENERAL;
    }

    /* Extract archive */
    printf("Extracting archive...\n");
    if (extract_archive(ctx->data.import.archive_path, tc_dir) != 0)
    {
        fprintf(stderr, "Failed to extract archive\n");
        rm_recursive(tc_dir);
        free(tc_dir);
        return EXIT_GENERAL;
    }

    printf("✓ Successfully imported toolchain: %s\n", version);
    printf("  Toolchain location: %s\n", tc_dir);

    /* Always set as current */
    char *current_link = get_current_toolchain_link_path();
    if (current_link)
    {
        unlink(current_link); /* Remove existing symlink if it exists */
        if (symlink(tc_dir, current_link) != 0)
        {
            fprintf(stderr, "Failed to set current toolchain\n");
            free(current_link);
            free(tc_dir);
            return EXIT_GENERAL;
        }
        free(current_link);
        printf("✓ Set as current toolchain\n");
    }

    printf("\nTo use this toolchain, run:\n");
    printf("  source <(zircon env)\n");
    printf("*** You may need to restart your shell or source your profile for changes to take effect. ***\n");

    free(tc_dir);
    return EXIT_SUCCESS;
}
