//! Installation and file management operations

use std::fs;
use std::path::Path;

/// Copy a file with error handling
pub fn copy_file(src: &Path, dst: &Path) -> Result<(), Box<dyn std::error::Error>> {
    if let Some(parent) = dst.parent() {
        fs::create_dir_all(parent)?;
    }
    fs::copy(src, dst)?;
    Ok(())
}

/// Copy a directory recursively
pub fn copy_dir_recursive(src: &Path, dst: &Path) -> Result<(), Box<dyn std::error::Error>> {
    fs::create_dir_all(dst)?;

    for entry in fs::read_dir(src)? {
        let entry = entry?;
        let file_type = entry.file_type()?;
        let src_path = entry.path();
        let dst_path = dst.join(entry.file_name());

        if file_type.is_dir() {
            copy_dir_recursive(&src_path, &dst_path)?;
        } else {
            fs::copy(&src_path, &dst_path)?;
        }
    }

    Ok(())
}

/// Install zrc binary to a toolchain directory
pub fn install_zrc_binary(
    source_dir: &Path,
    toolchain_bin_dir: &Path,
) -> Result<(), Box<dyn std::error::Error>> {
    let binary_name = if cfg!(windows) { "zrc.exe" } else { "zrc" };
    let src = source_dir.join("target").join("release").join(binary_name);
    let dst = toolchain_bin_dir.join(binary_name);

    if !src.exists() {
        return Err(format!("Built binary not found at {}", src.display()).into());
    }

    println!("Installing zrc binary to {}", dst.display());
    copy_file(&src, &dst)?;

    // Make executable on Unix
    #[cfg(unix)]
    {
        use std::os::unix::fs::PermissionsExt;
        let mut perms = fs::metadata(&dst)?.permissions();
        perms.set_mode(0o755);
        fs::set_permissions(&dst, perms)?;
    }

    Ok(())
}

/// Install zircop binary to a toolchain directory if it exists
/// Returns Ok(true) if zircop was found and installed, Ok(false) if not found
pub fn install_zircop_binary(
    source_dir: &Path,
    toolchain_bin_dir: &Path,
) -> Result<bool, Box<dyn std::error::Error>> {
    let binary_name = if cfg!(windows) {
        "zircop.exe"
    } else {
        "zircop"
    };
    let src = source_dir.join("target").join("release").join(binary_name);
    let dst = toolchain_bin_dir.join(binary_name);

    // Check if zircop exists in the build output
    if !src.exists() {
        return Ok(false);
    }

    println!("Installing zircop binary to {}", dst.display());
    copy_file(&src, &dst)?;

    // Make executable on Unix
    #[cfg(unix)]
    {
        use std::os::unix::fs::PermissionsExt;
        let mut perms = fs::metadata(&dst)?.permissions();
        perms.set_mode(0o755);
        fs::set_permissions(&dst, perms)?;
    }

    Ok(true)
}

/// Install include files to a toolchain directory
pub fn install_include_files(
    source_dir: &Path,
    toolchain_include_dir: &Path,
) -> Result<(), Box<dyn std::error::Error>> {
    let src = source_dir.join("include");

    if !src.exists() {
        return Err(format!("Include directory not found at {}", src.display()).into());
    }

    println!(
        "Installing include files to {}",
        toolchain_include_dir.display()
    );
    copy_dir_recursive(&src, toolchain_include_dir)?;

    Ok(())
}
