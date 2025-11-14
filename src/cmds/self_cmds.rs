//! Commands to manage Zircon itself

mod cmd_version;

use std::error::Error;

use clap::Subcommand;

use crate::cli::DispatchCommand;

/// Valid subcommands on `zircon self`
#[derive(Subcommand)]
pub enum SelfCmds {
    /// Print Zircon's version
    Version,
}
impl DispatchCommand for SelfCmds {
    fn dispatch(self) -> Result<(), Box<dyn Error>> {
        match self {
            Self::Version => {
                cmd_version::cmd_version();
                Ok(())
            }
        }
    }
}
