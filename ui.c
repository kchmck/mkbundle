// See copyright notice in Copying.

#include <inttypes.h>
#include <string.h>

#include "common-block.h"
#include "primary-block.h"
#include "ui.h"
#include "util.h"

#ifdef MKBUNDLE_TEST
#include "greatest.h"
#endif

cmd_t parse_cmd(const char *str) {
    static const sym_t MAP[] = {
        {CMD_HELP, "--help"},
        {CMD_HELP, "-h"},
        {CMD_HELP, "help"},
        {CMD_PRIMARY, "primary"},
        {CMD_COMPILE, "compile"},
    };

    uint32_t cmd = sym_parse(str, MAP, ASIZE(MAP));

    if (cmd == SYM_INVALID)
        return CMD_INVALID;

    return (cmd_t) cmd;
}

uint32_t parse_primary_flag(const char *str) {
    static const sym_t MAP[] = {
        {FLAG_IS_FRAGMENT, "bundle-is-fragment"},
        {FLAG_ADMIN, "admin-record"},
        {FLAG_NO_FRAGMENT, "no-fragmentation"},
        {FLAG_CUSTODY, "custody-transfer"},
        {FLAG_SINGLETON, "singleton"},
        {FLAG_ACK, "ack"},
    };

    uint32_t flag = sym_parse(str, MAP, ASIZE(MAP));

    if (flag == SYM_INVALID)
        return FLAG_INVALID;

    return flag;
}

uint32_t parse_prio(const char *str) {
    static const sym_t MAP[] = {
        {PRIO_BULK, "bulk"},
        {PRIO_NORMAL, "normal"},
        {PRIO_EXPEDITED, "expedited"},
    };

    uint32_t prio = sym_parse(str, MAP, ASIZE(MAP));

    if (prio == SYM_INVALID)
        return FLAG_INVALID;

    return prio;
}

uint32_t parse_report(const char *str) {
    static const sym_t MAP[] = {
        {REPORT_RECEPTION, "reception"},
        {REPORT_CUSTODY, "custody"},
        {REPORT_FORWARDING, "forwarding"},
        {REPORT_DELIVERY, "delivery"},
        {REPORT_DELETION, "deletion"},
    };

    uint32_t report = sym_parse(str, MAP, ASIZE(MAP));

    if (report == SYM_INVALID)
        return FLAG_INVALID;

    return report;
}

ext_block_type_t parse_ext_block_type(const char *str) {
    static const sym_t MAP[] = {
        {EXT_BLOCK_PAYLOAD, "payload"},
    };

    uint32_t type = sym_parse(str, MAP, ASIZE(MAP));

    if (type == SYM_INVALID)
        return EXT_BLOCK_INVALID;

    return (ext_block_type_t) type;
}

#ifdef MKBUNDLE_TEST
SUITE(ui_suite) {
}
#endif
