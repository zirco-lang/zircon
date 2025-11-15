//! LLVM and clang dependency checking

use std::process::Command;

/// Check if LLVM 20 is installed (REQUIRED for Zirco)
pub fn check_llvm() -> Result<String, Box<dyn std::error::Error>> {
    // List of possible llvm-config command names to try
    let llvm_config_candidates = [
        // Direct command
        "llvm-config",
        // Version-suffixed (common on Ubuntu/Debian)
        "llvm-config-20",
        // MacPorts prefix
        "llvm-config-mp-20",
        // Homebrew paths (Intel Mac)
        "/usr/local/opt/llvm@20/bin/llvm-config",
        "/usr/local/opt/llvm/bin/llvm-config",
        // Homebrew paths (Apple Silicon Mac)
        "/opt/homebrew/opt/llvm@20/bin/llvm-config",
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
            if version.starts_with("20.") {
                return Ok(version.to_string());
            }

            // If we found LLVM but it's not version 20, warn about it
            if !version.is_empty() {
                eprintln!(
                    "⚠ Found LLVM {} at '{}', but Zirco requires LLVM 20.x",
                    version, cmd
                );
            }
        }
    }

    Err("LLVM 20 not found. Zirco REQUIRES LLVM 20.x specifically.\n  Install instructions:\n    - macOS (Homebrew): brew install llvm@20\n    - macOS (MacPorts): sudo port install llvm-20\n    - Ubuntu/Debian: sudo apt install llvm-20 llvm-20-dev\n    - Windows: Download from https://releases.llvm.org/".into())
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
        Ok(version) => println!("✓ LLVM 20 found: {}", version),
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
