//! Commands for managing toolchains

use std::error::Error;

use clap::Parser;

use crate::{cli::DispatchCommand, paths, toolchains};

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
