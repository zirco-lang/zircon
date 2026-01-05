#include "env_cmds.h"
#include "../paths.h"

static char* detect_shell(void) {
    char* shell_env = getenv("SHELL");
    if (shell_env) {
        if (strstr(shell_env, "fish")) return "fish";
        if (strstr(shell_env, "zsh")) return "zsh";
        if (strstr(shell_env, "bash")) return "bash";
    }
    return "sh";
}

static void escape_for_posix_shell(const char* path, char* output, size_t size) {
    snprintf(output, size, "'%s'", path);
}

int dispatch_env_command(cli_context_t* ctx) {
    char* bin = bin_dir();
    if (!bin) return ERR_GENERAL;
    
    char* shell = ctx->data.env_cmd.shell ? ctx->data.env_cmd.shell : detect_shell();
    
    char escaped[MAX_PATH * 2];
    escape_for_posix_shell(bin, escaped, sizeof(escaped));
    
    if (strcmp(shell, "fish") == 0) {
        printf("set -gx PATH %s $PATH;\n", escaped);
    } else {
        printf("export PATH=%s:$PATH;\n", escaped);
    }
    
    char* env_sh = current_toolchain_env_sh();
    if (env_sh && path_exists(env_sh)) {
        char escaped_env[MAX_PATH * 2];
        escape_for_posix_shell(env_sh, escaped_env, sizeof(escaped_env));
        printf("source %s;\n", escaped_env);
    }
    SAFE_FREE(env_sh);
    
    free(bin);
    return ERR_OK;
}
