#include "env.h"
#include "../toolchains.h"
#include "../common.h"
#include "../paths.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *detect_shell(void)
{
    char *shell_env = getenv("SHELL");
    if (shell_env)
    {
        if (strstr(shell_env, "fish"))
            return "fish";
        if (strstr(shell_env, "zsh"))
            return "zsh";
        if (strstr(shell_env, "bash"))
            return "bash";
    }
    return "sh";
}

static void escape_for_posix_shell(const char *path, char *output, size_t size)
{
    size_t j = 0;
    if (j < size - 1)
        output[j++] = '\'';
    for (size_t i = 0; path[i] && j < size - 4; i++)
    {
        if (path[i] == '\'')
        {
            // End quote, add escaped quote, restart quote: '\''
            output[j++] = '\'';
            output[j++] = '\\';
            output[j++] = '\'';
            output[j++] = '\'';
        }
        else
        {
            output[j++] = path[i];
        }
    }
    if (j < size - 1)
        output[j++] = '\'';
    output[j] = '\0';
}

int cmd_env(const cli_context_t *ctx)
{
    (void)ctx;

    char *bin = get_self_bin_dir();
    if (!bin)
        return EXIT_GENERAL;

    char *shell = detect_shell();

    char escaped[MAX_CMD];
    escape_for_posix_shell(bin, escaped, sizeof(escaped));

    if (strcmp(shell, "fish") == 0)
        printf("set -gx PATH %s $PATH;\n", escaped);
    else
        printf("export PATH=%s:$PATH;\n", escaped);

    char *env_sh = path_join(get_current_toolchain_link_path(), "env.sh");
    if (env_sh && path_exists(env_sh))
    {
        char escaped_env[MAX_CMD];
        escape_for_posix_shell(env_sh, escaped_env, sizeof(escaped_env));
        printf("source %s;\n", escaped_env);
    }

    if (env_sh)
        free(env_sh);
    free(bin);
    return EXIT_SUCCESS;
}
