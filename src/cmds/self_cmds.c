#include "self_cmds.h"
#include "../build.h"
#include "../git_utils.h"
#include "../paths.h"
#include <curl/curl.h>
#include <archive.h>
#include <archive_entry.h>

static void cmd_version(void) {
    printf("zircon %s\n", ZIRCON_VERSION);
}

/* Detect platform and architecture */
static int detect_platform_and_arch(char* platform, size_t platform_size, 
                                     char* arch, size_t arch_size) {
    #ifdef __linux__
        snprintf(platform, platform_size, "linux");
    #elif defined(__APPLE__)
        snprintf(platform, platform_size, "macos");
    #else
        fprintf(stderr, "Unsupported platform. Only linux and macos are supported.\n");
        return -1;
    #endif
    
    #if defined(__x86_64__) || defined(_M_X64)
        snprintf(arch, arch_size, "x64");
    #elif defined(__aarch64__) || defined(_M_ARM64)
        snprintf(arch, arch_size, "arm64");
    #else
        fprintf(stderr, "Unsupported architecture. Only x86_64 (x64) and aarch64 (arm64) are supported.\n");
        return -1;
    #endif
    
    return 0;
}

/* Callback for curl to write data to file */
static size_t write_data(void* ptr, size_t size, size_t nmemb, FILE* stream) {
    return fwrite(ptr, size, nmemb, stream);
}

/* Download file from URL to destination path */
static int download_file(const char* url, const char* dest_path) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Failed to initialize curl\n");
        return -1;
    }
    
    FILE* fp = fopen(dest_path, "wb");
    if (!fp) {
        fprintf(stderr, "Failed to open file for writing: %s\n", dest_path);
        curl_easy_cleanup(curl);
        return -1;
    }
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
    
    CURLcode res = curl_easy_perform(curl);
    
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    
    fclose(fp);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        fprintf(stderr, "Failed to download file: %s (HTTP %ld)\n", curl_easy_strerror(res), http_code);
        fprintf(stderr, "The release may not be available or may not have pre-built binaries for your platform.\n");
        unlink(dest_path);
        return -1;
    }
    
    return 0;
}

/* Extract archive to self directory */
static int extract_self_archive(const char* archive_path, const char* dest_dir) {
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

/* Import Zircon from an archive */
static int cmd_self_import(const char* archive_path) {
    printf("Importing Zircon from archive...\n");
    
    /* Verify archive exists */
    if (!path_exists(archive_path)) {
        fprintf(stderr, "Archive not found: %s\n", archive_path);
        return ERR_NOT_FOUND;
    }
    
    /* Ensure directories exist */
    if (ensure_directories() != 0) {
        fprintf(stderr, "Failed to create directories\n");
        return ERR_IO;
    }
    
    char* root = zircon_root();
    if (!root) return ERR_GENERAL;
    
    char self_dir[MAX_PATH];
    snprintf(self_dir, sizeof(self_dir), "%s/self", root);
    free(root);
    
    /* Remove existing self directory if it exists */
    if (path_exists(self_dir)) {
        if (remove_directory_recursive(self_dir) != 0) {
            fprintf(stderr, "Failed to remove existing self directory\n");
            return ERR_IO;
        }
    }
    
    /* Create self directory */
    if (make_directory_recursive(self_dir) != 0) {
        fprintf(stderr, "Failed to create self directory\n");
        return ERR_IO;
    }
    
    /* Extract archive to self directory */
    printf("Extracting archive...\n");
    if (extract_self_archive(archive_path, self_dir) != 0) {
        fprintf(stderr, "Failed to extract archive\n");
        return ERR_GENERAL;
    }
    
    /* Validate that bin directory exists */
    char self_bin_dir[MAX_PATH];
    snprintf(self_bin_dir, sizeof(self_bin_dir), "%s/bin", self_dir);
    
    if (!path_exists(self_bin_dir) || !is_directory(self_bin_dir)) {
        fprintf(stderr, "Invalid archive structure: missing 'bin' directory at %s\n", self_bin_dir);
        return ERR_GENERAL;
    }
    
    /* Find zircon binary in self/bin */
    char zircon_binary[MAX_PATH];
    snprintf(zircon_binary, sizeof(zircon_binary), "%s/bin/zircon", self_dir);
    
    if (!path_exists(zircon_binary)) {
        fprintf(stderr, "Zircon binary not found in archive at %s\n", zircon_binary);
        return ERR_NOT_FOUND;
    }
    
    /* Make executable on Unix */
    #ifdef __unix__
    chmod(zircon_binary, 0755);
    #endif
    
    /* Create link in bin directory */
    char* zircon_link = zircon_binary_link();
    if (zircon_link) {
        create_link(zircon_binary, zircon_link);
        free(zircon_link);
    }
    
    printf("✓ Zircon imported successfully!\n");
    printf("  Location: %s\n", self_dir);
    
    return ERR_OK;
}

/* Install a pre-built Zircon release */
static int cmd_self_install(const char* tag) {
    printf("Installing Zircon %s release...\n", tag);
    
    /* Detect platform and architecture */
    char platform[32], arch[32];
    if (detect_platform_and_arch(platform, sizeof(platform), arch, sizeof(arch)) != 0) {
        return ERR_GENERAL;
    }
    
    /* Construct download URL for zircon repository */
    char filename[256];
    snprintf(filename, sizeof(filename), "zircon-%s-%s.tar.gz", platform, arch);
    
    char url[1024];
    snprintf(url, sizeof(url), 
             "https://github.com/zirco-lang/zircon/releases/download/%s/%s",
             tag, filename);
    
    printf("Downloading from: %s\n", url);
    
    /* Create temporary file for download */
    char temp_file[MAX_PATH];
    snprintf(temp_file, sizeof(temp_file), "/tmp/zircon-self-download-%s.tar.gz", tag);
    
    /* Download the file */
    if (download_file(url, temp_file) != 0) {
        return ERR_NETWORK;
    }
    
    printf("Download complete. Importing Zircon...\n");
    
    /* Import the downloaded archive */
    int result = cmd_self_import(temp_file);
    
    /* Clean up temp file (best effort) */
    unlink(temp_file);
    
    return result;
}

int dispatch_self_command(cli_context_t* ctx) {
    switch (ctx->data.self_cmd.subcmd) {
        case SELF_CMD_VERSION:
            cmd_version();
            return ERR_OK;
        case SELF_CMD_BUILD:
            return cmd_self_build(ctx->data.self_cmd.reference);
        case SELF_CMD_IMPORT:
            return cmd_self_import(ctx->data.self_cmd.archive);
        case SELF_CMD_INSTALL:
            return cmd_self_install(ctx->data.self_cmd.tag);
        default:
            return ERR_INVALID_ARG;
    }
}
