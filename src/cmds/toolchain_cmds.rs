//! Commands for managing toolchains

use std::error::Error;
use std::fs::File;
use std::io::{self, Read};
use std::path::{Path, PathBuf};

use clap::Parser;
use flate2::read::GzDecoder;
use sha2::{Digest, Sha256};
use tar::Archive;
use zip::ZipArchive;

use crate::{cli::DispatchCommand, paths, toolchains};

/// Switch to a different installed toolchain version
#[derive(Parser)]
pub struct SwitchCmd {
    /// The version to switch to
    pub version: String,
}

impl DispatchCommand for SwitchCmd {
    fn dispatch(self) -> Result<(), Box<dyn Error>> {
        let toolchain_dir = paths::toolchain_dir(&self.version);

        if !toolchains::toolchain_exists(&self.version) {
            return Err(format!(
                "Toolchain '{}' not found at {}\nUse 'zircon build {}' to install it.",
                self.version,
                toolchain_dir.display(),
                self.version
            )
            .into());
        }

        // Update current symlink
        let current_link = paths::current_toolchain_link();
        paths::create_link(&toolchain_dir, &current_link)?;

        println!("✓ Switched to toolchain: {}", self.version);

        Ok(())
    }
}

/// Import a toolchain from an archive file
#[derive(Parser)]
#[command(about = "Import a toolchain from an archive (.tar.gz, .tar, or .zip)")]
pub struct ImportCmd {
    /// Path to the archive (.tar.gz, .tar, or .zip) containing the toolchain
    pub archive: PathBuf,
}

impl DispatchCommand for ImportCmd {
    fn dispatch(self) -> Result<(), Box<dyn Error>> {
        // Verify archive exists
        if !self.archive.exists() {
            return Err(format!(
                "Archive not found: {}",
                self.archive.display()
            )
            .into());
        }

        // Compute hash of the tarball
        let hash = compute_archive_hash(&self.archive)?;
        
        // Extract base name from archive filename and append hash
        let base_name = extract_version_from_filename(&self.archive)?;
        let version = format!("{}-{}", base_name, hash);

        println!("Importing toolchain: {}", version);

        // Check if toolchain already exists
        if toolchains::toolchain_exists(&version) {
            return Err(format!(
                "Toolchain '{}' already exists.\nUse 'zircon delete {}' to remove it first.",
                version, version
            )
            .into());
        }

        // Ensure directories exist
        paths::ensure_directories()?;

        // Create toolchain directory
        let toolchain_dir = paths::toolchain_dir(&version);
        std::fs::create_dir_all(&toolchain_dir)?;

        // Extract archive
        println!("Extracting archive...");
        extract_archive(&self.archive, &toolchain_dir)?;

        // Validate toolchain structure
        validate_toolchain_structure(&toolchain_dir)?;

        println!("✓ Successfully imported toolchain: {}", version);
        println!("  Toolchain location: {}", toolchain_dir.display());

        // Always set as current
        let current_link = paths::current_toolchain_link();
        paths::create_link(&toolchain_dir, &current_link)?;
        println!("✓ Set as current toolchain");

        println!("\nTo use this toolchain, run:");
        println!("  source <(zircon env)");

        Ok(())
    }
}

/// Extract version name from archive filename
fn extract_version_from_filename(path: &Path) -> Result<String, Box<dyn Error>> {
    let filename = path
        .file_name()
        .and_then(|n| n.to_str())
        .ok_or("Invalid archive filename")?;

    // Remove common extensions
    let name = filename
        .trim_end_matches(".tar.gz")
        .trim_end_matches(".tgz")
        .trim_end_matches(".tar")
        .trim_end_matches(".zip");

    if name.is_empty() {
        return Err("Could not determine version name from archive filename".into());
    }

    Ok(name.to_string())
}

/// Compute a short hash of the archive file for uniqueness
fn compute_archive_hash(path: &Path) -> Result<String, Box<dyn Error>> {
    let mut file = File::open(path)?;
    let mut hasher = Sha256::new();
    let mut buffer = [0; 8192];

    loop {
        let bytes_read = file.read(&mut buffer)?;
        if bytes_read == 0 {
            break;
        }
        hasher.update(&buffer[..bytes_read]);
    }

    let result = hasher.finalize();
    // Return first 8 characters of the hex digest for a "super shortened" hash
    Ok(format!("{:x}", result)[..8].to_string())
}

/// Extract archive to destination directory
fn extract_archive(archive_path: &Path, dest_dir: &Path) -> Result<(), Box<dyn Error>> {
    // Determine archive type by extension
    let extension = archive_path
        .extension()
        .and_then(|e| e.to_str())
        .unwrap_or("");

    match extension {
        "zip" => extract_zip(archive_path, dest_dir)?,
        "gz" | "tgz" => extract_tar_gz(archive_path, dest_dir)?,
        "tar" => extract_tar(archive_path, dest_dir)?,
        _ => {
            return Err(format!(
                "Unsupported archive format: '{}'. Supported formats: .tar.gz, .tgz, .tar, .zip",
                extension
            )
            .into())
        }
    }

    Ok(())
}

