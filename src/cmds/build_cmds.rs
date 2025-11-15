//! Commands for building zrc toolchains

use clap::Parser;
use std::error::Error;

use crate::cli::DispatchCommand;
use crate::{build, deps, git_utils, installer, paths};

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

        // Determine version name (use tag if it's a tag, otherwise use ref + commit)
        let version = if self.reference.starts_with('v') && !self.reference.contains('/') {
            self.reference
        } else {
            format!("{}@{}", self.reference.replace('/', "-"), commit_sha)
        };

        println!("Building version: {}", version);

        // Check if cargo is available
        build::check_cargo()?;

        // Build zrc
        build::build_zrc(&source_dir)?;

        // Create toolchain directory
        let toolchain_dir = paths::toolchain_dir(&version);
        let toolchain_bin_dir = paths::toolchain_bin_dir(&version);
        let toolchain_include_dir = paths::toolchain_include_dir(&version);

        std::fs::create_dir_all(&toolchain_bin_dir)?;
        std::fs::create_dir_all(&toolchain_include_dir)?;

        // Install binary
        installer::install_zrc_binary(&source_dir, &toolchain_bin_dir)?;

        // Install include files
        installer::install_include_files(&source_dir, &toolchain_include_dir)?;

        // Update current symlink
        let current_link = paths::current_toolchain_link();
        paths::create_link(&toolchain_dir, &current_link)?;

        // Create/update bin links
        let zrc_link = paths::zrc_binary_link();
        let zrc_binary = paths::toolchain_zrc_binary(&version);
        paths::create_link(&zrc_binary, &zrc_link)?;

        // Create/update include link
        let include_link = paths::include_dir_link();
        paths::create_link(&toolchain_include_dir, &include_link)?;

        println!("\n✓ Successfully built and installed zrc {}", version);
        println!("  Toolchain location: {}", toolchain_dir.display());
        println!("\nTo use zrc, add to your PATH:");
        println!("  export PATH=\"{}:$PATH\"", paths::bin_dir().display());
        println!("\nOr run:");
        println!("  source <(zircon env)");

        Ok(())
    }
}
