#include "platform.h"
#include <stdio.h>

int detect_platform_and_arch(char *platform, size_t platform_size, char *arch, size_t arch_size)
{
#if defined(__APPLE__) && defined(_M_X64)
    fprintf(stderr, "Zirco no longer builds binaries for x64 macOS, please build from source.\n");
    return -1;
#endif

#ifdef __linux__
    snprintf(platform, platform_size, "linux");
#elif defined(__APPLE__)
    snprintf(platform, platform_size, "macos");
#else
    fprintf(stderr, "Unsupported platform, only Linux and macOS are supported.\n");
    return -1;
#endif

#if defined(__x86_64__) || defined(_M_X64)
    snprintf(arch, arch_size, "x64");
#elif defined(__aarch64__) || defined(_M_ARM64)
    snprintf(arch, arch_size, "arm64");
#else
    fprintf(stderr, "Unsupported architecture, only x64 and arm64 are supported.\n");
    return -1;
#endif

    return 0;
}
