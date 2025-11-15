//! Command line interface for Zircon.

use std::error::Error;

use clap::{ArgAction, Parser, Subcommand};

use crate::cmds::self_cmds;
use crate::cmds::build_cmds;
use crate::cmds::toolchain_cmds;
use crate::cmds::env_cmds;
use crate::cmds::internal_cmds;

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
    
    /// Build a specific version of zrc
    Build(build_cmds::BuildCmd),
    
    /// Switch to a different toolchain version
    Switch(toolchain_cmds::SwitchCmd),
    
    /// List installed toolchains
    List(toolchain_cmds::ListCmd),
    
    /// Delete a specific toolchain
    Delete(toolchain_cmds::DeleteCmd),
    
    /// Remove unused toolchains (keep only current)
    Prune(toolchain_cmds::PruneCmd),
    
    /// Output shell environment configuration
    Env(env_cmds::EnvCmd),
    
    /// Internal commands (for bootstrap and tooling)
    #[command(name = "_", subcommand, hide = true)]
    Internal(internal_cmds::InternalCmds),
}

/// A trait for dispatching commands
pub trait DispatchCommand {
    /// Dispatch the command
    fn dispatch(self) -> Result<(), Box<dyn Error>>;
}
