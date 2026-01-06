#include "install.h"
#include "import.h"
#include "../common.h"
#include "../toolchains.h"
#include "../paths.h"
#include "../platform.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Callback for curl to write data to file */
static size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    return fwrite(ptr, size, nmemb, stream);
}

/* Download file from URL to destination path */
int download_file(const char *url, const char *dest_path)
{
    CURL *curl = curl_easy_init();
    if (!curl)
    {
        fprintf(stderr, "Failed to initialize curl\n");
        return -1;
    }

    FILE *fp = fopen(dest_path, "wb");
    if (!fp)
    {
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

    if (res != CURLE_OK)
    {
        fprintf(stderr, "Failed to download file: %s (HTTP %ld)\n", curl_easy_strerror(res), http_code);
        fprintf(stderr, "The release may not be available or may not have pre-built binaries for your platform.\n");
        unlink(dest_path);
        return -1;
    }

    return 0;
}

int cmd_install(const cli_context_t *ctx)
{
    printf("Installing %s release...\n", ctx->data.install.tag);

    /* Detect platform and architecture */
    char platform[32], arch[32];
    if (detect_platform_and_arch(platform, sizeof(platform), arch, sizeof(arch)) != 0)
    {
        return EXIT_GENERAL;
    }

    /* Construct download URL */
    char filename[256];
    snprintf(filename, sizeof(filename), "zrc-%s-%s.tar.gz", platform, arch);

    char url[1024];
    snprintf(url, sizeof(url),
             "https://github.com/zirco-lang/zrc/releases/download/%s/%s",
             ctx->data.install.tag, filename);

    printf("Downloading from: %s\n", url);

    /* Create temporary file for download */
    char temp_file[MAX_PATH];
    snprintf(temp_file, sizeof(temp_file), "/tmp/%s", filename);

    /* Download the file */
    if (download_file(url, temp_file) != 0)
    {
        return EXIT_GENERAL;
    }

    printf("Download complete. Importing toolchain...\n");

    /* Use the import functionality to install the downloaded archive
     * This will compute the SHA256 hash and create a versioned name */
    cli_context_t import_ctx;
    import_ctx.command = CMD_IMPORT;
    import_ctx.data.import.archive_path = temp_file;

    int result = cmd_import(&import_ctx);

    /* Clean up temp file (best effort) */
    unlink(temp_file);

    return result;
}
