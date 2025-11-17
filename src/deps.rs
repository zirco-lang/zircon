//! LLVM and clang dependency checking

use std::process::Command;

use crate::config;

/// Check if LLVM 20 is installed (REQUIRED for Zirco)
pub fn check_llvm() -> Result<String, Box<dyn std::error::Error>> {
    // List of possible llvm-config command names to try
    let llvm_config_candidates = [
        // Direct command
        "llvm-config",
        // Version-suffixed (common on Ubuntu/Debian)
        &format!("llvm-config-{}", config::REQUIRED_LLVM_VERSION),
        // MacPorts prefix
        &format!("llvm-config-mp-{}", config::REQUIRED_LLVM_VERSION),
        // Homebrew paths (Intel Mac)
        &format!(
            "/usr/local/opt/llvm@{}/bin/llvm-config",
            config::REQUIRED_LLVM_VERSION
        ),
        "/usr/local/opt/llvm/bin/llvm-config",
        // Homebrew paths (Apple Silicon Mac)
        &format!(
            "/opt/homebrew/opt/llvm@{}/bin/llvm-config",
            config::REQUIRED_LLVM_VERSION
        ),
        "/opt/homebrew/opt/llvm/bin/llvm-config",
    ];

    for cmd in &llvm_config_candidates {
        let output = Command::new(cmd).arg("--version").output();

        if let Ok(output) = output
            && output.status.success()
        {
            let version_str = String::from_utf8_lossy(&output.stdout);
            let version = version_str.trim();

            // Check if it's LLVM 20.x.x
            if version.starts_with(&format!("{}.", config::REQUIRED_LLVM_VERSION)) {
                return Ok(version.to_string());
            }

            // If we found LLVM but it's not version 20, warn about it
            if !version.is_empty() {
                eprintln!(
                    "⚠ Found LLVM {} at '{}', but Zirco requires {}",
                    version,
                    cmd,
                    config::LLVM_VERSION_DESC
                );
            }
        }
    }

    Err(format!(
        "{} not found. Zirco REQUIRES {} specifically.\n  Install instructions:\n    - macOS (Homebrew): brew install llvm@{}\n    - macOS (MacPorts): sudo port install llvm-{}\n    - Ubuntu/Debian: sudo apt install llvm-{} llvm-{}-dev\n    - Windows: Download from https://releases.llvm.org/",
        config::LLVM_VERSION_DESC,
        config::LLVM_VERSION_DESC,
        config::REQUIRED_LLVM_VERSION,
        config::REQUIRED_LLVM_VERSION,
        config::REQUIRED_LLVM_VERSION,
        config::REQUIRED_LLVM_VERSION
    )
    .into())
}

/// Check if clang is installed
pub fn check_clang() -> Result<String, Box<dyn std::error::Error>> {
    let output = Command::new("clang").arg("--version").output();

    if let Ok(output) = output
        && output.status.success()
    {
        let version = String::from_utf8_lossy(&output.stdout);
        // Extract just the version line
        let version_line = version.lines().next().unwrap_or("unknown");
        return Ok(version_line.to_string());
    }

    Err("clang not found. Please install clang".into())
}

/// Warn about missing dependencies but don't fail
pub fn warn_dependencies() {
    println!("Checking dependencies...");

    match check_llvm() {
        Ok(version) => println!("✓ {} found: {}", config::LLVM_VERSION_DESC, version),
        Err(e) => {
            eprintln!("✗ {}", e);
        }
    }

    match check_clang() {
        Ok(version) => println!("✓ clang found: {}", version),
        Err(e) => {
            eprintln!("⚠ {}", e);
            eprintln!("  You may encounter build errors without clang.");
        }
    }
}
