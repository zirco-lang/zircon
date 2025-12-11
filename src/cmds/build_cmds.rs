//! Commands for building zrc toolchains

use std::{error::Error, process::Command};

use clap::Parser;

use crate::{cli::DispatchCommand, deps, git_utils, paths};

/// Build a specific version of zrc
#[derive(Parser)]
pub struct BuildCmd {
    /// The git reference to build (branch, tag, or commit)
    pub reference: String,

    /// Custom zrc repository URL
    #[arg(
        long = "zrc-repo",
        default_value = "https://github.com/zirco-lang/zrc.git"
    )]
    pub repo_url: String,
}

impl DispatchCommand for BuildCmd {
    fn dispatch(self) -> Result<(), Box<dyn Error>> {
        // Check dependencies first
        deps::warn_dependencies();

        // Ensure directories exist
        paths::ensure_directories()?;

        let source_dir = paths::zrc_source_dir();

        // Clone or open repository
        let repo = git_utils::clone_or_open(&self.repo_url, &source_dir)?;

        // Fetch latest changes
        git_utils::fetch(&repo)?;

        // Checkout the requested reference
        git_utils::checkout_ref(&repo, &self.reference)?;

        // Get commit SHA for version naming
        let commit_sha = git_utils::get_current_commit_short(&repo)?;

        // Determine reference type and create appropriate version name
        let ref_type = git_utils::determine_ref_type(&repo, &self.reference);
        let version = match ref_type {
            git_utils::RefType::Tag(tag) => tag,
            git_utils::RefType::Branch(branch) => {
                format!("{}@{}", branch.replace('/', "-"), commit_sha)
            }
            git_utils::RefType::Commit(commit) => commit, // No prefix for commits
        };

        println!("Building version: {}", version);

        // Create toolchain directory
        let toolchain_dir = paths::toolchain_dir(&version);
        std::fs::create_dir_all(&toolchain_dir)?;

        // Execute the hook script from the zrc repo
        // The hook handles building and installing to the toolchain directory
        let hook_script = source_dir.join("hooks").join("zircon.sh");
        if !hook_script.exists() {
            return Err(format!(
                "Hook script not found at {}. This version of zrc may not support zircon hooks.",
                hook_script.display()
            )
            .into());
        }

        println!("Running zrc build hook...");
        let status = Command::new("bash")
            .arg(&hook_script)
            .env("ZIRCON_TOOLCHAIN_DIR", &toolchain_dir)
            .current_dir(&source_dir)
            .status()?;

        if !status.success() {
            let exit_code = status.code().unwrap_or(-1);
            return Err(format!("Hook script failed (exit code: {})", exit_code).into());
        }

        // Update current symlink
        let current_link = paths::current_toolchain_link();
        paths::create_link(&toolchain_dir, &current_link)?;

        println!("\n✓ Successfully built and installed zrc {}", version);
        println!("  Toolchain location: {}", toolchain_dir.display());
        println!("\nTo use zrc, run:");
        println!("  source <(zircon env)");

        Ok(())
    }
}
