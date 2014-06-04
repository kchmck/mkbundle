// See copyright notice in Copying.

#include <assert.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef MKBUNDLE_TEST
#include "greatest.h"
#endif

#include "block.h"
#include "primary-block.h"
#include "sdnv.h"
#include "strbuf.h"

#define HTABLE_COMMON
#include "htable.h"

static htable_hash_t fnv(const char *key) {
    htable_hash_t hval = 0x811c9dc5;

    while (*key) {
        hval ^= *key;
        hval *= 0x01000193;

        key += 1;
    }

    return hval;
}

#define HTABLE_RESET
#include "htable.h"
#define HTABLE_NAME eid_map
#define HTABLE_KEY_TYPE const char *
#define HTABLE_DATA_TYPE size_t
#define HTABLE_HASH_KEY(key) fnv(key)
#define HTABLE_DEFAULT_SIZE (1 << 6)
#include "htable.h"
#include "htable.c"

#define CHECK(str, flag) do { \
    if (strcmp(s, str) == 0) \
        return flag; \
} while (0)

static flag_t flag_parse(const char *s) {
    CHECK("bundle-is-fragment", FLAG_IS_FRAGMENT);
    CHECK("admin-record", FLAG_ADMIN);
    CHECK("no-fragmentation", FLAG_NO_FRAGMENT);
    CHECK("custody-transfer", FLAG_CUSTODY);
    CHECK("singleton", FLAG_SINGLETON);
    CHECK("ack", FLAG_ACK);

    return FLAG_INVALID;
}

#ifdef MKBUNDLE_TEST
TEST test_flag_parse() {
    ASSERT_EQ(flag_parse("singleton"), FLAG_SINGLETON);
    ASSERT_EQ(flag_parse("BEEP"), FLAG_INVALID);

    PASS();
}
#endif

static flag_t prio_parse(const char *s) {
    CHECK("bulk", PRIO_BULK);
    CHECK("normal", PRIO_NORMAL);
    CHECK("expedited", PRIO_EXPEDITED);

    return FLAG_INVALID;
}

#ifdef MKBUNDLE_TEST
TEST test_prio_parse() {
    ASSERT_EQ(prio_parse("expedited"), PRIO_EXPEDITED);
    ASSERT_EQ(prio_parse("expedited"), 0x0100);
    ASSERT_EQ(prio_parse("BEEP"), FLAG_INVALID);

    PASS();
}
#endif

static flag_t report_parse(const char *s) {
    CHECK("reception", REPORT_RECEPTION);
    CHECK("custody", REPORT_CUSTODY);
    CHECK("forwarding", REPORT_FORWARDING);
    CHECK("delivery", REPORT_DELIVERY);
    CHECK("deletion", REPORT_DELETION);

    return FLAG_INVALID;
}

#ifdef MKBUNDLE_TEST
TEST test_report_parse() {
    ASSERT_EQ(report_parse("reception"), REPORT_RECEPTION);
    ASSERT_EQ(report_parse("BOOP"), FLAG_INVALID);

    PASS();
}
#endif

#undef CHECK

// Print the given formatted string and exit.
#define DIEF(fmt, ...) do { \
    fprintf(stderr, "error: " fmt "\n", __VA_ARGS__); \
    exit(EXIT_FAILURE); \
} while (0)

