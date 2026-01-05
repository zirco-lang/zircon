#include "git_utils.h"

void init_git(void) {
    git_libgit2_init();
}

void shutdown_git(void) {
    git_libgit2_shutdown();
}

static int transfer_progress_callback(const git_indexer_progress *stats, void *payload) {
    (void)payload;
    if (stats->received_objects == stats->total_objects) {
        fprintf(stderr, "\rResolving deltas %u/%u", stats->indexed_deltas, stats->total_deltas);
    } else if (stats->total_objects > 0) {
        fprintf(stderr, "\rReceived %u/%u objects", stats->received_objects, stats->total_objects);
    }
    fflush(stderr);
    return 0;
}

int clone_or_open(const char* url, const char* path, git_repository** repo_out) {
    if (path_exists(path)) {
        return git_repository_open(repo_out, path);
    }
    
    git_clone_options clone_opts = GIT_CLONE_OPTIONS_INIT;
    clone_opts.fetch_opts.callbacks.transfer_progress = transfer_progress_callback;
    
    fprintf(stderr, "Cloning %s...\n", url);
    int error = git_clone(repo_out, url, path, &clone_opts);
    if (error == 0) {
        fprintf(stderr, "\nClone complete\n");
    }
    return error;
}

int fetch_repo(git_repository* repo) {
    git_remote* remote = NULL;
    int error = git_remote_lookup(&remote, repo, "origin");
    if (error != 0) return error;
    
    git_fetch_options fetch_opts = GIT_FETCH_OPTIONS_INIT;
    fetch_opts.callbacks.transfer_progress = transfer_progress_callback;
    
    const char* refspecs[] = {
        "refs/heads/*:refs/remotes/origin/*",
        "refs/tags/*:refs/tags/*"
    };
    git_strarray refspec_array = { (char**)refspecs, 2 };
    
    fprintf(stderr, "Fetching updates...\n");
    error = git_remote_fetch(remote, &refspec_array, &fetch_opts, NULL);
    if (error == 0) {
        fprintf(stderr, "\nFetch complete\n");
    }
    
    git_remote_free(remote);
    return error;
}

int checkout_ref(git_repository* repo, const char* ref_name) {
    git_object* obj = NULL;
    git_reference* ref = NULL;
    int error = 0;
    
    /* Try remote branch first */
    char remote_ref[MAX_PATH];
    snprintf(remote_ref, sizeof(remote_ref), "refs/remotes/origin/%s", ref_name);
    error = git_reference_lookup(&ref, repo, remote_ref);
    
    if (error != 0) {
        /* Try short name resolution */
        error = git_reference_dwim(&ref, repo, ref_name);
    }
    
    if (error != 0) {
        /* Try as commit SHA */
        git_oid oid;
        if (git_oid_fromstr(&oid, ref_name) == 0) {
            error = git_object_lookup(&obj, repo, &oid, GIT_OBJECT_COMMIT);
        } else {
            return error;
        }
    } else {
        error = git_reference_peel(&obj, ref, GIT_OBJECT_COMMIT);
    }
    
    if (error != 0) {
        if (ref) git_reference_free(ref);
        return error;
    }
    
    error = git_checkout_tree(repo, obj, NULL);
    if (error != 0) {
        git_object_free(obj);
        if (ref) git_reference_free(ref);
        return error;
    }
    
    if (ref) {
        error = git_repository_set_head(repo, git_reference_name(ref));
        git_reference_free(ref);
    } else {
        error = git_repository_set_head_detached(repo, git_object_id(obj));
    }
    
    git_object_free(obj);
    
    if (error == 0) {
        fprintf(stderr, "Checked out: %s\n", ref_name);
    }
    
    return error;
}

char* get_current_commit_short(git_repository* repo) {
    git_reference* head = NULL;
    if (git_repository_head(&head, repo) != 0) return NULL;
    
    git_commit* commit = NULL;
    if (git_reference_peel((git_object**)&commit, head, GIT_OBJECT_COMMIT) != 0) {
        git_reference_free(head);
        return NULL;
    }
    
    const git_oid* oid = git_commit_id(commit);
    char oid_str[GIT_OID_HEXSZ + 1];
    git_oid_tostr(oid_str, sizeof(oid_str), oid);
    
    char* result = malloc(9);
    if (result) {
        memcpy(result, oid_str, 8);
        result[8] = '\0';
    }
    
    git_commit_free(commit);
    git_reference_free(head);
    
    return result;
}

ref_info_t* determine_ref_type(git_repository* repo, const char* ref_name) {
    ref_info_t* info = malloc(sizeof(ref_info_t));
    if (!info) return NULL;
    
    /* Try as tag */
    char tag_ref[MAX_PATH];
    snprintf(tag_ref, sizeof(tag_ref), "refs/tags/%s", ref_name);
    git_reference* ref = NULL;
    if (git_reference_lookup(&ref, repo, tag_ref) == 0) {
        info->type = REF_TYPE_TAG;
        info->name = string_duplicate(ref_name);
        git_reference_free(ref);
        return info;
    }
    
    /* Try as branch */
    char branch_ref[MAX_PATH];
    snprintf(branch_ref, sizeof(branch_ref), "refs/heads/%s", ref_name);
    if (git_reference_lookup(&ref, repo, branch_ref) == 0) {
        info->type = REF_TYPE_BRANCH;
        info->name = string_duplicate(ref_name);
        git_reference_free(ref);
        return info;
    }
    
    snprintf(branch_ref, sizeof(branch_ref), "refs/remotes/origin/%s", ref_name);
    if (git_reference_lookup(&ref, repo, branch_ref) == 0) {
        info->type = REF_TYPE_BRANCH;
        info->name = string_duplicate(ref_name);
        git_reference_free(ref);
        return info;
    }
    
    /* Try as commit */
    git_oid oid;
    if (git_oid_fromstr(&oid, ref_name) == 0) {
        git_commit* commit = NULL;
        if (git_commit_lookup(&commit, repo, &oid) == 0) {
            char short_sha[9];
            memcpy(short_sha, ref_name, 8);
            short_sha[8] = '\0';
            info->type = REF_TYPE_COMMIT;
            info->name = string_duplicate(short_sha);
            git_commit_free(commit);
            return info;
        }
    }
    
    /* Default to branch */
    info->type = REF_TYPE_BRANCH;
    info->name = string_duplicate(ref_name);
    return info;
}

void free_ref_info(ref_info_t* info) {
    if (info) {
        SAFE_FREE(info->name);
        free(info);
    }
}
