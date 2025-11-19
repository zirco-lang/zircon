//! Git operations for repository management

use git2::{FetchOptions, RemoteCallbacks, Repository, build::RepoBuilder};

/// Clone a repository or open an existing one
pub fn clone_or_open(url: &str, path: &std::path::Path) -> Result<Repository, git2::Error> {
    if path.exists() {
        // Open existing repository
        Repository::open(path)
    } else {
        // Clone with progress reporting
        let mut callbacks = RemoteCallbacks::new();
        callbacks.transfer_progress(|stats| {
            if stats.received_objects() == stats.total_objects() {
                print!(
                    "\rResolving deltas {}/{}",
                    stats.indexed_deltas(),
                    stats.total_deltas()
                );
            } else if stats.total_objects() > 0 {
                print!(
                    "\rReceived {}/{} objects",
                    stats.received_objects(),
                    stats.total_objects()
                );
            } else {
                // No progress yet
            }
            std::io::Write::flush(&mut std::io::stdout()).ok();
            true
        });

        let mut fo = FetchOptions::new();
        fo.remote_callbacks(callbacks);

        println!("Cloning {}...", url);
        let repo = RepoBuilder::new().fetch_options(fo).clone(url, path)?;
        println!("\nClone complete");
        Ok(repo)
    }
}

/// Fetch updates from remote
pub fn fetch(repo: &Repository) -> Result<(), git2::Error> {
    let mut remote = repo.find_remote("origin")?;

    let mut callbacks = RemoteCallbacks::new();
    callbacks.transfer_progress(|stats| {
        if stats.received_objects() == stats.total_objects() {
            print!(
                "\rResolving deltas {}/{}",
                stats.indexed_deltas(),
                stats.total_deltas()
            );
        } else if stats.total_objects() > 0 {
            print!(
                "\rReceived {}/{} objects",
                stats.received_objects(),
                stats.total_objects()
            );
        } else {
            // No progress yet
        }
        std::io::Write::flush(&mut std::io::stdout()).ok();
        true
    });

    let mut fo = FetchOptions::new();
    fo.remote_callbacks(callbacks);

    println!("Fetching updates...");
    remote.fetch(
        &[
            "refs/heads/*:refs/remotes/origin/*",
            "refs/tags/*:refs/tags/*",
        ],
        Some(&mut fo),
        None,
    )?;
    println!("\nFetch complete");
    Ok(())
}

/// Checkout a specific reference (branch, tag, or commit)
///
/// This function checks references in the following order:
/// 1. Remote branch (`refs/remotes/origin/{ref_name}`) - ensures we use latest after fetch
/// 2. Short name resolution (tags, local branches) - handled by git2
/// 3. Commit SHA - direct object lookup
pub fn checkout_ref(repo: &Repository, ref_name: &str) -> Result<(), Box<dyn std::error::Error>> {
    let (object, reference) = {
        // First, try as a remote branch to ensure we use the latest fetched version.
        // This is important because resolve_reference_from_short_name prefers local
        // branches, which may be outdated even after fetching.
        let remote_ref = format!("refs/remotes/origin/{}", ref_name);
        if let Ok(reference) = repo.find_reference(&remote_ref) {
            let object = reference.peel_to_commit()?.into_object();
            (object, Some(reference))
        } else if let Ok(reference) = repo.resolve_reference_from_short_name(ref_name) {
            // Try short name resolution (handles tags, local branches, etc.)
            let object = reference.peel_to_commit()?.into_object();
            (object, Some(reference))
        } else if let Ok(oid) = git2::Oid::from_str(ref_name) {
            // Try as a direct commit SHA
            let object = repo.find_object(oid, None)?;
            (object, None)
        } else {
            return Err(format!("Could not find reference: {}", ref_name).into());
        }
    };

    repo.checkout_tree(&object, None)?;

    match reference {
        Some(gref) => repo.set_head(gref.name().ok_or("Invalid reference name")?),
        None => repo.set_head_detached(object.id()),
    }?;

    println!("Checked out: {}", ref_name);
    Ok(())
}

/// Get the current HEAD commit SHA (short version)
pub fn get_current_commit_short(repo: &Repository) -> Result<String, git2::Error> {
    let head = repo.head()?;
    let commit = head.peel_to_commit()?;
    let oid = commit.id();
    Ok(oid.to_string()[..8].to_string())
}

/// Reference type for better version naming
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum RefType {
    /// Semantic version tag (e.g., v0.1.0)
    Tag(String),
    /// Branch name
    Branch(String),
    /// Direct commit SHA
    Commit(String),
}

/// Determine the type of reference and get appropriate version name
pub fn determine_ref_type(repo: &Repository, ref_name: &str) -> RefType {
    // First, try to find it as a tag
    let tag_ref = format!("refs/tags/{}", ref_name);
    if repo.find_reference(&tag_ref).is_ok() {
        // It's a tag
        return RefType::Tag(ref_name.to_string());
    }

    // Try as a branch (local or remote)
    let local_branch = format!("refs/heads/{}", ref_name);
    let remote_branch = format!("refs/remotes/origin/{}", ref_name);
    if repo.find_reference(&local_branch).is_ok() || repo.find_reference(&remote_branch).is_ok() {
        return RefType::Branch(ref_name.to_string());
    }

    // Try to parse as commit SHA
    if let Ok(oid) = git2::Oid::from_str(ref_name)
        && repo.find_commit(oid).is_ok()
    {
        // Return short commit hash without "commit-" prefix
        return RefType::Commit(ref_name[..8.min(ref_name.len())].to_string());
    }

    // If we can resolve it via short name, check what it resolves to
    if let Ok(reference) = repo.resolve_reference_from_short_name(ref_name)
        && let Some(ref_name_str) = reference.name()
    {
        if ref_name_str.starts_with("refs/tags/") {
            return RefType::Tag(ref_name.to_string());
        }
        if ref_name_str.starts_with("refs/heads/") || ref_name_str.starts_with("refs/remotes/") {
            return RefType::Branch(ref_name.to_string());
        }
    }

    // Default to treating it as a branch name
    RefType::Branch(ref_name.to_string())
}
