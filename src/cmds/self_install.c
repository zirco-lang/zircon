#include "self_install.h"
#include "install.h"
#include "import.h"
#include "self_import.h"
#include "../common.h"
#include "../toolchains.h"
#include "../paths.h"
#include "../platform.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int cmd_self_install(const cli_context_t *ctx)
{
    const char *tag = ctx->data.self_install.tag;
    printf("Installing Zircon %s release...\n", tag);

    /* Detect platform and architecture */
    char platform[32], arch[32];
    if (detect_platform_and_arch(platform, sizeof(platform), arch, sizeof(arch)) != 0)
    {
        return EXIT_GENERAL;
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
    if (download_file(url, temp_file) != 0)
    {
        return EXIT_GENERAL;
    }

    printf("Download complete. Importing Zircon...\n");

    /* Import the downloaded archive */
    cli_context_t import_ctx;
    import_ctx.command = CMD_SELF_IMPORT;
    import_ctx.data.self_import.archive_path = temp_file;
    int result = cmd_self_import(&import_ctx);

    /* Clean up temp file (best effort) */
    unlink(temp_file);

    return result;
}
