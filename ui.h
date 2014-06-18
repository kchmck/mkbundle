// See copyright notice in Copying.

#ifndef UI_H
#define UI_H

#include <inttypes.h>

#include "ext-block.h"

// Signature of a command function and a help function.
typedef void (*cmd_fn)(const char *name, int argc, char **argv);
typedef void (*help_fn)(const char *name);

typedef enum {
    CMD_HELP,
    CMD_PRIMARY,
    CMD_EXTENSION,
    CMD_COMPILE,

    CMD_INVALID,
} cmd_t;

// Parse the given string into a command. Return CMD_INVALID on error.
cmd_t parse_cmd(const char *str);

// Parse the given string into a block flag. Return FLAG_INVALID on error.
uint32_t parse_primary_flag(const char *str);
uint32_t parse_ext_flag(const char *str);

// Parse the given string into a bundle priority flag. Return FLAG_INVALID on
// error.
uint32_t parse_prio(const char *str);

// Parse the given string into a status report flag. Return FLAG_INVALID on
// error.
uint32_t parse_report(const char *str);

// Parse the given string into an extension block type. Return EXT_BLOCK_INVALID
// on error.
ext_block_type_t parse_ext_block_type(const char *str);

#endif
