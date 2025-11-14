//! Git operations for managing source repositories

use std::path::Path;
use git2::{Repository, build::RepoBuilder, FetchOptions, RemoteCallbacks};

/// Clone a repository or open if it already exists
pub fn clone_or_open(url: &str, path: &Path) -> Result<Repository, git2::Error> {
    if path.exists() {
        Repository::open(path)
    } else {
        let mut callbacks = RemoteCallbacks::new();
        callbacks.transfer_progress(|stats| {
            if stats.received_objects() == stats.total_objects() {
                print!("\rResolving deltas {}/{}", stats.indexed_deltas(), stats.total_deltas());
            } else if stats.total_objects() > 0 {
                print!("\rReceived {}/{} objects", stats.received_objects(), stats.total_objects());
            } else {
                // No progress yet
            }
            std::io::Write::flush(&mut std::io::stdout()).ok();
            true
        });
        
        let mut fo = FetchOptions::new();
        fo.remote_callbacks(callbacks);
        
        println!("Cloning {}...", url);
        let repo = RepoBuilder::new()
            .fetch_options(fo)
            .clone(url, path)?;
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
            print!("\rResolving deltas {}/{}", stats.indexed_deltas(), stats.total_deltas());
        } else if stats.total_objects() > 0 {
            print!("\rReceived {}/{} objects", stats.received_objects(), stats.total_objects());
        } else {
            // No progress yet
        }
        std::io::Write::flush(&mut std::io::stdout()).ok();
        true
    });
    
    let mut fo = FetchOptions::new();
    fo.remote_callbacks(callbacks);
    
    println!("Fetching updates...");
    remote.fetch(&["refs/heads/*:refs/remotes/origin/*", "refs/tags/*:refs/tags/*"], Some(&mut fo), None)?;
    println!("\nFetch complete");
    Ok(())
}

/// Checkout a specific reference (branch, tag, or commit)
pub fn checkout_ref(repo: &Repository, ref_name: &str) -> Result<(), Box<dyn std::error::Error>> {
    // Try to find the reference
    let (object, reference) = if let Ok(reference) = repo.resolve_reference_from_short_name(ref_name) {
        let object = reference.peel_to_commit()?.into_object();
        (object, Some(reference))
    } else if let Ok(oid) = git2::Oid::from_str(ref_name) {
        // It might be a commit SHA
        let object = repo.find_object(oid, None)?;
        (object, None)
    } else {
        // Try as a remote branch
        let remote_ref = format!("refs/remotes/origin/{}", ref_name);
        if let Ok(reference) = repo.find_reference(&remote_ref) {
            let object = reference.peel_to_commit()?.into_object();
            (object, Some(reference))
        } else {
            // Try as a tag
            let tag_ref = format!("refs/tags/{}", ref_name);
            if let Ok(reference) = repo.find_reference(&tag_ref) {
                let object = reference.peel_to_commit()?.into_object();
                (object, Some(reference))
            } else {
                return Err(format!("Could not find reference: {}", ref_name).into());
            }
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
