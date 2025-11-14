//! `zircon self version`

/// `zircon self version` command implementation
pub fn cmd_version() {
    println!("zircon {}", env!("CARGO_PKG_VERSION"));
}