#ifndef MKBUNDLE_TEST
int main(int argc, char **argv) {
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
        {"creation", required_argument, NULL, OPT_CREATION_TS},
        {"creation-seq", required_argument, NULL, OPT_CREATION_SEQ},
        {"lifetime", required_argument, NULL, OPT_LIFETIME},
        {0},
    };

    opterr = 0;

    bundle_params_t params;
    bundle_params_init(&params);

    eid_map_t *eid_map;
    eid_map_init(&eid_map);

    const struct option *opt;
    int ret;
    char *end;

    while ((ret = getopt_long(argc, argv, ":h", OPTIONS, NULL)) >= 0) {
        switch (ret) {
        case 'h':
        case OPT_HELP:
            fprintf(stderr,
                "usage: %s OPTION...\n"
                "OPTIONS:\n"
                "  --version VERSION\n"
                "          set the bundle version\n"
                "  --flag FLAG\n"
                "          set a FLAG on the bundle (can be specified\n"
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
                "FLAGS:\n"
                "  bundle-is-fragment\n"
                "  admin-record\n"
                "  no-fragmentation\n"
                "  custody-tranfer\n"
                "  singleton\n"
                "  ack\n"
                "PRIORITIES:\n"
                "  bulk\n"
                "  normal\n"
                "  expedited\n"
                "REPORTS:\n"
                "  reception\n"
                "  custody\n"
                "  forwarding\n"
                "  delivery\n"
                "  deletion\n"
                ,
                argv[0]
            );

            exit(EXIT_SUCCESS);
        break;

        case OPT_VERSION:
            params.version = strtoul(optarg, &end, 10);

            if (end == optarg)
                DIEF("invalid version '%s'", optarg);
        break;

        case OPT_FLAG:
            params.flags |= flag_parse(optarg);

            if (params.flags & FLAG_INVALID)
                DIEF("invalid flag '%s'", optarg);
        break;

        case OPT_PRIO:
            params.flags &= PRIO_RESET;
            params.flags |= prio_parse(optarg);

            if (params.flags & FLAG_INVALID)
                DIEF("invalid priority '%s'", optarg);
        break;

        case OPT_REPORT:
            params.flags |= report_parse(optarg);

            if (params.flags & FLAG_INVALID)
                DIEF("invalid status report '%s'", optarg);
        break;

#define ADD_EID(field, str, len) do { \
    size_t *pos = eid_map_lookup(eid_map, (str)); \
    if (!pos) { \
        pos = eid_map_add(&eid_map, (str)); \
        assert(pos); \
        *pos = params.dict->pos; \
        strbuf_append(&params.dict, (uint8_t *)(str), (len)); \
        strbuf_finish(&params.dict); \
    } \
    *(field) = *pos; \
} while (0)

#define PARSE_EID(desc, eid) do { \
    char *sep = strchr(optarg, ':'); \
    if (!sep) \
        DIEF("invalid " desc " EID '%s'", optarg); \
    *sep = 0; \
    ADD_EID(&(eid)->scheme, optarg, sep - optarg); \
    sep += 1; \
    ADD_EID(&(eid)->ssp, sep, strlen(sep)); \
} while (0)

        case OPT_DEST:
            PARSE_EID("destination", &params.dest);
        break;

        case OPT_SRC:
            PARSE_EID("source", &params.src);
        break;

        case OPT_REPORT_TO:
            PARSE_EID("report-to", &params.report_to);
        break;

        case OPT_CUSTODIAN:
            PARSE_EID("custodian", &params.custodian);
        break;

#undef PARSE_EID
#undef ADD_EID

        case OPT_CREATION_TS:
            params.creation_ts = strtoul(optarg, &end, 10);

            if (end == optarg)
                DIEF("invalid creation timestamp '%s'", optarg);
        break;

        case OPT_CREATION_SEQ:
            params.creation_seq = strtoul(optarg, &end, 10);

            if (end == optarg)
                DIEF("invalid creation sequence '%s'", optarg);
        break;

        case OPT_LIFETIME:
            params.lifetime = strtoul(optarg, &end, 10);

            if (end == optarg)
                DIEF("invalid lifetime '%s'", optarg);
        break;

        case ':':
            for (opt = OPTIONS; opt; opt += 1)
                if (opt->val == optopt)
                    DIEF("option '%s' requires an argument", opt->name);
        break;

        case '?':
            fprintf(stderr, "warning: unknown option\n");
        break;
        }
    }

    static const uint8_t DATA[] = "hello";
    block_params_t block_params;
    block_params_init(&block_params, DATA, sizeof(DATA));

    block_t block;
    block_init(&block, &block_params);
    block_build(&block);

    bundle_t bundle;
    bundle_init(&bundle, &params, &block);
    bundle_build(&bundle);

    FILE *f = fopen("test.bundle", "wb");
    bundle_write(&bundle, f);
    fclose(f);

    block_destroy(&block);

    bundle_destroy(&bundle);
    bundle_params_destroy(&params);

    eid_map_destroy(eid_map);
}
#else
extern SUITE(sdnv_suite);

SUITE(mkbundle_suite) {
    RUN_TEST(test_flag_parse);
    RUN_TEST(test_prio_parse);
    RUN_TEST(test_report_parse);
}

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();
    GREATEST_RUN_SUITE(sdnv_suite);
    GREATEST_RUN_SUITE(mkbundle_suite);
    GREATEST_MAIN_END();
}
#endif
