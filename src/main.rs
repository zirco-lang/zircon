#![doc=include_str!("../README.md")]
#![allow(unknown_lints)] // in case you use non-nightly clippy
#![warn(
    clippy::cargo,
    clippy::nursery,
    clippy::pedantic,
    clippy::missing_docs_in_private_items,
    missing_docs,
    clippy::dbg_macro,
    clippy::decimal_literal_representation,
    clippy::deref_by_slicing,
    clippy::disallowed_script_idents,
    clippy::empty_structs_with_brackets,
    clippy::format_push_string,
    clippy::if_then_some_else_none,
    clippy::let_underscore_must_use,
    clippy::mixed_read_write_in_expression,
    clippy::multiple_inherent_impl,
    clippy::multiple_unsafe_ops_per_block,
    clippy::redundant_type_annotations,
    clippy::rest_pat_in_fully_bound_structs,
    clippy::same_name_method,
    clippy::semicolon_inside_block,
    clippy::unseparated_literal_suffix,
    clippy::implicit_clone,
    clippy::todo,
    clippy::undocumented_unsafe_blocks,
    clippy::unimplemented,
    clippy::unneeded_field_pattern,
    clippy::wildcard_enum_match_arm,
    let_underscore_drop,
    macro_use_extern_crate,
    missing_debug_implementations,
    non_exhaustive_omitted_patterns,
    unsafe_op_in_unsafe_fn,
    unused_crate_dependencies,
    variant_size_differences,
    unused_qualifications,
    clippy::unwrap_used
)]
#![allow(
    clippy::multiple_crate_versions,
    clippy::cargo_common_metadata,
    clippy::module_name_repetitions,
    clippy::doc_comment_double_space_linebreaks,
    clippy::else_if_without_else,
    clippy::min_ident_chars,
    clippy::non_ascii_literal,
    clippy::absolute_paths,
    clippy::uninlined_format_args
)]

mod build;
mod cli;
mod cmds;
mod config;
mod deps;
mod git_utils;
mod paths;
mod toolchains;
mod update_check;

use std::error::Error;

use clap::Parser;
use cli::{Cli, DispatchCommand, ZirconCommand};

fn main() -> Result<(), Box<dyn Error>> {
    let cli = Cli::parse();

    // Check for updates (non-blocking, best effort)
    update_check::check_for_updates();

    match cli.command {
        ZirconCommand::SelfCmds(self_cmds) => self_cmds.dispatch(),
        ZirconCommand::Build(build_cmd) => build_cmd.dispatch(),
        ZirconCommand::Install(install_cmd) => install_cmd.dispatch(),
        ZirconCommand::Import(import_cmd) => import_cmd.dispatch(),
        ZirconCommand::Switch(switch_cmd) => switch_cmd.dispatch(),
        ZirconCommand::List(list_cmd) => list_cmd.dispatch(),
        ZirconCommand::Delete(delete_cmd) => delete_cmd.dispatch(),
        ZirconCommand::Prune(prune_cmd) => prune_cmd.dispatch(),
        ZirconCommand::Env(env_cmd) => env_cmd.dispatch(),
        ZirconCommand::Internal(internal_cmds) => internal_cmds.dispatch(),
    }
}
