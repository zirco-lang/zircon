//! Commands for environment configuration

use clap::Parser;
use std::error::Error;
use std::path::Path;

use crate::cli::DispatchCommand;
use crate::paths;

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

        // Check for hook/env.sh in current toolchain
        let current_toolchain = paths::current_toolchain_link();
        let env_hook = current_toolchain.join("hook").join("env.sh");

        if env_hook.exists() {
            // Source the env.sh hook script
            match shell_type.as_str() {
                "fish" => {
                    let env_hook_escaped = escape_for_fish(&env_hook);
                    println!("source {};", env_hook_escaped);
                }
                "powershell" | "pwsh" => {
                    // PowerShell doesn't have a direct bash script sourcing equivalent
                    // Fall back to manual env setup for PowerShell
                    output_manual_env(&bin_dir, &shell_type)?;
                }
                "cmd" => {
                    // CMD doesn't support sourcing bash scripts
                    // Fall back to manual env setup
                    output_manual_env(&bin_dir, &shell_type)?;
                }
                _ => {
                    // Bash/Zsh/sh - source the env.sh script
                    let env_hook_escaped = escape_for_posix_shell(&env_hook);
                    println!("source {};", env_hook_escaped);
                }
            }
        } else {
            // Fallback to manual environment setup
            output_manual_env(&bin_dir, &shell_type)?;
        }

        Ok(())
    }
}

/// Output manual environment configuration (fallback)
fn output_manual_env(bin_dir: &Path, shell_type: &str) -> Result<(), Box<dyn Error>> {
    // Use current toolchain's include directory instead of the old .zircon/include link
    let include_dir = paths::current_toolchain_link().join("include");

    match shell_type {
        "fish" => {
            let bin_escaped = escape_for_fish(bin_dir);
            println!("set -gx PATH {} $PATH;", bin_escaped);
            if include_dir.exists() {
                let include_escaped = escape_for_fish(&include_dir);
                println!("set -gx ZIRCO_INCLUDE_PATH {};", include_escaped);
            }
        }
        "powershell" | "pwsh" => {
            let bin_escaped = escape_for_powershell(bin_dir);
            println!("$env:Path = \"{};$env:Path\";", bin_escaped);
            if include_dir.exists() {
                let include_escaped = escape_for_powershell(&include_dir);
                println!("$env:ZIRCO_INCLUDE_PATH = \"{}\";", include_escaped);
            }
        }
        "cmd" => {
            let bin_escaped = escape_for_cmd(bin_dir);
            println!("set PATH={};%PATH%", bin_escaped);
            if include_dir.exists() {
                let include_escaped = escape_for_cmd(&include_dir);
                println!("set ZIRCO_INCLUDE_PATH={}", include_escaped);
            }
        }
        _ => {
            // Default for bash/zsh/sh and unknown shells
            let bin_escaped = escape_for_posix_shell(bin_dir);
            println!("export PATH={}:$PATH;", bin_escaped);
            if include_dir.exists() {
                let include_escaped = escape_for_posix_shell(&include_dir);
                println!("export ZIRCO_INCLUDE_PATH={};", include_escaped);
            }
        }
    }

    Ok(())
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
    // For fish, we can use double quotes and escape internal double quotes, backslashes, and dollar signs
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
