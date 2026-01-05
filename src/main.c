#include "common.h"
#include "cli.h"
#include "git_utils.h"
#include "update_check.h"

int main(int argc, char** argv) {
    /* Initialize libgit2 */
    init_git();
    
    /* Check for updates (non-blocking, best effort) */
    check_for_updates();
    
    /* Parse command line arguments */
    cli_context_t* ctx = parse_args(argc, argv);
    if (!ctx) {
        shutdown_git();
        return 0;  /* Help/version was printed */
    }
    
    /* Dispatch command */
    int result = dispatch_command(ctx);
    
    /* Cleanup */
    free_cli_context(ctx);
    shutdown_git();
    
    return result;
}
