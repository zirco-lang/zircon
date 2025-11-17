//! Build operations for compiling zrc

use std::path::Path;
use std::process::Command;

/// Build zrc using cargo
pub fn build_zrc(source_dir: &Path) -> Result<(), Box<dyn std::error::Error>> {
    println!("Building zrc (this may take several minutes)...");

    let output = Command::new("cargo")
        .arg("build")
        .arg("--release")
        .current_dir(source_dir)
        .output()?;

    if !output.status.success() {
        let exit_code = output.status.code().unwrap_or(-1);
        let stderr = String::from_utf8_lossy(&output.stderr);
        let stdout = String::from_utf8_lossy(&output.stdout);

        let mut error_msg = format!("Failed to build zrc (exit code: {})", exit_code);
        if !stderr.is_empty() {
            error_msg.push_str("\n\nStderr:\n");
            error_msg.push_str(&stderr);
        }
        if !stdout.is_empty() {
            error_msg.push_str("\n\nStdout:\n");
            error_msg.push_str(&stdout);
        }

        return Err(error_msg.into());
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
