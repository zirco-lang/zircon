//! Commands to manage Zircon itself

mod cmd_version;

use std::{error::Error, fs};

use clap::{Parser, Subcommand};

use crate::cli::DispatchCommand;

/// Valid subcommands on `zircon self`
#[derive(Subcommand)]
pub enum SelfCmds {
    /// Print Zircon's version
    Version,

    /// Build Zircon itself from source
    Build(BuildSelfCmd),

    /// Import Zircon from an archive
    Import(ImportSelfCmd),

    /// Install a pre-built Zircon release
    Install(InstallSelfCmd),
}

/// Build Zircon itself from source
#[derive(Parser)]
pub struct BuildSelfCmd {
    /// Git reference to build (branch, tag, or commit). Defaults to 'main'
    #[arg(default_value = "main")]
    pub reference: String,
}

/// Import Zircon from an archive file
#[derive(Parser)]
pub struct ImportSelfCmd {
    /// Path to the archive (.tar.gz, .tar, or .zip) containing Zircon
    pub archive: std::path::PathBuf,
}

/// Install a pre-built Zircon release
#[derive(Parser)]
pub struct InstallSelfCmd {
    /// The release tag to install (e.g., "nightly", "v0.1.0")
    #[arg(default_value = "nightly")]
    pub tag: String,
}

impl DispatchCommand for SelfCmds {
    fn dispatch(self) -> Result<(), Box<dyn Error>> {
        match self {
            Self::Version => {
                cmd_version::cmd_version();
                Ok(())
            }
            Self::Build(cmd) => cmd_self_build(&cmd.reference),
            Self::Import(cmd) => cmd_self_import(&cmd.archive),
            Self::Install(cmd) => cmd_self_install(&cmd.tag),
        }
    }
}

/// Build Zircon itself from source
fn cmd_self_build(reference: &str) -> Result<(), Box<dyn Error>> {
    use crate::{build, git_utils, paths};

    println!("Building Zircon from '{}'...", reference);

    let zircon_source = paths::zircon_source_dir();

    // Clone or open the zircon repository
    let repo =
        git_utils::clone_or_open("https://github.com/zirco-lang/zircon.git", &zircon_source)?;

    // Fetch and checkout the specified reference
    git_utils::fetch(&repo)?;
    git_utils::checkout_ref(&repo, reference)?;

    println!("Building Zircon...");
    build::check_cargo()?;
    build::build_rust_project(&zircon_source)?;

    // Copy the new binary
    let binary_name = if cfg!(windows) {
        "zircon.exe"
    } else {
        "zircon"
    };
    let new_binary = zircon_source
        .join("target")
        .join("release")
        .join(binary_name);
    let self_binary = paths::self_zircon_binary();

    if !new_binary.exists() {
        return Err("Failed to build new Zircon binary".into());
    }

    // Only copy if source and destination are different paths
    // If they're the same, the binary is already in place from cargo build
    if new_binary == self_binary {
        println!("Binary already in place from build...");
    } else {
        println!("Installing updated binary...");
        fs::copy(&new_binary, &self_binary)?;

        // Make executable on Unix
        #[cfg(unix)]
        {
            use std::os::unix::fs::PermissionsExt;
            let mut perms = fs::metadata(&self_binary)?.permissions();
            perms.set_mode(0o755);
            fs::set_permissions(&self_binary, perms)?;
        }
    }

    // Update the link in bin
    let zircon_link = paths::zircon_binary_link();
    paths::create_link(&self_binary, &zircon_link)?;

    println!("✓ Zircon built successfully from '{}'!", reference);

    Ok(())
}

/// Import Zircon from an archive
fn cmd_self_import(archive: &std::path::Path) -> Result<(), Box<dyn Error>> {
    println!("Importing Zircon from archive...");

    // Verify archive exists
    if !archive.exists() {
        return Err(format!("Archive not found: {}", archive.display()).into());
    }

    // Ensure directories exist
    crate::paths::ensure_directories()?;

    let self_dir = crate::paths::zircon_root().join("self");

    // Remove existing self directory if it exists
    if self_dir.exists() {
        fs::remove_dir_all(&self_dir)?;
    }

    // Create self directory
    fs::create_dir_all(&self_dir)?;

    // Extract archive to self directory
    println!("Extracting archive...");
    extract_self_archive(archive, &self_dir)?;

    // Validate that bin directory exists
    let self_bin_dir = self_dir.join("bin");
    if !self_bin_dir.exists() || !self_bin_dir.is_dir() {
        return Err(format!(
            "Invalid archive structure: missing 'bin' directory at {}",
            self_bin_dir.display()
        )
        .into());
    }

    // Find zircon binary in self/bin
    let zircon_binary = self_bin_dir.join(if cfg!(windows) {
        "zircon.exe"
    } else {
        "zircon"
    });

    if !zircon_binary.exists() {
        return Err(format!(
            "Zircon binary not found in archive at {}",
            zircon_binary.display()
        )
        .into());
    }

    // Make executable on Unix
    #[cfg(unix)]
    {
        use std::os::unix::fs::PermissionsExt;
        let mut perms = fs::metadata(&zircon_binary)?.permissions();
        perms.set_mode(0o755);
        fs::set_permissions(&zircon_binary, perms)?;
    }

    // Create link in bin directory
    let zircon_link = crate::paths::zircon_binary_link();
    crate::paths::create_link(&zircon_binary, &zircon_link)?;

    println!("✓ Zircon imported successfully!");
    println!("  Location: {}", self_dir.display());

    Ok(())
}

