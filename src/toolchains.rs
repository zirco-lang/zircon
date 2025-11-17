//! Toolchain management operations

use std::error::Error;
use std::fs;

use crate::paths;

/// Information about an installed toolchain
#[derive(Debug, Clone)]
pub struct ToolchainInfo {
    /// The version name
    pub name: String,
    /// Whether this is the currently active toolchain
    pub is_current: bool,
}

/// List all installed toolchains
pub fn list_toolchains() -> Result<Vec<ToolchainInfo>, Box<dyn Error>> {
    let toolchains_dir = paths::toolchains_dir();

    if !toolchains_dir.exists() {
        return Ok(Vec::new());
    }

    // Get current toolchain if it exists
    let current_link = paths::current_toolchain_link();
    let current_version = if current_link.exists() {
        fs::read_link(&current_link)
            .ok()
            .and_then(|p| p.file_name().map(|n| n.to_string_lossy().to_string()))
    } else {
        None
    };

    let mut toolchains: Vec<ToolchainInfo> = fs::read_dir(&toolchains_dir)?
        .filter_map(Result::ok)
        .filter(|e| {
            // Skip the "current" symlink
            e.file_name() != "current" && e.path().is_dir()
        })
        .map(|e| {
            let name = e.file_name().to_string_lossy().to_string();
            let is_current = current_version.as_ref() == Some(&name);
            ToolchainInfo { name, is_current }
        })
        .collect();

    // Sort by name
    toolchains.sort_by(|a, b| a.name.cmp(&b.name));

    Ok(toolchains)
}

/// Get the currently active toolchain name
pub fn get_current_toolchain() -> Result<Option<String>, Box<dyn Error>> {
    let current_link = paths::current_toolchain_link();

    if !current_link.exists() {
        return Ok(None);
    }

    let target = fs::read_link(&current_link)?;
    let version = target
        .file_name()
        .and_then(|n| n.to_str())
        .map(std::string::ToString::to_string);

    Ok(version)
}

/// Delete a specific toolchain
/// Returns an error if trying to delete the current toolchain
pub fn delete_toolchain(version: &str) -> Result<(), Box<dyn Error>> {
    let toolchain_dir = paths::toolchain_dir(version);

    if !toolchain_dir.exists() {
        return Err(format!("Toolchain '{}' not found.", version).into());
    }

    // Check if this is the current toolchain
    if let Some(current) = get_current_toolchain()?
        && current == version
    {
        return Err(format!(
                "Cannot delete '{}' because it is the current toolchain.\nSwitch to another toolchain first with 'zircon switch <version>'.",
                version
            ).into());
    }

    fs::remove_dir_all(&toolchain_dir)?;

    Ok(())
}

/// Get list of toolchains that can be pruned (all except current)
pub fn get_prunable_toolchains() -> Result<Vec<String>, Box<dyn Error>> {
    let current = get_current_toolchain()?;
    let all_toolchains = list_toolchains()?;

    let prunable: Vec<String> = all_toolchains
        .into_iter()
        .filter(|tc| Some(&tc.name) != current.as_ref())
        .map(|tc| tc.name)
        .collect();

    Ok(prunable)
}

/// Check if a toolchain exists
pub fn toolchain_exists(version: &str) -> bool {
    paths::toolchain_dir(version).exists()
}
