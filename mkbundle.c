// See copyright notice in Copying.

#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef MKBUNDLE_TEST
#include "greatest.h"
#endif

#include "block.h"
#include "common-block.h"
#include "primary-block.h"
#include "strbuf.h"
#include "ui.h"
#include "util.h"

#ifndef MKBUNDLE_TEST
// Print the given formatted string and exit.
#define DIEF(fmt, ...) do { \
    fprintf(stderr, "error: " fmt "\n", __VA_ARGS__); \
    exit(EXIT_FAILURE); \
} while (0)

// Print the given string and exit.
#define DIES(str) do { \
    fputs("error: " str "\n", stderr); \
    exit(EXIT_FAILURE); \
} while (0)

// Handle getopt errors.
static void handle_opt(int o, const struct option *opts, char **argv) {
    const struct option *opt;

    switch (o) {
    case ':':
        for (opt = opts; opt->name; opt += 1)
            if (opt->val == optopt)
                DIEF("option '--%s' requires an argument", opt->name);

        DIEF("option '-%c' requires an argument", optopt);
    break;

    case '?':
        if (optopt)
            fprintf(stderr, "warning: unknown option '-%c'\n", optopt);
        else
            fprintf(stderr, "warning: unknown option '%s'\n", argv[optind - 1]);
    break;
    }
}

// Try to open a file and die on error.
static FILE *try_open(const char *path, const char *mode) {
    FILE *file = fopen(path, mode);

    if (!file)
        DIEF("unable to open '%s': %s", path, strerror(errno));

    return file;
}

static void help_primary(const char *name) {
    fprintf(stderr,
        "usage: %s primary [OPTION...]\n"
        "OPTIONS\n"
        "  -o FILE\n"
        "          output params to FILE instead of stdout\n"
        "  --version VERSION\n"
        "          set the bundle version\n"
        "  --flag FLAG\n"
        "          set a FLAG on the block (can be specified\n"
        "          multiple times)\n"
        "  --prio PRIORITY\n"
        "          set the bundle priority\n"
        "  --report REPORT\n"
        "          enable a status report (can be specified\n"
        "          multiple times)\n"
        "  --dest DESTINATION-SCHEME\n"
        "          set destination EID scheme\n"
        "  --src SOURCE-SCHEME\n"
        "          set source EID scheme\n"
        "  --report-to REPORT-TO-SCHEME\n"
        "          set report-to EID scheme\n"
        "  --custodian CUSTODIAN-SCHEME\n"
        "          set custodian EID scheme\n"
        "  --creation CREATION-TIMESTAMP\n"
        "          set the creation timestamp\n"
        "  --creation-seq CREATION-SEQUENCE-NUMBER\n"
        "          set the creation sequence number\n"
        "  --lifetime LIFETIME-OFFSET\n"
        "          set the lifetime offset\n"
        "FLAGS\n"
        "  bundle-is-fragment  bundle is a fragment\n"
        "  admin-record        application data unit is an administrative record\n"
        "  no-fragmentation    bundle must not be fragmented\n"
        "  custody-transfer    custody transfer is requested\n"
        "  singleton           destination endpoint is a singleton\n"
        "  ack                 acknowledgement by application is requested\n"
        "PRIORITIES\n"
        "  bulk       bulk class of service\n"
        "  normal     normal class of service\n"
        "  expedited  expedited class of service\n"
        "REPORTS\n"
        "  reception   request reporting of bundle reception\n"
        "  custody     request reporting of custody acceptance\n"
        "  forwarding  request reporting of bundle forwarding\n"
        "  delivery    request reporting of bundle delivery\n"
        "  deletion    request reporting of bundle deletion\n"
        ,
        name
    );
}

