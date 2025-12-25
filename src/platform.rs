//! Platform detection utilities

use std::error::Error;

/// Detect the current platform and return the artifact name
///
/// # Errors
///
/// Returns an error if the platform is not supported
pub fn get_platform_artifact_name() -> Result<String, Box<dyn Error>> {
    let os = std::env::consts::OS;
    let arch = std::env::consts::ARCH;

    let platform_str = match (os, arch) {
        ("linux", "x86_64") => "linux-x64",
        ("linux", "aarch64") => "linux-arm64",
        ("macos", "x86_64") => "macos-x64",
        ("macos", "aarch64") => "macos-arm64",
        _ => {
            return Err(format!(
                "Unsupported platform: {} {}. Pre-built binaries are only available for:\n\
                 - Linux x64 (linux-x64)\n\
                 - Linux ARM64 (linux-arm64)\n\
                 - macOS x64 (macos-x64)\n\
                 - macOS ARM64 (macos-arm64)\n\
                 \n\
                 Consider using 'zircon build' to build from source instead.",
                os,
                arch
            )
            .into());
        }
    };

    Ok(format!("zrc-{}.tar.gz", platform_str))
}

#[cfg(test)]
mod tests {
    #[test]
    fn test_get_platform_artifact_name() {
        // Just test that it doesn't panic
        let _ = super::get_platform_artifact_name();
    }
}
