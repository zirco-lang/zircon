//! Commands for environment configuration

use std::{error::Error, path::Path};

use clap::Parser;

use crate::{cli::DispatchCommand, paths};

/// Output shell environment configuration
#[derive(Parser)]
pub struct EnvCmd {
    /// Specify shell format (bash, zsh, fish, powershell, cmd)
    #[arg(long)]
    shell: Option<String>,
}

impl DispatchCommand for EnvCmd {
    fn dispatch(self) -> Result<(), Box<dyn Error>> {
        let bin_dir = paths::bin_dir();

        // Determine shell type
        let shell_type = self
            .shell
            .map_or_else(detect_shell, |shell| shell.to_lowercase());

        match shell_type.as_str() {
            "fish" => {
                // Fish shell syntax - use double quotes and escape internal quotes
                let bin_escaped = escape_for_fish(&bin_dir);
                println!("set -gx PATH {} $PATH;", bin_escaped);
                // Source the toolchain's bin.sh if it exists (fish uses source command too)
                let toolchain_bin_sh = paths::current_toolchain_bin_sh();
                if toolchain_bin_sh.exists() {
                    let bin_sh_escaped = escape_for_fish(&toolchain_bin_sh);
                    println!("source {};", bin_sh_escaped);
                }
            }
            "powershell" | "pwsh" => {
                // PowerShell syntax - double-quote and escape internal double quotes
                let bin_escaped = escape_for_powershell(&bin_dir);
                println!("$env:Path = \"{};$env:Path\";", bin_escaped);
                // Source the toolchain's bin.ps1 if it exists (PowerShell uses . for sourcing)
                let toolchain_bin_ps1 = paths::current_toolchain_bin_ps1();
                if toolchain_bin_ps1.exists() {
                    let bin_ps1_escaped = escape_for_powershell(&toolchain_bin_ps1);
                    println!(". \"{}\";", bin_ps1_escaped);
                }
            }
            "cmd" => {
                // Windows CMD syntax - escape percent signs and carets
                let bin_escaped = escape_for_cmd(&bin_dir);
                println!("set PATH={};%PATH%", bin_escaped);
                // Source the toolchain's bin.bat if it exists (CMD uses call)
                let toolchain_bin_bat = paths::current_toolchain_bin_bat();
                if toolchain_bin_bat.exists() {
                    let bin_bat_escaped = escape_for_cmd(&toolchain_bin_bat);
                    println!("call {}", bin_bat_escaped);
                }
            }
            "zsh" | "bash" | "sh" => {
                // Bash/Zsh syntax - use single quotes and escape internal single quotes
                let bin_escaped = escape_for_posix_shell(&bin_dir);
                println!("export PATH={}:$PATH;", bin_escaped);
                // Source the toolchain's bin.sh if it exists
                let toolchain_bin_sh = paths::current_toolchain_bin_sh();
                if toolchain_bin_sh.exists() {
                    let bin_sh_escaped = escape_for_posix_shell(&toolchain_bin_sh);
                    println!("source {};", bin_sh_escaped);
                }
            }
            _ => {
                // Default for unknown shells - use POSIX syntax
                let bin_escaped = escape_for_posix_shell(&bin_dir);
                println!("export PATH={}:$PATH;", bin_escaped);
                // Source the toolchain's bin.sh if it exists
                let toolchain_bin_sh = paths::current_toolchain_bin_sh();
                if toolchain_bin_sh.exists() {
                    let bin_sh_escaped = escape_for_posix_shell(&toolchain_bin_sh);
                    println!("source {};", bin_sh_escaped);
                }
            }
        }

        Ok(())
    }
}

/// Escape a path for POSIX shells (bash, zsh, sh)
/// Uses single quotes and escapes internal single quotes
fn escape_for_posix_shell(path: &Path) -> String {
    let path_str = path.display().to_string();
    // Replace ' with '\''
    format!("'{}'", path_str.replace('\'', "'\\''"))
}

/// Escape a path for Fish shell
/// Uses double quotes or falls back to proper escaping
fn escape_for_fish(path: &Path) -> String {
    let path_str = path.display().to_string();
    // For fish, we can use double quotes and escape internal double quotes,
    // backslashes, and dollar signs
    let escaped = path_str
        .replace('\\', "\\\\")
        .replace('"', "\\\"")
        .replace('$', "\\$");
    format!("\"{}\"", escaped)
}

/// Escape a path for `PowerShell`
/// Uses double quotes and escapes internal double quotes
fn escape_for_powershell(path: &Path) -> String {
    let path_str = path.display().to_string();
    // Escape double quotes by doubling them
    path_str.replace('"', "\"\"")
}

/// Escape a path for Windows CMD
/// Escapes percent signs, carets, and other special characters
fn escape_for_cmd(path: &Path) -> String {
    let path_str = path.display().to_string();
    // Escape special CMD characters
    path_str
        .replace('%', "%%")
        .replace('^', "^^")
        .replace('&', "^&")
        .replace('|', "^|")
        .replace('<', "^<")
        .replace('>', "^>")
}

/// Detect the current shell type
fn detect_shell() -> String {
    // On Windows, check for PowerShell or CMD
    #[cfg(windows)]
    {
        // Check for PowerShell indicators
        if std::env::var("PSModulePath").is_ok() || std::env::var("PSVersionTable").is_ok() {
            return "powershell".to_string();
        }

        // Check COMSPEC for cmd.exe
        if let Ok(comspec) = std::env::var("COMSPEC") {
            if comspec.to_lowercase().contains("cmd.exe") {
                return "cmd".to_string();
            }
        }

        // Default to cmd on Windows
        return "cmd".to_string();
    }

    // On Unix-like systems, check SHELL environment variable
    #[cfg(not(windows))]
    {
        if let Ok(shell) = std::env::var("SHELL") {
            // Extract basename of shell path
            let shell_name = Path::new(&shell)
                .file_name()
                .and_then(|s| s.to_str())
                .unwrap_or("");

            // Match against known shells
            match shell_name {
                "fish" => return "fish".to_string(),
                "zsh" => return "zsh".to_string(),
                "bash" => return "bash".to_string(),
                "ksh" | "ksh93" => return "ksh".to_string(),
                "dash" => return "dash".to_string(),
                "sh" => return "sh".to_string(),
                _ => {
                    // If it contains the shell name anywhere in the path
                    if shell.contains("fish") {
                        return "fish".to_string();
                    } else if shell.contains("zsh") {
                        return "zsh".to_string();
                    } else if shell.contains("bash") {
                        return "bash".to_string();
                    }
                }
            }
        }

        // Safe default for unknown shells
        "sh".to_string()
    }
}
