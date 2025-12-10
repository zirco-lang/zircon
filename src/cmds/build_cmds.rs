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

        // Check if cargo is available
        build::check_cargo()?;

        // Build zrc
        build::build_zrc(&source_dir)?;

        // Create toolchain directory
        let toolchain_dir = paths::toolchain_dir(&version);
        std::fs::create_dir_all(&toolchain_dir)?;

        // Check if hook/install.sh exists
        let install_hook = source_dir.join("hook").join("install.sh");
        if install_hook.exists() {
            println!("Running install hook...");
            // Call hook/install.sh with CWD in source_dir and pass TOOLCHAIN_DIR
            let status = std::process::Command::new("bash")
                .arg(&install_hook)
                .current_dir(&source_dir)
                .env("TOOLCHAIN_DIR", &toolchain_dir)
                .status()?;

            if !status.success() {
                let exit_code = status.code().unwrap_or(-1);
                return Err(format!("Install hook failed (exit code: {})", exit_code).into());
            }
        } else {
            // Fallback to manual installation for compatibility
            println!("No install hook found, using fallback installation...");
            let toolchain_bin_dir = paths::toolchain_bin_dir(&version);
            let toolchain_include_dir = paths::toolchain_include_dir(&version);

            std::fs::create_dir_all(&toolchain_bin_dir)?;
            std::fs::create_dir_all(&toolchain_include_dir)?;

            installer::install_zrc_binary(&source_dir, &toolchain_bin_dir)?;
            installer::install_zircop_binary(&source_dir, &toolchain_bin_dir)?;
            installer::install_include_files(&source_dir, &toolchain_include_dir)?;
        }

        // Update current symlink
        let current_link = paths::current_toolchain_link();
        paths::create_link(&toolchain_dir, &current_link)?;

        // Create/update bin links for zrc and zircop
        let toolchain_bin_dir = paths::toolchain_bin_dir(&version);
        let zrc_binary = toolchain_bin_dir.join(if cfg!(windows) { "zrc.exe" } else { "zrc" });
        let zircop_binary = toolchain_bin_dir.join(if cfg!(windows) { "zircop.exe" } else { "zircop" });

        if zrc_binary.exists() {
            let zrc_link = paths::zrc_binary_link();
            paths::create_link(&zrc_binary, &zrc_link)?;
        }

        if zircop_binary.exists() {
            let zircop_link = paths::zircop_binary_link();
            paths::create_link(&zircop_binary, &zircop_link)?;
        }

        println!("\n✓ Successfully built and installed zrc {}", version);
        println!("  Toolchain location: {}", toolchain_dir.display());
        println!("\nTo use zrc, add to your PATH:");
        println!("  export PATH=\"{}:$PATH\"", paths::bin_dir().display());
        println!("\nOr run:");
        println!("  source <(zircon env)");

        Ok(())
    }
}
