//! Build operations for compiling zrc

use std::path::Path;
use std::process::Command;

/// Build zrc using cargo
pub fn build_zrc(source_dir: &Path) -> Result<(), Box<dyn std::error::Error>> {
    println!("Building zrc (this may take several minutes)...");

    let status = Command::new("cargo")
        .arg("build")
        .arg("--release")
        .current_dir(source_dir)
        .status()?;

    if !status.success() {
        return Err("Failed to build zrc".into());
    }

    println!("Build complete!");
    Ok(())
}

/// Check if cargo is available
pub fn check_cargo() -> Result<(), Box<dyn std::error::Error>> {
    let output = Command::new("cargo").arg("--version").output();

    match output {
        Ok(output) if output.status.success() => {
            let version = String::from_utf8_lossy(&output.stdout);
            println!("Found cargo: {}", version.trim());
            Ok(())
        }
        _ => Err("cargo not found. Please install Rust from https://rustup.rs/".into()),
    }
}