static void cmd_primary(const char *name, int argc, char **argv) {
    enum {
        OPT_HELP,
        OPT_VERSION,
        OPT_FLAG,
        OPT_PRIO,
        OPT_REPORT,
        OPT_DEST,
        OPT_SRC,
        OPT_REPORT_TO,
        OPT_CUSTODIAN,
        OPT_CREATION_TS,
        OPT_CREATION_SEQ,
        OPT_LIFETIME,
    };

    static const struct option OPTIONS[] = {
        {"help", no_argument, NULL, OPT_HELP},
        {"version", required_argument, NULL, OPT_VERSION},
        {"flag", required_argument, NULL, OPT_FLAG},
        {"prio", required_argument, NULL, OPT_PRIO},
        {"report", required_argument, NULL, OPT_REPORT},
        {"dest", required_argument, NULL, OPT_DEST},
        {"src", required_argument, NULL, OPT_SRC},
        {"report-to", required_argument, NULL, OPT_REPORT_TO},
        {"custodian", required_argument, NULL, OPT_CUSTODIAN},
        {"creation-ts", required_argument, NULL, OPT_CREATION_TS},
        {"creation-seq", required_argument, NULL, OPT_CREATION_SEQ},
        {"lifetime", required_argument, NULL, OPT_LIFETIME},
        {0},
    };

    FILE *out = stdout;
    int ret;
    char *end;

    primary_block_t block;
    primary_block_init(&block);

    while ((ret = getopt_long(argc, argv, ":ho:", OPTIONS, NULL)) >= 0) {
        switch (ret) {
        case 'h':
        case OPT_HELP:
            help_primary(name);
            exit(EXIT_SUCCESS);
        break;

        case 'o':
            out = try_open(optarg, "w");
        break;

        case OPT_VERSION:
            block.version = (uint8_t) strtoul(optarg, &end, 10);

            if (end == optarg)
                DIEF("invalid version '%s'", optarg);
        break;

        case OPT_FLAG:
            block.flags |= parse_primary_flag(optarg);

            if (block.flags & FLAG_INVALID)
                DIEF("invalid flag '%s'", optarg);
        break;

        case OPT_PRIO:
            block.flags &= PRIO_RESET;
            block.flags |= parse_prio(optarg);

            if (block.flags & FLAG_INVALID)
                DIEF("invalid priority '%s'", optarg);
        break;

        case OPT_REPORT:
            block.flags |= parse_report(optarg);

            if (block.flags & FLAG_INVALID)
                DIEF("invalid status report '%s'", optarg);
        break;

        case OPT_DEST:
            if (!primary_block_add_eid(&block, &block.dest, optarg))
                DIEF("invalid destination EID '%s'", optarg);
        break;

        case OPT_SRC:
            if (!primary_block_add_eid(&block, &block.src, optarg))
                DIEF("invalid source EID '%s'", optarg);
        break;

        case OPT_REPORT_TO:
            if (!primary_block_add_eid(&block, &block.report_to, optarg))
                DIEF("invalid report-to EID '%s'", optarg);
        break;

        case OPT_CUSTODIAN:
            if (!primary_block_add_eid(&block, &block.custodian, optarg))
                DIEF("invalid custodian EID '%s'", optarg);
        break;

        case OPT_CREATION_TS:
            block.creation_ts = (uint32_t) strtoul(optarg, &end, 10);

            if (end == optarg)
                DIEF("invalid creation timestamp '%s'", optarg);
        break;

        case OPT_CREATION_SEQ:
            block.creation_seq = (uint32_t) strtoul(optarg, &end, 10);

            if (end == optarg)
                DIEF("invalid creation sequence '%s'", optarg);
        break;

        case OPT_LIFETIME:
            block.lifetime = (uint32_t) strtoul(optarg, &end, 10);

            if (end == optarg)
                DIEF("invalid lifetime '%s'", optarg);
        break;

        default:
            handle_opt(ret, OPTIONS, argv);
        break;
        }
    }

    primary_block_serialize(&block, out);

    primary_block_destroy(&block);
    fclose(out);
}

static void help_extension(const char *name) {
    fprintf(stderr,
        "usage: %s extension [OPTION...]\n"
        "OPTIONS\n"
        "  -o FILE\n"
        "         output params to FILE instead of stdout\n"
        "  --type BLOCK-TYPE\n"
        "         set the block's type (can be an integer or a symbolic\n"
        "         BLOCK-TYPE)\n"
        "  --flag FLAG\n"
        "         set a FLAG on the block (can be specified multiple times)\n"
        "  --ref SCHEME-OFFSET:SSP-OFFSET\n"
        "         add an EID reference to the block\n"
        "  --payload-length PAYLOAD-LENGTH\n"
        "         set the payload length of the block\n"
        "FLAGS\n"
        "  replicate        block must be replicated in every fragment\n"
        "  transmit-status  transmit status report if block can't be processed\n"
        "  delete-bundle    delete bundle if block can't be processed\n"
        "  last-block       last block in the bundle\n"
        "  discard-block    discard block if it can't be processed\n"
        "  eid-ref          block contains an EID-reference field\n"
        "BLOCK-TYPES\n"
        "  payload  a payload block\n"
        "  phib     a PHIB block\n"
        ,
        name
    );
}

