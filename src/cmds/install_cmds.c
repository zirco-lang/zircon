#include "install_cmds.h"
#include "toolchain_cmds.h"
#include "../paths.h"
#include "../toolchains.h"
#include <curl/curl.h>
#include <archive.h>
#include <archive_entry.h>

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

/* Extract tar.gz archive to destination directory */
static int extract_tarball(const char* archive_path, const char* dest_dir) {
    struct archive* a = archive_read_new();
    struct archive* ext = archive_write_disk_new();
    struct archive_entry* entry;
    int r;
    
    archive_read_support_filter_gzip(a);
    archive_read_support_format_tar(a);
    archive_write_disk_set_options(ext, ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_ACL | ARCHIVE_EXTRACT_FFLAGS);
    
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

int dispatch_install_command(cli_context_t* ctx) {
    printf("Installing %s release...\n", ctx->data.install_cmd.tag);
    
    /* Detect platform and architecture */
    char platform[32], arch[32];
    if (detect_platform_and_arch(platform, sizeof(platform), arch, sizeof(arch)) != 0) {
        return ERR_GENERAL;
    }
    
    /* Construct download URL */
    char filename[256];
    snprintf(filename, sizeof(filename), "zrc-%s-%s.tar.gz", platform, arch);
    
    char url[1024];
    snprintf(url, sizeof(url), 
             "https://github.com/zirco-lang/zrc/releases/download/%s/%s",
             ctx->data.install_cmd.tag, filename);
    
    printf("Downloading from: %s\n", url);
    
    /* Create temporary file for download */
    char temp_file[MAX_PATH];
    snprintf(temp_file, sizeof(temp_file), "/tmp/zircon-download-%s.tar.gz", ctx->data.install_cmd.tag);
    
    /* Download the file */
    if (download_file(url, temp_file) != 0) {
        return ERR_NETWORK;
    }
    
    printf("Download complete. Extracting toolchain...\n");
    
    /* Create toolchain directory with version name */
    char* tc_dir = toolchain_dir(ctx->data.install_cmd.tag);
    if (!tc_dir) {
        unlink(temp_file);
        return ERR_GENERAL;
    }
    
    /* Check if toolchain already exists */
    if (toolchain_exists(ctx->data.install_cmd.tag)) {
        fprintf(stderr, "Toolchain '%s' already exists.\n", ctx->data.install_cmd.tag);
        fprintf(stderr, "Use 'zircon delete %s' to remove it first.\n", ctx->data.install_cmd.tag);
        free(tc_dir);
        unlink(temp_file);
        return ERR_GENERAL;
    }
    
    if (make_directory_recursive(tc_dir) != 0) {
        fprintf(stderr, "Failed to create toolchain directory\n");
        free(tc_dir);
        unlink(temp_file);
        return ERR_IO;
    }
    
    /* Extract archive */
    if (extract_tarball(temp_file, tc_dir) != 0) {
        fprintf(stderr, "Failed to extract archive\n");
        remove_directory_recursive(tc_dir);
        free(tc_dir);
        unlink(temp_file);
        return ERR_GENERAL;
    }
    
    /* Clean up temp file */
    unlink(temp_file);
    
    /* Set as current toolchain */
    char* current_link = current_toolchain_link();
    if (current_link) {
        create_link(tc_dir, current_link);
        free(current_link);
    }
    
    printf("✓ Successfully installed toolchain: %s\n", ctx->data.install_cmd.tag);
    printf("  Toolchain location: %s\n", tc_dir);
    printf("\nTo use this toolchain, run:\n");
    printf("  source <(zircon env)\n");
    
    free(tc_dir);
    return ERR_OK;
}
