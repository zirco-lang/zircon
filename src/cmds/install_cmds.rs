//! Commands for installing pre-built toolchains

use std::env;
use std::error::Error;
use std::fs::File;
use std::io::Write;
use std::path::PathBuf;

use clap::Parser;

use crate::cli::DispatchCommand;
use crate::cmds::toolchain_cmds;

/// Install pre-built toolchains
#[derive(Parser)]
pub struct InstallCmd {
    /// The release tag to install (e.g., "nightly", "v0.1.0")
    #[arg(default_value = "nightly")]
    pub tag: String,
}

impl DispatchCommand for InstallCmd {
    fn dispatch(self) -> Result<(), Box<dyn Error>> {
        install_tag(&self.tag)
    }
}

/// Install a pre-built toolchain from GitHub releases
fn install_tag(tag: &str) -> Result<(), Box<dyn Error>> {
    println!("Installing {} release...", tag);

    // Detect platform and architecture
    let (platform, arch) = detect_platform_and_arch()?;

    // Construct download URL
    let filename = format!("zrc-{}-{}.tar.gz", platform, arch);
    let url = format!(
        "https://github.com/zirco-lang/zrc/releases/download/{}/{}",
        tag, filename
    );

    println!("Downloading from: {}", url);

    // Create temporary directory for download
    let temp_dir = env::temp_dir();
    let temp_file = temp_dir.join(&filename);

    // Download the file
    download_file(&url, &temp_file)?;

    println!("Download complete. Importing toolchain...");

    // Use the existing import functionality
    let import_cmd = toolchain_cmds::ImportCmd {
        archive: temp_file.clone(),
    };

    // Import the toolchain
    let result = import_cmd.dispatch();

    // Clean up the temporary file (best effort)
    if temp_file.exists()
        && let Err(e) = std::fs::remove_file(&temp_file)
    {
        eprintln!("Warning: Failed to clean up temporary file: {}", e);
    }

    result
}

/// Detect the current platform and architecture
fn detect_platform_and_arch() -> Result<(String, String), Box<dyn Error>> {
    let os = env::consts::OS;
    let arch = env::consts::ARCH;

    let platform = match os {
        "linux" => "linux",
        "macos" => "macos",
        _ => {
            return Err(format!(
                "Unsupported platform: {}. Only linux and macos are supported.",
                os
            )
            .into());
        }
    };

    let architecture = match arch {
        "x86_64" => "x64",
        "aarch64" => "arm64",
        _ => {
            return Err(format!(
                "Unsupported architecture: {}. Only x86_64 (x64) and aarch64 (arm64) are supported.",
                arch
            )
            .into());
        }
    };

    Ok((platform.to_string(), architecture.to_string()))
}

/// Download a file from a URL to a local path
fn download_file(url: &str, dest: &PathBuf) -> Result<(), Box<dyn Error>> {
    let response = reqwest::blocking::get(url)?;

    if !response.status().is_success() {
        return Err(format!(
            "Failed to download file: HTTP {}. The release may not be available or may not have pre-built binaries for your platform.",
            response.status()
        )
        .into());
    }

    let mut file = File::create(dest)?;
    let content = response.bytes()?;
    file.write_all(&content)?;

    Ok(())
}
