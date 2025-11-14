//! Command line interface for Zircon.

use clap::{ArgAction, Parser};

/// The Zircon toolchain installer and build tool
#[derive(Parser)]
#[command(version, disable_version_flag = true)]
pub struct Cli {
    /// See what version of Zircon you are using
    #[arg(short, long, action = ArgAction::Version)]
    pub version: (),

    /// The command to run
    #[command(subcommand)]
    pub command: Command,
}

/// The command to run
#[derive(Parser)]
pub enum Command {
    // X,
}