static void cmd_extension(const char *name, int argc, char **argv) {
    enum {
        OPT_HELP,
        OPT_TYPE,
        OPT_FLAG,
        OPT_REF,
        OPT_REF_COUNT,
        OPT_PAYLOAD_LENGTH,
    };

    static const struct option OPTIONS[] = {
        {"help", no_argument, NULL, OPT_HELP},
        {"type", required_argument, NULL, OPT_TYPE},
        {"flag", required_argument, NULL, OPT_FLAG},
        {"ref", required_argument, NULL, OPT_REF},
        {"payload-length", required_argument, NULL, OPT_PAYLOAD_LENGTH},
        {0},
    };

    FILE *out = stdout;
    int ret;
    char *end;

    ext_block_t block;
    ext_block_init(&block);

    while ((ret = getopt_long(argc, argv, ":ho:", OPTIONS, NULL)) >= 0) {
        switch (ret) {
        case 'h':
        case OPT_HELP:
            help_extension(name);
            exit(EXIT_SUCCESS);
        break;

        case 'o':
            out = try_open(optarg, "w");
        break;

        case OPT_TYPE:
            block.type = parse_ext_block_type(optarg);

            if (block.type != EXT_BLOCK_INVALID)
                break;

            block.type = (uint8_t) strtoul(optarg, &end, 10);

            if (end == optarg)
                DIEF("invalid block type '%s'", optarg);
        break;

        case OPT_FLAG:
            block.flags |= (uint8_t) parse_ext_flag(optarg);

            if (block.flags & FLAG_INVALID)
                DIEF("invalid flag '%s'", optarg);
        break;

        case OPT_REF:
            if (!ext_block_add_ref(&block, optarg))
                DIEF("invalid ref '%s'", optarg);
        break;

        case OPT_PAYLOAD_LENGTH:
            block.length = (uint32_t) strtoul(optarg, &end, 10);

            if (end == optarg)
                DIEF("invalid payload length '%s'", optarg);
        break;

        default:
            handle_opt(ret, OPTIONS, argv);
        break;
        }
    }

    ext_block_serialize(&block, out);

    fclose(out);
}

static void help_compile(const char *name) {
    fprintf(stderr,
        "usage: %s compile OPTION...\n"
        "OPTIONS\n"
        "  -i FILE\n"
        "         read params from FILE instead of stdin\n"
        "  -o FILE\n"
        "         output to FILE instead of stdout\n"
        ,
        name
    );
}

static void cmd_compile(const char *name, int argc, char **argv) {
    enum {
        OPT_HELP,
    };

    static const struct option OPTIONS[] = {
        {"help", no_argument, NULL, OPT_HELP},
        {0},
    };

    FILE *in = stdin;
    FILE *out = stdout;
    int ret;

    while ((ret = getopt_long(argc, argv, ":hi:o:", OPTIONS, NULL)) >= 0) {
        switch (ret) {
        case 'h':
        case OPT_HELP:
            help_compile(name);
            exit(EXIT_SUCCESS);
        break;

        case 'i':
            in = try_open(optarg, "r");
        break;

        case 'o':
            out = try_open(optarg, "w");
        break;

        default:
            handle_opt(ret, OPTIONS, argv);
        break;
        }
    }

    strbuf_t *buf;
    strbuf_init(&buf, 256);
    collect(&buf, in);

    block_t block;
    block_init(&block);

    if (!block_unserialize(&block, buf->buf, buf->pos))
        DIES("unable to unserialize block");

    block_write(&block, out);

    block_destroy(&block);
    strbuf_destroy(buf);
    fclose(out);
    fclose(in);
}

static void help_main(const char *name) {
    fprintf(stderr,
        "usage: %s COMMAND [OPTION...]\n"
        "COMMANDS\n"
        "  help       show help for a command\n"
        "  primary    create a primary block param file\n"
        "  extension  create an extension block param file\n"
        "  compile    compile a param file into binary\n"
        "\n"
        "See the help for each command for more informantion on specific\n"
        "options.\n"
        ,
        name
    );
}

static void cmd_help(const char *name, int argc, char **argv) {
    static const help_fn HELPS[] = {
        [CMD_HELP] = help_main,
        [CMD_PRIMARY] = help_primary,
        [CMD_EXTENSION] = help_extension,
        [CMD_COMPILE] = help_compile,
    };

    if (argc < 2) {
        help_main(name);
        exit(EXIT_SUCCESS);
    }

    cmd_t cmd = parse_cmd(argv[1]);

    if (cmd == CMD_INVALID)
        DIEF("invalid command '%s'", argv[1]);

    HELPS[cmd](name);
}

int main(int argc, char **argv) {
    static const cmd_fn CMDS[] = {
        [CMD_HELP] = cmd_help,
        [CMD_PRIMARY] = cmd_primary,
        [CMD_EXTENSION] = cmd_extension,
        [CMD_COMPILE] = cmd_compile,
    };

    opterr = 0;

    if (argc < 2)
        DIES("no command given");

    cmd_t cmd = parse_cmd(argv[1]);

    if (cmd == CMD_INVALID)
        DIEF("invalid command '%s'", argv[1]);

    // Run the command and strip off the initial argument.
    CMDS[cmd](argv[0], argc - 1, &argv[1]);
}
#else
extern SUITE(sdnv_suite);
extern SUITE(parser_suite);
extern SUITE(util_suite);
extern SUITE(primary_block_suite);
extern SUITE(ext_block_suite);
extern SUITE(block_suite);
extern SUITE(ui_suite);

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();

    RUN_SUITE(sdnv_suite);
    RUN_SUITE(parser_suite);
    RUN_SUITE(util_suite);
    RUN_SUITE(primary_block_suite);
    RUN_SUITE(ext_block_suite);
    RUN_SUITE(block_suite);
    RUN_SUITE(ui_suite);

    GREATEST_MAIN_END();
}
#endif
