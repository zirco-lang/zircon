//! Auto-update checker for Zircon

use std::fs;
use std::path::PathBuf;
use std::time::{Duration, SystemTime};

/// Check if we should remind the user to update Zircon
/// Checks once daily if the main branch has moved forward
pub fn check_for_updates() {
    // Don't block or fail on errors - this is just a helpful reminder
    if let Err(_e) = try_check_for_updates() {
        // Silently ignore errors in update check
    }
}

/// Internal function that does the actual checking
fn try_check_for_updates() -> Result<(), Box<dyn std::error::Error>> {
    let update_check_file = get_update_check_file()?;

    // Check if we should check based on last check time (once per day)
    let should_check = if update_check_file.exists() {
        fs::metadata(&update_check_file)
            .ok()
            .and_then(|metadata| metadata.modified().ok())
            .and_then(|modified| SystemTime::now().duration_since(modified).ok())
            .is_none_or(|elapsed| {
                // Check once per day
                elapsed > Duration::from_secs(24 * 60 * 60)
            })
    } else {
        // First time, create the file and check
        true
    };

    if should_check {
        // Try to check if zircon sources exist and main has updates
        let zircon_source = crate::paths::zircon_source_dir();

        if zircon_source.exists() {
            // Try to open the repository and check if main has moved forward
            if let Ok(repo) = git2::Repository::open(&zircon_source) {
                // Get current HEAD commit
                if let Ok(head) = repo.head()
                    && let Ok(local_commit) = head.peel_to_commit()
                {
                    let local_oid = local_commit.id();

                    // Fetch from origin (silently, don't show errors)
                    drop(crate::git_utils::fetch(&repo));

                    // Check origin/main
                    if let Ok(remote_ref) = repo.find_reference("refs/remotes/origin/main")
                        && let Ok(remote_commit) = remote_ref.peel_to_commit()
                    {
                        let remote_oid = remote_commit.id();

                        // Check if remote is ahead
                        if local_oid != remote_oid {
                            // Check if local is ancestor of remote (remote is ahead)
                            if repo.graph_descendant_of(remote_oid, local_oid) == Ok(true) {
                                println!(
                                    "💡 Zircon update available! Run 'zircon self update' to update."
                                );
                                println!();
                            }
                        }
                    }
                }
            }
        }

        // Update the timestamp
        fs::write(&update_check_file, "")?;
    }

    Ok(())
}

/// Get the path to the update check file
fn get_update_check_file() -> Result<PathBuf, Box<dyn std::error::Error>> {
    let zircon_root = crate::paths::zircon_root();
    fs::create_dir_all(&zircon_root)?;
    Ok(zircon_root.join(".last_update_check"))
}
