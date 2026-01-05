#ifndef ZIRCON_GIT_UTILS_H
#define ZIRCON_GIT_UTILS_H

#include "common.h"
#include <git2.h>

/* Clone a repository or open an existing one */
int clone_or_open(const char* url, const char* path, git_repository** repo_out);

/* Fetch updates from remote */
int fetch_repo(git_repository* repo);

/* Checkout a specific reference (branch, tag, or commit) */
int checkout_ref(git_repository* repo, const char* ref_name);

/* Get the current HEAD commit SHA (short version) */
char* get_current_commit_short(git_repository* repo);

/* Reference type for better version naming */
typedef enum {
    REF_TYPE_TAG,
    REF_TYPE_BRANCH,
    REF_TYPE_COMMIT
} ref_type_t;

typedef struct {
    ref_type_t type;
    char* name;
} ref_info_t;

/* Determine the type of reference and get appropriate version name */
ref_info_t* determine_ref_type(git_repository* repo, const char* ref_name);

/* Free ref_info */
void free_ref_info(ref_info_t* info);

/* Initialize libgit2 */
void init_git(void);

/* Shutdown libgit2 */
void shutdown_git(void);

#endif /* ZIRCON_GIT_UTILS_H */
