//! Commands to manage Zircon itself

mod cmd_version;

use std::error::Error;
use std::fs;

use clap::{Parser, Subcommand};

use crate::cli::DispatchCommand;

/// Valid subcommands on `zircon self`
#[derive(Subcommand)]
pub enum SelfCmds {
    /// Print Zircon's version
    Version,

    /// Update Zircon itself
    Update(UpdateSelfCmd),
}

/// Update Zircon itself to a specific version
#[derive(Parser)]
pub struct UpdateSelfCmd {
    /// Git reference to update to (branch, tag, or commit). Defaults to 'main'
    #[arg(default_value = "main")]
    pub reference: String,
}

impl DispatchCommand for SelfCmds {
    fn dispatch(self) -> Result<(), Box<dyn Error>> {
        match self {
            Self::Version => {
                cmd_version::cmd_version();
                Ok(())
            }
            Self::Update(cmd) => cmd_self_update(&cmd.reference),
        }
    }
}

/// Update Zircon itself
fn cmd_self_update(reference: &str) -> Result<(), Box<dyn Error>> {
    use crate::{build, git_utils, paths};

    println!("Updating Zircon to '{}'...", reference);

    let zircon_source = paths::zircon_source_dir();

    // Clone or open the zircon repository
    let repo =
        git_utils::clone_or_open("https://github.com/zirco-lang/zircon.git", &zircon_source)?;

    // Fetch and checkout the specified reference
    git_utils::fetch(&repo)?;
    git_utils::checkout_ref(&repo, reference)?;

    println!("Building Zircon...");
    build::check_cargo()?;
    build::build_zrc(&zircon_source)?; // Reuse the build function, it just runs cargo build --release

    // Copy the new binary
    let binary_name = if cfg!(windows) {
        "zircon.exe"
    } else {
        "zircon"
    };
    let new_binary = zircon_source
        .join("target")
        .join("release")
        .join(binary_name);
    let self_binary = paths::self_zircon_binary();

    if !new_binary.exists() {
        return Err("Failed to build new Zircon binary".into());
    }

    println!("Installing updated binary...");
    fs::copy(&new_binary, &self_binary)?;

    // Make executable on Unix
    #[cfg(unix)]
    {
        use std::os::unix::fs::PermissionsExt;
        let mut perms = fs::metadata(&self_binary)?.permissions();
        perms.set_mode(0o755);
        fs::set_permissions(&self_binary, perms)?;
    }

    // Update the link in bin
    let zircon_link = paths::zircon_binary_link();
    paths::create_link(&self_binary, &zircon_link)?;

    println!("✓ Zircon updated successfully to '{}'!", reference);

    Ok(())
}
