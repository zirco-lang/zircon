//! LLVM and clang dependency checking

use std::process::Command;

use crate::config;

/// Check if LLVM 20 is installed (REQUIRED for Zirco)
pub fn check_llvm() -> Result<String, Box<dyn std::error::Error>> {
    // On Windows, llvm-config.exe is often not included in the installer
    // So we need to detect LLVM through alternative methods
    #[cfg(windows)]
    {
        if let Ok(version) = check_llvm_via_clang() {
            return Ok(version);
        }

        // Try to find LLVM in common Windows installation paths
        if let Ok(version) = check_llvm_windows_paths() {
            return Ok(version);
        }
    }

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

/// Check LLVM version via clang (Windows fallback when llvm-config is not available)
#[cfg(windows)]
fn check_llvm_via_clang() -> Result<String, Box<dyn std::error::Error>> {
    // Try clang and clang-cl (Windows-specific clang variant)
    for clang_cmd in ["clang", "clang-cl"] {
        let output = Command::new(clang_cmd).arg("--version").output();

        if let Ok(output) = output
            && output.status.success()
        {
            let version_str = String::from_utf8_lossy(&output.stdout);

            // Parse clang version output to extract LLVM version
            // Example output: "clang version 20.0.0git"
            // or "clang version 20.1.0"
            for line in version_str.lines() {
                if line.contains("clang version") || line.contains("LLVM version") {
                    // Extract version number
                    if let Some(version) = extract_llvm_version(line) {
                        // Check if it's LLVM 20.x.x
                        if version.starts_with(&format!("{}.", config::REQUIRED_LLVM_VERSION)) {
                            return Ok(version);
                        } else if !version.is_empty() {
                            eprintln!(
                                "⚠ Found LLVM {} via {}, but Zirco requires {}",
                                version,
                                clang_cmd,
                                config::LLVM_VERSION_DESC
                            );
                        }
                    }
                }
            }
        }
    }

    Err("LLVM not found via clang".into())
}

/// Check for LLVM in common Windows installation paths
#[cfg(windows)]
fn check_llvm_windows_paths() -> Result<String, Box<dyn std::error::Error>> {
    use std::path::PathBuf;

    // Common LLVM installation paths on Windows
    let base_paths = [r"C:\Program Files\LLVM", r"C:\Program Files (x86)\LLVM"];

    for base in &base_paths {
        let clang_path = PathBuf::from(base).join("bin").join("clang.exe");

        if clang_path.exists() {
            let output = Command::new(&clang_path).arg("--version").output();

            if let Ok(output) = output
                && output.status.success()
            {
                let version_str = String::from_utf8_lossy(&output.stdout);

                for line in version_str.lines() {
                    if line.contains("clang version") || line.contains("LLVM version") {
                        if let Some(version) = extract_llvm_version(line) {
                            // Check if it's LLVM 20.x.x
                            if version.starts_with(&format!("{}.", config::REQUIRED_LLVM_VERSION)) {
                                return Ok(version);
                            } else if !version.is_empty() {
                                eprintln!(
                                    "⚠ Found LLVM {} at '{}', but Zirco requires {}",
                                    version,
                                    base,
                                    config::LLVM_VERSION_DESC
                                );
                            }
                        }
                    }
                }
            }
        }
    }

    Err("LLVM not found in common Windows installation paths".into())
}

/// Extract LLVM version from clang version output
#[cfg(windows)]
fn extract_llvm_version(line: &str) -> Option<String> {
    // Try to extract version number from strings like:
    // "clang version 20.0.0git"
    // "clang version 20.1.0"
    // "LLVM version 20.0.0"

    // Find "version" keyword and extract the number after it
    if let Some(pos) = line.find("version") {
        let after_version = &line[pos + 7..].trim_start();

        // Extract the version number (digits and dots)
        let version: String = after_version
            .chars()
            .take_while(|c| c.is_ascii_digit() || *c == '.')
            .collect();

        if !version.is_empty() {
            return Some(version);
        }
    }

    None
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

/// Check dependencies and return error if LLVM 20 is missing (strict mode for bootstrap)
pub fn check_dependencies_strict() -> Result<(), Box<dyn std::error::Error>> {
    println!("Checking dependencies...");

    // LLVM is required - fail if not found
    match check_llvm() {
        Ok(version) => println!("✓ {} found: {}", config::LLVM_VERSION_DESC, version),
        Err(e) => {
            eprintln!("✗ {}", e);
            return Err(e);
        }
    }

    // Clang is recommended but not required
    match check_clang() {
        Ok(version) => println!("✓ clang found: {}", version),
        Err(e) => {
            eprintln!("⚠ {}", e);
            eprintln!("  You may encounter build errors without clang.");
        }
    }

    Ok(())
}
