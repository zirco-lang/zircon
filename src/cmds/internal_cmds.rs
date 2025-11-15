//! Internal commands for tooling and bootstrap

use std::error::Error;
use clap::Subcommand;

use crate::cli::DispatchCommand;
use crate::{paths, deps};

/// Internal commands (hidden from normal help)
#[derive(Subcommand)]
pub enum InternalCmds {
    /// Bootstrap command run by bootstrap.sh
    Bootstrap,
}

impl DispatchCommand for InternalCmds {
    fn dispatch(self) -> Result<(), Box<dyn Error>> {
        match self {
            Self::Bootstrap => cmd_bootstrap(),
        }
    }
}

/// Bootstrap the Zircon installation
fn cmd_bootstrap() -> Result<(), Box<dyn Error>> {
    println!("=== Zircon Bootstrap ===\n");
    
    // Check dependencies
    deps::warn_dependencies();
    
    // Ensure directories exist
    paths::ensure_directories()?;
    
    println!("\n✓ Bootstrap complete!");
    println!("\nZircon is installed at: {}", paths::zircon_root().display());
    
    #[cfg(windows)]
    {
        println!("\nNext steps:");
        println!("  1. Add Zircon to your PATH:");
        println!("     PowerShell: $env:Path = \"{};$env:Path\"", paths::bin_dir().display());
        println!("     CMD:        set PATH={};%PATH%", paths::bin_dir().display());
        println!("\n  2. Then load the environment with:");
        println!("     PowerShell: iex (zircon env --shell powershell)");
        println!("     CMD:        zircon env --shell cmd");
        println!("\n  3. Install a zrc version:");
        println!("     zircon build main");
        println!("     zircon build v0.1.0");
    }
    
    #[cfg(not(windows))]
    {
        println!("\nNext steps:");
        println!("  1. Add Zircon to your PATH:");
        println!("     export PATH=\"{}:$PATH\"", paths::bin_dir().display());
        println!("\n  2. Then load the environment with:");
        println!("     source <(zircon env)");
        println!("\n  3. Install a zrc version:");
        println!("     zircon build main");
        println!("     zircon build v0.1.0");
        println!("\n  To make these settings permanent, add to your shell profile (~/.bashrc, ~/.zshrc, etc.):");
        println!("     echo 'source <({}/zircon env)' >> ~/.bashrc", paths::bin_dir().display());
    }
    
    Ok(())
}
