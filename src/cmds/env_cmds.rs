//! Commands for environment configuration

use std::error::Error;
use clap::Parser;

use crate::cli::DispatchCommand;
use crate::paths;

/// Output shell environment configuration
#[derive(Parser)]
pub struct EnvCmd;

impl DispatchCommand for EnvCmd {
    fn dispatch(self) -> Result<(), Box<dyn Error>> {
        let bin_dir = paths::bin_dir();
        let include_dir = paths::include_dir_link();
        
        // Detect shell based on environment or default to bash
        let shell = std::env::var("SHELL")
            .unwrap_or_else(|_| "/bin/bash".to_string());
        
        if shell.contains("fish") {
            // Fish shell syntax
            println!("set -gx PATH \"{}\" $PATH;", bin_dir.display());
            if include_dir.exists() {
                println!("set -gx ZRC_INCLUDE_PATH \"{}\";", include_dir.display());
            }
        } else {
            // Bash/Zsh syntax (default)
            println!("export PATH=\"{}:$PATH\";", bin_dir.display());
            if include_dir.exists() {
                println!("export ZRC_INCLUDE_PATH=\"{}\";", include_dir.display());
            }
        }
        
        Ok(())
    }
}
