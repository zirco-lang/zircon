//! Auto-update checker for Zircon

use std::fs;
use std::path::PathBuf;
use std::time::{Duration, SystemTime};

/// Check if we should remind the user to update Zircon
/// This checks every 7 days and prints a reminder
pub fn check_for_updates() {
    // Don't block or fail on errors - this is just a helpful reminder
    if let Err(_e) = try_check_for_updates() {
        // Silently ignore errors in update check
    }
}

/// Internal function that does the actual checking
fn try_check_for_updates() -> Result<(), Box<dyn std::error::Error>> {
    let update_check_file = get_update_check_file()?;

    // Check if we should remind based on last check time
    let should_remind = if update_check_file.exists() {
        fs::metadata(&update_check_file)
            .ok()
            .and_then(|metadata| metadata.modified().ok())
            .and_then(|modified| SystemTime::now().duration_since(modified).ok())
            .map_or(true, |elapsed| {
                // Remind every 7 days
                elapsed > Duration::from_secs(7 * 24 * 60 * 60)
            })
    } else {
        // First time, create the file and don't show reminder yet
        fs::write(&update_check_file, "")?;
        false
    };

    if should_remind {
        println!("💡 Reminder: Update Zircon with 'zircon self update'");
        println!();

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