/// Install a pre-built Zircon release
fn cmd_self_install(tag: &str) -> Result<(), Box<dyn Error>> {
    use std::env;

    println!("Installing Zircon {} release...", tag);

    // Detect platform and architecture
    let (platform, arch) = detect_platform_and_arch()?;

    // Construct download URL for zircon repository
    let filename = format!("zircon-{}-{}.tar.gz", platform, arch);
    let url = format!(
        "https://github.com/zirco-lang/zircon/releases/download/{}/{}",
        tag, filename
    );

    println!("Downloading from: {}", url);

    // Create temporary directory for download
    let temp_dir = env::temp_dir();
    let temp_file = temp_dir.join(&filename);

    // Download the file
    download_file(&url, &temp_file)?;

    println!("Download complete. Importing Zircon...");

    // Import the downloaded archive
    let result = cmd_self_import(&temp_file);

    // Clean up the temporary file (best effort)
    if temp_file.exists() {
        if let Err(e) = fs::remove_file(&temp_file) {
            eprintln!("Warning: Failed to clean up temporary file: {}", e);
        }
    }

    result
}

/// Extract archive to self directory (supports tar.gz, tar, and zip)
fn extract_self_archive(
    archive_path: &std::path::Path,
    dest_dir: &std::path::Path,
) -> Result<(), Box<dyn Error>> {
    use flate2::read::GzDecoder;
    use std::fs::File;
    use std::io;
    use tar::Archive;
    use zip::ZipArchive;

    // Determine archive type by checking the full filename first
    let filename = archive_path
        .file_name()
        .and_then(|n| n.to_str())
        .unwrap_or("");

    // Check for multi-part extensions first
    if filename.ends_with(".tar.gz") || filename.ends_with(".tgz") {
        let file = File::open(archive_path)?;
        let decoder = GzDecoder::new(file);
        let mut archive = Archive::new(decoder);
        archive.unpack(dest_dir)?;
    } else {
        // Fall back to single extension check
        let extension = archive_path
            .extension()
            .and_then(|e| e.to_str())
            .unwrap_or("");

        match extension {
            "zip" => {
                let file = File::open(archive_path)?;
                let mut archive = ZipArchive::new(file)?;

                for i in 0..archive.len() {
                    let mut file = archive.by_index(i)?;
                    let outpath = match file.enclosed_name() {
                        Some(path) => dest_dir.join(path),
                        None => continue,
                    };

                    if file.name().ends_with('/') {
                        fs::create_dir_all(&outpath)?;
                    } else {
                        if let Some(parent) = outpath.parent() {
                            fs::create_dir_all(parent)?;
                        }
                        let mut outfile = File::create(&outpath)?;
                        io::copy(&mut file, &mut outfile)?;
                    }

                    // Preserve Unix permissions on Unix systems
                    #[cfg(unix)]
                    {
                        use std::os::unix::fs::PermissionsExt;
                        if let Some(mode) = file.unix_mode() {
                            fs::set_permissions(&outpath, fs::Permissions::from_mode(mode))?;
                        }
                    }
                }
            }
            "tar" => {
                let file = File::open(archive_path)?;
                let mut archive = Archive::new(file);
                archive.unpack(dest_dir)?;
            }
            _ => {
                return Err(format!(
                    "Unsupported archive format. Supported formats: .tar.gz, .tgz, .tar, .zip"
                )
                .into());
            }
        }
    }

    Ok(())
}

/// Detect the current platform and architecture
fn detect_platform_and_arch() -> Result<(String, String), Box<dyn Error>> {
    let os = std::env::consts::OS;
    let arch = std::env::consts::ARCH;

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
fn download_file(url: &str, dest: &std::path::PathBuf) -> Result<(), Box<dyn Error>> {
    use std::fs::File;
    use std::io::Write;

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
