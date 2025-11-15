//! Commands for environment configuration

use std::error::Error;
use clap::Parser;

use crate::cli::DispatchCommand;
use crate::paths;

/// Output shell environment configuration
#[derive(Parser)]
pub struct EnvCmd {
    /// Specify shell format (bash, fish, powershell, cmd)
    #[arg(long)]
    shell: Option<String>,
}

impl DispatchCommand for EnvCmd {
    fn dispatch(self) -> Result<(), Box<dyn Error>> {
        let bin_dir = paths::bin_dir();
        let include_dir = paths::include_dir_link();
        
        // Determine shell type
        let shell_type = self.shell.map_or_else(detect_shell, |shell| shell.to_lowercase());
        
        match shell_type.as_str() {
            "fish" => {
                // Fish shell syntax
                println!("set -gx PATH \"{}\" $PATH;", bin_dir.display());
                if include_dir.exists() {
                    println!("set -gx ZRC_INCLUDE_PATH \"{}\";", include_dir.display());
                }
            }
            "powershell" | "pwsh" => {
                // PowerShell syntax
                println!("$env:Path = \"{};$env:Path\";", bin_dir.display());
                if include_dir.exists() {
                    println!("$env:ZRC_INCLUDE_PATH = \"{}\";", include_dir.display());
                }
            }
            "cmd" => {
                // Windows CMD syntax
                println!("set PATH={};%PATH%", bin_dir.display());
                if include_dir.exists() {
                    println!("set ZRC_INCLUDE_PATH={}", include_dir.display());
                }
            }
            _ => {
                // Bash/Zsh syntax (default)
                println!("export PATH=\"{}:$PATH\";", bin_dir.display());
                if include_dir.exists() {
                    println!("export ZRC_INCLUDE_PATH=\"{}\";", include_dir.display());
                }
            }
        }
        
        Ok(())
    }
}

/// Detect the current shell type
fn detect_shell() -> String {
    // On Windows, check for PowerShell or CMD
    #[cfg(windows)]
    {
        // Check if running in PowerShell
        if std::env::var("PSModulePath").is_ok() {
            return "powershell".to_string();
        }
        // Default to cmd on Windows
        return "cmd".to_string();
    }
    
    // On Unix-like systems, check SHELL environment variable
    #[cfg(not(windows))]
    {
        let shell = std::env::var("SHELL")
            .unwrap_or_else(|_| "/bin/bash".to_string());
        
        if shell.contains("fish") {
            "fish".to_string()
        } else {
            "bash".to_string()
        }
    }
}
