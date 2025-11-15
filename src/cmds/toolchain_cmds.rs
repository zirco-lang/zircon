//! Commands for managing toolchains

use clap::Parser;
use std::error::Error;
use std::fs;

use crate::cli::DispatchCommand;
use crate::paths;

/// Switch to a different installed toolchain version
#[derive(Parser)]
pub struct SwitchCmd {
    /// The version to switch to
    pub version: String,
}

impl DispatchCommand for SwitchCmd {
    fn dispatch(self) -> Result<(), Box<dyn Error>> {
        let toolchain_dir = paths::toolchain_dir(&self.version);

        if !toolchain_dir.exists() {
            return Err(format!(
                "Toolchain '{}' not found at {}\nUse 'zircon build {}' to install it.",
                self.version,
                toolchain_dir.display(),
                self.version
            )
            .into());
        }

        // Update current symlink
        let current_link = paths::current_toolchain_link();
        paths::create_link(&toolchain_dir, &current_link)?;

        // Update bin links
        let zrc_link = paths::zrc_binary_link();
        let zrc_binary = paths::toolchain_zrc_binary(&self.version);
        paths::create_link(&zrc_binary, &zrc_link)?;

        // Update include link
        let include_link = paths::include_dir_link();
        let toolchain_include_dir = paths::toolchain_include_dir(&self.version);
        paths::create_link(&toolchain_include_dir, &include_link)?;

        println!("✓ Switched to toolchain: {}", self.version);

        Ok(())
    }
}

/// List installed toolchains
#[derive(Parser)]
pub struct ListCmd;

impl DispatchCommand for ListCmd {
    fn dispatch(self) -> Result<(), Box<dyn Error>> {
        let toolchains_dir = paths::toolchains_dir();

        if !toolchains_dir.exists() {
            println!("No toolchains installed.");
            return Ok(());
        }

        // Get current toolchain if it exists
        let current_link = paths::current_toolchain_link();
        let current_version = if current_link.exists() {
            fs::read_link(&current_link)
                .ok()
                .and_then(|p| p.file_name().map(|n| n.to_string_lossy().to_string()))
        } else {
            None
        };

        println!("Installed toolchains:");

        let mut entries: Vec<_> = fs::read_dir(&toolchains_dir)?
            .filter_map(Result::ok)
            .filter(|e| {
                // Skip the "current" symlink
                e.file_name() != "current" && e.path().is_dir()
            })
            .collect();

        if entries.is_empty() {
            println!("  (none)");
            return Ok(());
        }

        // Sort by name
        entries.sort_by_key(fs::DirEntry::file_name);

        for entry in entries {
            let name = entry.file_name().to_string_lossy().to_string();
            let is_current = current_version.as_ref() == Some(&name);

            if is_current {
                println!("  {} (current)", name);
            } else {
                println!("  {}", name);
            }
        }

        Ok(())
    }
}

/// Delete a specific toolchain
#[derive(Parser)]
pub struct DeleteCmd {
    /// The version to delete
    pub version: String,
}

impl DispatchCommand for DeleteCmd {
    fn dispatch(self) -> Result<(), Box<dyn Error>> {
        let toolchain_dir = paths::toolchain_dir(&self.version);

        if !toolchain_dir.exists() {
            return Err(format!("Toolchain '{}' not found.", self.version).into());
        }

        // Check if this is the current toolchain
        let current_link = paths::current_toolchain_link();
        if current_link.exists()
            && let Ok(target) = fs::read_link(&current_link)
            && let Some(current_name) = target.file_name()
            && current_name == self.version.as_str()
        {
            return Err(format!(
                    "Cannot delete '{}' because it is the current toolchain.\nSwitch to another toolchain first with 'zircon switch <version>'.",
                    self.version
                ).into());
        }

        println!("Deleting toolchain: {}", self.version);
        fs::remove_dir_all(&toolchain_dir)?;
        println!("✓ Toolchain '{}' deleted", self.version);

        Ok(())
    }
}

/// Remove unused toolchains (keep only current)
#[derive(Parser)]
pub struct PruneCmd {
    /// Skip confirmation prompt
    #[arg(short = 'y', long = "yes")]
    yes: bool,
}

impl DispatchCommand for PruneCmd {
    fn dispatch(self) -> Result<(), Box<dyn Error>> {
        let toolchains_dir = paths::toolchains_dir();

        if !toolchains_dir.exists() {
            println!("No toolchains directory found.");
            return Ok(());
        }

        // Get current toolchain
        let current_link = paths::current_toolchain_link();
        let current_version = if current_link.exists() {
            fs::read_link(&current_link)
                .ok()
                .and_then(|p| p.file_name().map(|n| n.to_string_lossy().to_string()))
        } else {
            None
        };

        // Find toolchains to prune
        let mut to_prune: Vec<String> = fs::read_dir(&toolchains_dir)?
            .filter_map(Result::ok)
            .filter(|e| {
                let name = e.file_name().to_string_lossy().to_string();
                // Skip the "current" symlink and the current version
                name != "current" && e.path().is_dir() && Some(&name) != current_version.as_ref()
            })
            .map(|e| e.file_name().to_string_lossy().to_string())
            .collect();

        if to_prune.is_empty() {
            println!("No unused toolchains to prune.");
            return Ok(());
        }

        to_prune.sort();

        println!("Toolchains to be deleted:");
        for name in &to_prune {
            println!("  {}", name);
        }

        if let Some(ref current) = current_version {
            println!("\nCurrent toolchain '{}' will be kept.", current);
        }

        if !self.yes {
            println!("\nProceed with deletion? (y/N): ");
            let mut input = String::new();
            std::io::stdin().read_line(&mut input)?;
            let input = input.trim().to_lowercase();

            if input != "y" && input != "yes" {
                println!("Cancelled.");
                return Ok(());
            }
        }

        println!("\nDeleting toolchains...");
        for name in &to_prune {
            let tc_dir = paths::toolchain_dir(name);
            fs::remove_dir_all(&tc_dir)?;
            println!("  ✓ Deleted {}", name);
        }

        println!("\n✓ Pruned {} toolchain(s)", to_prune.len());

        Ok(())
    }
}
