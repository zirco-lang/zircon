//! Commands for managing toolchains

use clap::Parser;
use std::error::Error;

use crate::cli::DispatchCommand;
use crate::{paths, toolchains};

/// Switch to a different installed toolchain version
#[derive(Parser)]
pub struct SwitchCmd {
    /// The version to switch to
    pub version: String,
}

impl DispatchCommand for SwitchCmd {
    fn dispatch(self) -> Result<(), Box<dyn Error>> {
        let toolchain_dir = paths::toolchain_dir(&self.version);

        if !toolchains::toolchain_exists(&self.version) {
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

        // Update zircop bin link if it exists in the toolchain
        let zircop_link = paths::zircop_binary_link();
        let zircop_binary = paths::toolchain_zircop_binary(&self.version);
        if zircop_binary.exists() {
            paths::create_link(&zircop_binary, &zircop_link)?;
        } else {
            // Remove zircop link if it exists but the new toolchain doesn't have it
            if zircop_link.exists() || zircop_link.read_link().is_ok() {
                std::fs::remove_file(&zircop_link).ok();
            }
        }

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
        let toolchains = toolchains::list_toolchains()?;

        if toolchains.is_empty() {
            println!("No toolchains installed.");
            return Ok(());
        }

        println!("Installed toolchains:");

        for tc in toolchains {
            if tc.is_current {
                println!("  {} (current)", tc.name);
            } else {
                println!("  {}", tc.name);
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
        println!("Deleting toolchain: {}", self.version);
        toolchains::delete_toolchain(&self.version)?;
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
        let to_prune = toolchains::get_prunable_toolchains()?;

        if to_prune.is_empty() {
            println!("No unused toolchains to prune.");
            return Ok(());
        }

        println!("Toolchains to be deleted:");
        for name in &to_prune {
            println!("  {}", name);
        }

        if let Some(current) = toolchains::get_current_toolchain()? {
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
            toolchains::delete_toolchain(name)?;
            println!("  ✓ Deleted {}", name);
        }

        println!("\n✓ Pruned {} toolchain(s)", to_prune.len());

        Ok(())
    }
}