/// Extract gzipped tarball
fn extract_tar_gz(tarball_path: &Path, dest_dir: &Path) -> Result<(), Box<dyn Error>> {
    let file = File::open(tarball_path)?;
    let decoder = GzDecoder::new(file);
    let mut archive = Archive::new(decoder);
    archive.unpack(dest_dir)?;
    Ok(())
}

/// Extract plain tarball
fn extract_tar(tarball_path: &Path, dest_dir: &Path) -> Result<(), Box<dyn Error>> {
    let file = File::open(tarball_path)?;
    let mut archive = Archive::new(file);
    archive.unpack(dest_dir)?;
    Ok(())
}

/// Extract zip file
fn extract_zip(zip_path: &Path, dest_dir: &Path) -> Result<(), Box<dyn Error>> {
    let file = File::open(zip_path)?;
    let mut archive = ZipArchive::new(file)?;

    for i in 0..archive.len() {
        let mut file = archive.by_index(i)?;
        let outpath = match file.enclosed_name() {
            Some(path) => dest_dir.join(path),
            None => continue,
        };

        if file.name().ends_with('/') {
            std::fs::create_dir_all(&outpath)?;
        } else {
            if let Some(parent) = outpath.parent() {
                std::fs::create_dir_all(parent)?;
            }
            let mut outfile = File::create(&outpath)?;
            io::copy(&mut file, &mut outfile)?;
        }

        // Preserve Unix permissions on Unix systems
        #[cfg(unix)]
        {
            use std::os::unix::fs::PermissionsExt;
            if let Some(mode) = file.unix_mode() {
                std::fs::set_permissions(&outpath, std::fs::Permissions::from_mode(mode))?;
            }
        }
    }

    Ok(())
}

/// Validate that the extracted toolchain has the expected structure
fn validate_toolchain_structure(toolchain_dir: &Path) -> Result<(), Box<dyn Error>> {
    // Check for bin directory
    let bin_dir = toolchain_dir.join("bin");
    if !bin_dir.exists() || !bin_dir.is_dir() {
        return Err(format!(
            "Invalid toolchain structure: missing 'bin' directory at {}",
            bin_dir.display()
        )
        .into());
    }

    // Check for include directory (optional but expected)
    let include_dir = toolchain_dir.join("include");
    if !include_dir.exists() {
        eprintln!("Warning: 'include' directory not found in toolchain");
    }

    Ok(())
}

/// List installed toolchains
#[derive(Parser)]
pub struct ListCmd;

impl DispatchCommand for ListCmd {
    fn dispatch(self) -> Result<(), Box<dyn Error>> {
        let toolchains = toolchains::list_toolchains()?;

        if toolchains.is_empty() {
            println!("No toolchains installed.");
            return Ok(());
        }

        println!("Installed toolchains:");

        for tc in toolchains {
            if tc.is_current {
                println!("  {} (current)", tc.name);
            } else {
                println!("  {}", tc.name);
            }
        }

        Ok(())
    }
}

/// Delete a specific toolchain
#[derive(Parser)]
pub struct DeleteCmd {
    /// The version to delete
    pub version: String,
}

impl DispatchCommand for DeleteCmd {
    fn dispatch(self) -> Result<(), Box<dyn Error>> {
        println!("Deleting toolchain: {}", self.version);
        toolchains::delete_toolchain(&self.version)?;
        println!("✓ Toolchain '{}' deleted", self.version);

        Ok(())
    }
}

/// Remove unused toolchains (keep only current)
#[derive(Parser)]
pub struct PruneCmd {
    /// Skip confirmation prompt
    #[arg(short = 'y', long = "yes")]
    yes: bool,
}

impl DispatchCommand for PruneCmd {
    fn dispatch(self) -> Result<(), Box<dyn Error>> {
        let to_prune = toolchains::get_prunable_toolchains()?;

        if to_prune.is_empty() {
            println!("No unused toolchains to prune.");
            return Ok(());
        }

        println!("Toolchains to be deleted:");
        for name in &to_prune {
            println!("  {}", name);
        }

        if let Some(current) = toolchains::get_current_toolchain()? {
            println!("\nCurrent toolchain '{}' will be kept.", current);
        }

        if !self.yes {
            println!("\nProceed with deletion? (y/N): ");
            let mut input = String::new();
            io::stdin().read_line(&mut input)?;
            let input = input.trim().to_lowercase();

            if input != "y" && input != "yes" {
                println!("Cancelled.");
                return Ok(());
            }
        }

        println!("\nDeleting toolchains...");
        for name in &to_prune {
            toolchains::delete_toolchain(name)?;
            println!("  ✓ Deleted {}", name);
        }

        println!("\n✓ Pruned {} toolchain(s)", to_prune.len());

        Ok(())
    }
}
