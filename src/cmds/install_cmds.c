#include "install_cmds.h"
#include "toolchain_cmds.h"
#include "../paths.h"
#include "../toolchains.h"
#include <curl/curl.h>

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
    
    printf("Download complete. Importing toolchain...\n");
    
    /* Use the import functionality to install the downloaded archive
     * This will compute the SHA256 hash and create a versioned name */
    cli_context_t import_ctx;
    import_ctx.command = CMD_IMPORT;
    import_ctx.data.import_cmd.archive = temp_file;
    
    int result = dispatch_import_command(&import_ctx);
    
    /* Clean up temp file (best effort) */
    unlink(temp_file);
    
    return result;
}
