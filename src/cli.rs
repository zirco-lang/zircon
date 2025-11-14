//! Command line interface for Zircon.

use std::error::Error;

use clap::{ArgAction, Parser, Subcommand};

use crate::cmds::self_cmds;

/// The Zircon toolchain installer and build tool
#[derive(Parser)]
#[command(version, disable_version_flag = true)]
pub struct Cli {
    /// See what version of Zircon you are using
    #[arg(short, long, action = ArgAction::Version)]
    pub version: (),

    /// The command to run
    #[command(subcommand)]
    pub command: ZirconCommand,
}

/// The command to run
#[derive(Subcommand)]
pub enum ZirconCommand {
    /// Commands to manage Zircon itself
    #[command(name = "self", subcommand)]
    SelfCmds(self_cmds::SelfCmds),
}

/// A trait for dispatching commands
pub trait DispatchCommand {
    /// Dispatch the command
    fn dispatch(self) -> Result<(), Box<dyn Error>>;
}
