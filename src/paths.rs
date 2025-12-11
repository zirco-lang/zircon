//! Path management for Zircon directories

use std::path::{Path, PathBuf};

/// Get the Zircon root directory (default: ~/.zircon or %USERPROFILE%\.zircon)
///
/// Can be overridden with `ZIRCON_PREFIX` environment variable
pub fn zircon_root() -> PathBuf {
    std::env::var("ZIRCON_PREFIX").map_or_else(
        |_| dirs::home_dir().map_or_else(|| PathBuf::from(".zircon"), |home| home.join(".zircon")),
        PathBuf::from,
    )
}

/// Get the sources directory
pub fn sources_dir() -> PathBuf {
    zircon_root().join("sources")
}

/// Get the zirco-lang organization directory
pub fn zirco_lang_dir() -> PathBuf {
    sources_dir().join("zirco-lang")
}

/// Get the zrc source directory
pub fn zrc_source_dir() -> PathBuf {
    zirco_lang_dir().join("zrc")
}

/// Get the zircon source directory (for self updates)
pub fn zircon_source_dir() -> PathBuf {
    zirco_lang_dir().join("zircon")
}

/// Get the toolchains directory
pub fn toolchains_dir() -> PathBuf {
    zircon_root().join("toolchains")
}

/// Get a specific toolchain directory
pub fn toolchain_dir(version: &str) -> PathBuf {
    toolchains_dir().join(version)
}

/// Get the current toolchain symlink path
pub fn current_toolchain_link() -> PathBuf {
    toolchains_dir().join("current")
}

/// Get the bin.sh script path in the current toolchain
pub fn current_toolchain_bin_sh() -> PathBuf {
    current_toolchain_link().join("bin.sh")
}

/// Get the bin.ps1 script path in the current toolchain (for `PowerShell`)
pub fn current_toolchain_bin_ps1() -> PathBuf {
    current_toolchain_link().join("bin.ps1")
}

/// Get the bin.bat script path in the current toolchain (for CMD)
pub fn current_toolchain_bin_bat() -> PathBuf {
    current_toolchain_link().join("bin.bat")
}

/// Get the self directory (symlink to zircon source)
#[allow(dead_code)]
pub fn self_dir() -> PathBuf {
    zircon_root().join("self")
}

/// Get the self bin directory
pub fn self_bin_dir() -> PathBuf {
    // Self is a symlink to sources/zirco-lang/zircon
    // So the binary is actually in sources/zirco-lang/zircon/target/release
    zircon_source_dir().join("target").join("release")
}

/// Get the zircon binary path in self
pub fn self_zircon_binary() -> PathBuf {
    self_bin_dir().join(if cfg!(windows) {
        "zircon.exe"
    } else {
        "zircon"
    })
}

/// Get the root bin directory
pub fn bin_dir() -> PathBuf {
    zircon_root().join("bin")
}

/// Get the zircon binary link in root bin
pub fn zircon_binary_link() -> PathBuf {
    bin_dir().join(if cfg!(windows) {
        "zircon.exe"
    } else {
        "zircon"
    })
}

/// Ensure all necessary directories exist
pub fn ensure_directories() -> std::io::Result<()> {
    std::fs::create_dir_all(sources_dir())?;
    std::fs::create_dir_all(zirco_lang_dir())?;
    std::fs::create_dir_all(toolchains_dir())?;
    std::fs::create_dir_all(self_bin_dir())?;
    std::fs::create_dir_all(bin_dir())?;
    Ok(())
}

/// Create a symlink or directory junction (Windows) or copy (fallback)
#[cfg(unix)]
pub fn create_link(src: &Path, dst: &Path) -> std::io::Result<()> {
    // Remove existing link if present
    if dst.exists() || dst.read_link().is_ok() {
        if dst.is_dir() && dst.read_link().is_err() {
            std::fs::remove_dir_all(dst)?;
        } else {
            std::fs::remove_file(dst).ok();
        }
    }

    std::os::unix::fs::symlink(src, dst)
}

/// Create a symlink or directory junction (Windows) or copy (fallback)
#[cfg(windows)]
pub fn create_link(src: &Path, dst: &Path) -> std::io::Result<()> {
    // Remove existing link if present
    if dst.exists() {
        if dst.is_dir() {
            std::fs::remove_dir_all(dst)?;
        } else {
            std::fs::remove_file(dst)?;
        }
    }

    if src.is_dir() {
        std::os::windows::fs::symlink_dir(src, dst)
    } else {
        std::os::windows::fs::symlink_file(src, dst)
    }
}
