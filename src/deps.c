#include "deps.h"
#include "config.h"

static int run_command_get_output(const char* cmd, char* output, size_t output_size) {
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return -1;
    
    if (fgets(output, output_size, pipe) != NULL) {
        /* Remove trailing newline */
        size_t len = strlen(output);
        if (len > 0 && output[len - 1] == '\n') {
            output[len - 1] = '\0';
        }
    }
    
    int status = pclose(pipe);
    return WIFEXITED(status) && WEXITSTATUS(status) == 0 ? 0 : -1;
}

int check_llvm(char** version_out) {
    const char* candidates[] = {
        "llvm-config",
        "llvm-config-" REQUIRED_LLVM_VERSION,
        "llvm-config-mp-" REQUIRED_LLVM_VERSION,
        "/usr/local/opt/llvm@" REQUIRED_LLVM_VERSION "/bin/llvm-config",
        "/usr/local/opt/llvm/bin/llvm-config",
        "/opt/homebrew/opt/llvm@" REQUIRED_LLVM_VERSION "/bin/llvm-config",
        "/opt/homebrew/opt/llvm/bin/llvm-config",
    };
    
    for (size_t i = 0; i < ARRAY_SIZE(candidates); i++) {
        char cmd[MAX_CMD];
        snprintf(cmd, sizeof(cmd), "%s --version 2>/dev/null", candidates[i]);
        
        char version[256];
        if (run_command_get_output(cmd, version, sizeof(version)) == 0) {
            /* Check if it's LLVM 20.x.x */
            if (string_starts_with(version, REQUIRED_LLVM_VERSION ".")) {
                if (version_out) {
                    *version_out = string_duplicate(version);
                }
                return 0;
            }
            
            /* Found LLVM but wrong version */
            if (version[0] != '\0') {
                fprintf(stderr, "⚠ Found LLVM %s at '%s', but Zirco requires %s\n",
                        version, candidates[i], LLVM_VERSION_DESC);
            }
        }
    }
    
    fprintf(stderr, "%s not found. Zirco REQUIRES %s specifically.\n",
            LLVM_VERSION_DESC, LLVM_VERSION_DESC);
    fprintf(stderr, "  Consider using `llvmenv` to compile an appropriate version of LLVM.\n");
    fprintf(stderr, "  Please note that the LLVM binary distributions are NOT supported due to missing components.\n");
    
    return -1;
}

int check_clang(char** version_out) {
    const char* candidates[] = {
        "clang",
        "clang-" REQUIRED_LLVM_VERSION,
        "clang-mp-" REQUIRED_LLVM_VERSION,
        "/usr/local/opt/llvm@" REQUIRED_LLVM_VERSION "/bin/clang",
        "/usr/local/opt/llvm/bin/clang",
        "/opt/homebrew/opt/llvm@" REQUIRED_LLVM_VERSION "/bin/clang",
        "/opt/homebrew/opt/llvm/bin/clang",
    };
    
    for (size_t i = 0; i < ARRAY_SIZE(candidates); i++) {
        char cmd[MAX_CMD];
        snprintf(cmd, sizeof(cmd), "%s --version 2>/dev/null", candidates[i]);
        
        char version[1024];
        if (run_command_get_output(cmd, version, sizeof(version)) == 0) {
            if (version_out) {
                *version_out = string_duplicate(version);
            }
            return 0;
        }
    }
    
    fprintf(stderr, "clang not found. Please install clang\n");
    return -1;
}

int check_dependencies_strict(void) {
    printf("Checking dependencies...\n");
    
    char* llvm_version = NULL;
    if (check_llvm(&llvm_version) != 0) {
        return -1;
    }
    printf("✓ %s found: %s\n", LLVM_VERSION_DESC, llvm_version ? llvm_version : "");
    SAFE_FREE(llvm_version);
    
    char* clang_version = NULL;
    if (check_clang(&clang_version) != 0) {
        return -1;
    }
    printf("✓ clang found: %s\n", clang_version ? clang_version : "");
    SAFE_FREE(clang_version);
    
    return 0;
}
