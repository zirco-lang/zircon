//! LLVM and clang dependency checking

use std::process::Command;

/// Check if LLVM is installed
pub fn check_llvm() -> Result<String, Box<dyn std::error::Error>> {
    // Try llvm-config first
    let output = Command::new("llvm-config")
        .arg("--version")
        .output();
    
    if let Ok(output) = output
        && output.status.success() {
            let version = String::from_utf8_lossy(&output.stdout);
            return Ok(version.trim().to_string());
        }
    
    // Try llvm-config with version suffix (common on some systems)
    for version in &["20", "19", "18", "17", "16", "15"] {
        let cmd_name = format!("llvm-config-{}", version);
        let output = Command::new(&cmd_name)
            .arg("--version")
            .output();
        
        if let Ok(output) = output
            && output.status.success() {
                let ver = String::from_utf8_lossy(&output.stdout);
                return Ok(ver.trim().to_string());
            }
    }
    
    Err("LLVM not found. Please install LLVM (preferably version 20)".into())
}

/// Check if clang is installed
pub fn check_clang() -> Result<String, Box<dyn std::error::Error>> {
    let output = Command::new("clang")
        .arg("--version")
        .output();
    
    if let Ok(output) = output
        && output.status.success() {
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
        Ok(version) => println!("✓ LLVM found: {}", version),
        Err(e) => {
            eprintln!("⚠ {}", e);
            eprintln!("  You may encounter build errors without LLVM.");
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
