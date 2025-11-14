//! Commands for managing toolchains

use std::error::Error;
use clap::Parser;

use crate::cli::DispatchCommand;
use crate::{paths, git_utils};

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
                self.version, toolchain_dir.display(), self.version
            ).into());
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

/// Update the current toolchain (fetch and rebuild)
#[derive(Parser)]
pub struct UpdateCmd;

impl DispatchCommand for UpdateCmd {
    fn dispatch(self) -> Result<(), Box<dyn Error>> {
        // Read the current link to find which version is active
        let current_link = paths::current_toolchain_link();
        
        if !current_link.exists() {
            return Err("No toolchain is currently active. Use 'zircon build <version>' first.".into());
        }
        
        // Get the target of the symlink
        let current_target = std::fs::read_link(&current_link)?;
        let version = current_target
            .file_name()
            .and_then(|n| n.to_str())
            .ok_or("Could not determine current version")?;
        
        println!("Updating current toolchain: {}", version);
        
        // Parse the version to get the original reference
        // Format is either "vX.Y.Z" or "ref@commit"
        let reference = if version.starts_with('v') && !version.contains('@') {
            version.to_string()
        } else if let Some(at_pos) = version.find('@') {
            version[..at_pos].replace('-', "/")
        } else {
            return Err("Cannot determine original reference for this version".into());
        };
        
        println!("Fetching updates for reference: {}", reference);
        
        let source_dir = paths::zrc_source_dir();
        
        if !source_dir.exists() {
            return Err("Source directory not found. The toolchain may have been installed externally.".into());
        }
        
        let repo = git_utils::clone_or_open("https://github.com/zirco-lang/zrc.git", &source_dir)?;
        git_utils::fetch(&repo)?;
        git_utils::checkout_ref(&repo, &reference)?;
        
        // Now rebuild using the build command logic
        crate::cmds::build_cmds::BuildCmd {
            reference,
            repo_url: "https://github.com/zirco-lang/zrc.git".to_string(),
        }.dispatch()?;
        
        Ok(())
    }
}
