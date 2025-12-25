//! Commands for installing pre-built zrc toolchains

use std::error::Error;
use std::path::Path;
use std::time::Duration;

use clap::Parser;
use flate2::read::GzDecoder;
use tar::Archive;

use crate::{cli::DispatchCommand, paths, platform};

/// Install a pre-built version of zrc from GitHub releases
#[derive(Parser)]
pub struct InstallCmd {
    /// The release tag to install (e.g., "nightly", "v0.1.0")
    pub tag: String,

    /// Custom zrc repository (organization/repo)
    #[arg(
        long = "zrc-repo",
        default_value = "zirco-lang/zrc"
    )]
    pub repo: String,
}

impl DispatchCommand for InstallCmd {
    fn dispatch(self) -> Result<(), Box<dyn Error>> {
        // Ensure directories exist
        paths::ensure_directories()?;

        // Detect platform
        let artifact_name = platform::get_platform_artifact_name()?;
        println!("Detected platform artifact: {}", artifact_name);

        // Construct download URL
        let download_url = format!(
            "https://github.com/{}/releases/download/{}/{}",
            self.repo, self.tag, artifact_name
        );

        println!("Downloading from: {}", download_url);

        // Download the artifact with timeout to prevent indefinite hanging
        let client = reqwest::blocking::Client::builder()
            .timeout(Duration::from_secs(300)) // 5 minute timeout
            .build()?;
        let response = client.get(&download_url).send()?;

        if !response.status().is_success() {
            return Err(format!(
                "Failed to download artifact: HTTP {}. \
                 The release '{}' may not exist or may not have pre-built binaries for your platform.\n\
                 Available at: https://github.com/{}/releases/tag/{}",
                response.status(),
                self.tag,
                self.repo,
                self.tag
            )
            .into());
        }

        let bytes = response.bytes()?;
        println!("Downloaded {} bytes", bytes.len());

        // Create a version name for the toolchain
        let version = format!("{}-prebuilt", self.tag);
        println!("Installing as version: {}", version);

        // Create toolchain directory
        let toolchain_dir = paths::toolchain_dir(&version);
        if toolchain_dir.exists() {
            return Err(format!(
                "Toolchain '{}' already exists at {}\n\
                 Use 'zircon delete {}' to remove it first.",
                version,
                toolchain_dir.display(),
                version
            )
            .into());
        }

        std::fs::create_dir_all(&toolchain_dir)?;

        // Extract the tar.gz archive with security validation
        println!("Extracting archive...");
        let decoder = GzDecoder::new(&*bytes);
        let mut archive = Archive::new(decoder);
        
        // Extract to toolchain directory with path validation to prevent directory traversal
        for entry in archive.entries()? {
            let mut entry = entry?;
            let entry_path = entry.path()?;
            
            // Security check: prevent directory traversal attacks (zip slip)
            if entry_path
                .components()
                .any(|c| matches!(c, std::path::Component::ParentDir))
            {
                return Err(format!(
                    "Archive contains invalid path with '..' component: {}",
                    entry_path.display()
                )
                .into());
            }
            
            // Security check: reject absolute paths
            if entry_path.is_absolute() {
                return Err(format!(
                    "Archive contains absolute path: {}",
                    entry_path.display()
                )
                .into());
            }
            
            entry.unpack_in(&toolchain_dir)?;
        }

        println!("✓ Extracted to: {}", toolchain_dir.display());

        // Verify that essential files exist
        verify_toolchain(&toolchain_dir)?;

        // Update current symlink
        let current_link = paths::current_toolchain_link();
        paths::create_link(&toolchain_dir, &current_link)?;

        println!("\n✓ Successfully installed zrc {}", version);
        println!("  Toolchain location: {}", toolchain_dir.display());
        println!("\nTo use zrc, run:");
        println!("  source <(zircon env)");

        Ok(())
    }
}

/// Verify that the toolchain contains essential files
fn verify_toolchain(toolchain_dir: &Path) -> Result<(), Box<dyn Error>> {
    let bin_dir = toolchain_dir.join("bin");
    let zrc_binary = bin_dir.join("zrc");

    if !zrc_binary.exists() {
        return Err(format!(
            "Invalid toolchain: zrc binary not found at {}.\n\
             The downloaded archive may be corrupted or have an unexpected structure.",
            zrc_binary.display()
        )
        .into());
    }

    // Make sure the binary is executable on Unix systems
    #[cfg(unix)]
    {
        use std::os::unix::fs::PermissionsExt;
        let mut perms = std::fs::metadata(&zrc_binary)?.permissions();
        perms.set_mode(0o755);
        std::fs::set_permissions(&zrc_binary, perms)?;
    }

    println!("✓ Verified toolchain structure");
    Ok(())
}
