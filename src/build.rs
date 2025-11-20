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
        let exit_code = status.code().unwrap_or(-1);
        return Err(format!("Failed to build zrc (exit code: {})", exit_code).into());
    }

    println!("Build complete!");
    Ok(())
}

/// Check if cargo is available
pub fn check_cargo() -> Result<(), Box<dyn std::error::Error>> {
    let result = Command::new("cargo").arg("--version").output();

    match result {
        Err(e) if e.kind() == std::io::ErrorKind::NotFound => {
            Err("cargo not found. Please install Rust from https://rustup.rs/".into())
        }
        Err(e) => Err(format!("Failed to execute cargo: {}", e).into()),
        Ok(output) => {
            if output.status.success() {
                let version = String::from_utf8_lossy(&output.stdout);
                println!("Found cargo: {}", version.trim());
                Ok(())
            } else {
                let exit_code = output.status.code().unwrap_or(-1);
                let stderr = String::from_utf8_lossy(&output.stderr);
                Err(format!(
                    "cargo --version failed (exit code: {})\nStderr: {}",
                    exit_code, stderr
                )
                .into())
            }
        }
    }
}
