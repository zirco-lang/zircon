#ifndef ZIRCON_TOOLCHAIN_CMDS_H
#define ZIRCON_TOOLCHAIN_CMDS_H

#include "../cli.h"

int dispatch_switch_command(cli_context_t* ctx);
int dispatch_import_command(cli_context_t* ctx);
int dispatch_list_command(cli_context_t* ctx);
int dispatch_delete_command(cli_context_t* ctx);
int dispatch_prune_command(cli_context_t* ctx);

#endif
