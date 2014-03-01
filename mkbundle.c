// See copyright notice in Copying.

#include <assert.h>
#include <getopt.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef MKBUNDLE_TEST
#include "greatest.h"
#endif

#include "strbuf.h"
#include "sdnv.h"

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

enum { DEFAULT_VERSION = 0x06};

typedef struct {
    uint8_t version;
    uint32_t flags;
    uint32_t priority;

    const char *dest;
    const char *dest_ssp;

    const char *src;
    const char *src_ssp;

    const char *report_to;
    const char *report_to_ssp;

    const char *custodian;
    const char *custodian_ssp;

    int creation_ts;
    int creation_ts_seq;
    int lifetime;
} bundle_params_t;

static void bundle_params_init(bundle_params_t *p) {
    *p = (bundle_params_t) {
        .version = DEFAULT_VERSION,
    };
}

static bool bundle_params_verify(const bundle_params_t *p) {
    return p->dest && p->dest_ssp && p->src && p->src_ssp &&
           p->report_to && p->report_to_ssp && p->custodian &&
           p->custodian_ssp && p->creation_ts && p->creation_ts_seq;
}

typedef enum {
    FLAG_IS_FRAGMENT = 1 << 0,
    FLAG_ADMIN = 1 << 1,
    FLAG_NO_FRAGMENT = 1 << 2,
    FLAG_CUSTODY = 1 << 3,
    FLAG_SINGLETON = 1 << 4,
    FLAG_ACK = 1 << 5,

    FLAG_INVALID,
} flag_t;

typedef enum {
    PRIO_BULK = 0x0 << 7,
    PRIO_NORMAL = 0x1 << 7,
    PRIO_EXPEDITED = 0x2 << 7,

    PRIO_INVALID,
} prio_t;

typedef enum {
    REPORT_RECEPTION = 1 << 14,
    REPORT_CUSTODY = 1 << 15,
    REPORT_FORWARDING = 1 << 16,
    REPORT_DELIVERY = 1 << 17,
    REPORT_DELETION = 1 << 18,

    REPORT_INVALID,
} report_t;

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

static prio_t prio_parse(const char *s) {
    CHECK("bulk", PRIO_BULK);
    CHECK("normal", PRIO_NORMAL);
    CHECK("expedited", PRIO_EXPEDITED);

    return PRIO_INVALID;
}

#ifdef MKBUNDLE_TEST
TEST test_prio_parse() {
    ASSERT_EQ(prio_parse("expedited"), PRIO_EXPEDITED);
    ASSERT_EQ(prio_parse("expedited"), 0x0100);
    ASSERT_EQ(prio_parse("BEEP"), PRIO_INVALID);

    PASS();
}
#endif

static report_t report_parse(const char *s) {
    CHECK("reception", REPORT_RECEPTION);
    CHECK("custody", REPORT_CUSTODY);
    CHECK("forwarding", REPORT_FORWARDING);
    CHECK("delivery", REPORT_DELIVERY);
    CHECK("deletion", REPORT_DELETION);

    return REPORT_INVALID;
}

#ifdef MKBUNDLE_TEST
TEST test_report_parse() {
    ASSERT_EQ(report_parse("reception"), REPORT_RECEPTION);
    ASSERT_EQ(report_parse("BOOP"), REPORT_INVALID);

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
        OPT_VER,
        OPT_FLAG,
        OPT_PRIO,
        OPT_REPORT,
        OPT_DEST,
        OPT_DEST_SSP,
        OPT_SRC,
        OPT_SRC_SSP,
        OPT_REPORT_TO,
        OPT_REPORT_TO_SSP,
        OPT_CUST,
        OPT_CUST_SSP,
        OPT_CREATION,
        OPT_CREATION_SEQ,
        OPT_LIFETIME,
    };

    static const struct option OPTIONS[] = {
        {"help", no_argument, NULL, OPT_HELP},
        {"ver", required_argument, NULL, OPT_VER},
        {"flag", required_argument, NULL, OPT_FLAG},
        {"priority", required_argument, NULL, OPT_PRIO},
        {"report", required_argument, NULL, OPT_REPORT},
        {"dest", required_argument, NULL, OPT_DEST},
        {"dest-ssp", required_argument, NULL, OPT_DEST_SSP},
        {"src", required_argument, NULL, OPT_SRC},
        {"src-ssp", required_argument, NULL, OPT_SRC_SSP},
        {"report-to", required_argument, NULL, OPT_REPORT_TO},
        {"report-to-ssp", required_argument, NULL, OPT_REPORT_TO_SSP},
        {"custodian", required_argument, NULL, OPT_CUST},
        {"custodian-ssp", required_argument, NULL, OPT_CUST_SSP},
        {"creation", required_argument, NULL, OPT_CREATION},
        {"creation-seqnum", required_argument, NULL, OPT_CREATION_SEQ},
        {"lifetime", required_argument, NULL, OPT_LIFETIME},
        {0},
    };

    opterr = 0;

    bundle_params_t params = {0};
    flag_t flag;
    prio_t prio;
    report_t report;

    const struct option *option;
    int opt;
    char *end;

    while ((opt = getopt_long(argc, argv, ":h", OPTIONS, NULL)) >= 0) {
        switch (opt) {
        case 'h':
        case OPT_HELP:
            fprintf(stderr,
                "usage: %s OPTION...\n"
                "OPTION:\n"
                "  --ver VERSION  set the bundle version\n"
                "  --flag FLAG    set a FLAG on the bundle (can be specified\n"
                "                 multiple times)"
                "  --priority PRIORITY"
                "                 set the bundle priority\n"
                "  --report REPORT"
                "                 enable a status report (can be specified\n"
                "                 multiple times)"
                "  --dest DESTINATION-SCHEME\n"
                "                 set destination EID scheme\n"
                "  --dest-ssp DESTINATION-SSP-SCHEME\n"
                "                 set destination-ssp EID scheme\n"
                "  --src SOURCE-SCHEME\n"
                "                 set source EID scheme\n"
                "  --src-ssp SOURCE-SSP-SCHEME\n"
                "                 set source-ssp EID scheme\n"
                "  --report-to REPORT-TO-SCHEME\n"
                "                 set report-to EID scheme\n"
                "  --report-to-ssp REPORT-TO-SSP-SCHEME\n"
                "                 set report-to-ssp EID scheme\n"
                "  --custodian CUSTODIAN-SCHEME\n"
                "                 set custodian EID scheme\n"
                "  --custodian-ssp CUSTODIAN-SSP-SCHEME\n"
                "                 set custodian-ssp EID scheme\n"
                "  --creation CREATION-TIMESTAMP\n"
                "                 set the creation timestamp\n"
                "  --creation-seqnum CREATION-SEQUENCE-NUMBER\n"
                "                 set the creation sequence number\n"
                "  --lifetime LIFETIME-OFFSET\n"
                "                 set the lifetime offset\n"
                "FLAG:\n"
                "  bundle-is-fragment\n"
                "  admin-record\n"
                "  no-fragmentation\n"
                "  custody-tranfer\n"
                "  singleton\n"
                "  ack\n"
                "PRIORITY:\n"
                "  bulk\n"
                "  normal\n"
                "  expedited\n"
                "REPORT:\n"
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

        case OPT_VER:
            params.version = strtol(optarg, &end, 10);

            if (end == optarg)
                DIEF("invalid version '%s'", optarg);
        break;

        case OPT_FLAG:
            flag = flag_parse(optarg);

            if (flag >= FLAG_INVALID)
                DIEF("invalid flag '%s'", optarg);

            params.flags |= 1 << flag;
        break;

        case OPT_PRIO:
            prio = prio_parse(optarg);

            if (prio >= PRIO_INVALID)
                DIEF("invalid priority '%s'", optarg);

            params.priority = prio;
        break;

        case OPT_REPORT:
            report = report_parse(optarg);

            if (report >= REPORT_INVALID)
                DIEF("invalid status report '%s'", optarg);

            params.flags |= report;
        break;

        case OPT_DEST:
            params.dest = optarg;
        break;

        case OPT_DEST_SSP:
            params.dest_ssp = optarg;
        break;

        case OPT_SRC:
            params.src = optarg;
        break;

        case OPT_SRC_SSP:
            params.src_ssp = optarg;
        break;

        case OPT_REPORT_TO:
            params.report_to = optarg;
        break;

        case OPT_REPORT_TO_SSP:
            params.report_to_ssp = optarg;
        break;

        case OPT_CUST:
            params.custodian = optarg;
        break;

        case OPT_CUST_SSP:
            params.custodian_ssp = optarg;
        break;

        case OPT_CREATION:
            params.creation_ts = strtol(optarg, &end, 10);

            if (end == optarg)
                DIEF("invalid creation timestamp '%s'", optarg);
        break;

        case OPT_CREATION_SEQ:
            params.creation_ts_seq = strtol(optarg, &end, 10);

            if (end == optarg)
                DIEF("invalid creation sequence '%s'", optarg);
        break;

        case OPT_LIFETIME:
            params.lifetime = strtol(optarg, &end, 10);

            if (end == optarg)
                DIEF("invalid lifetime '%s'", optarg);
        break;

        case ':':
            for (option = OPTIONS; option; option += 1)
                if (option->val == optopt)
                    DIEF("option '%s' requires an argument", option->name);

            DIEF("option %d requires an argument", optopt);
        break;

        case '?':
            fprintf(stderr, "unkown option '%d'\n", optopt);
        break;
        }
    }

    if (!bundle_params_verify(&params))
        DIEF("%s", "missing bundle parameters");

    eid_map_t *eid_map;
    eid_map_init(&eid_map);

    strbuf_t *eid_buf;
    strbuf_init(&eid_buf, 1 << 10);

#define SDNV_ENCODE(x) sdnv_encode((uint8_t *)(&x), sizeof(x));

    sdnv_t *version = SDNV_ENCODE(params.version);
    sdnv_t *flags = SDNV_ENCODE(params.flags);

    sdnv_t *dest, *dest_ssp;
    sdnv_t *src, *src_ssp;
    sdnv_t *report_to, *report_to_ssp;
    sdnv_t *custodian, *custodian_ssp;

    size_t *pos;

#define ADD(scheme, sdnv_buf) do { \
    pos = eid_map_lookup(eid_map, scheme); \
    if (pos) { \
        sdnv_buf = SDNV_ENCODE(*pos); \
    } else { \
        pos = eid_map_add(&eid_map, scheme); \
        *pos = eid_buf->pos; \
        sdnv_buf = SDNV_ENCODE(eid_buf->pos); \
        strbuf_append(&eid_buf, scheme, strlen(scheme)); \
        strbuf_finish(&eid_buf); \
    } \
} while (0)

    ADD(params.dest, dest);
    ADD(params.dest_ssp, dest_ssp);
    ADD(params.src, src);
    ADD(params.src_ssp, src_ssp);
    ADD(params.report_to, report_to);
    ADD(params.report_to_ssp, report_to_ssp);
    ADD(params.custodian, custodian);
    ADD(params.custodian_ssp, custodian_ssp);

#undef ADD

    sdnv_t *creation = SDNV_ENCODE(params.creation_ts);
    sdnv_t *creation_seq = SDNV_ENCODE(params.creation_ts_seq);
    sdnv_t *lifetime = SDNV_ENCODE(params.lifetime);
    sdnv_t *dict_len = SDNV_ENCODE(eid_buf->pos);

    uint32_t block_length;

    block_length =
        dest->len + dest_ssp->len +
        src->len + src_ssp->len +
        report_to->len + report_to_ssp->len +
        custodian->len + custodian_ssp->len +
        creation->len + creation_seq->len +
        lifetime->len + dict_len->len;

    sdnv_t *block_length_sdnv = SDNV_ENCODE(block_length);

#undef SDNV_ENCODE

    strbuf_t *bundle_buf;
    strbuf_init(&bundle_buf, 1 << 10);

#define APPEND(sdnv) strbuf_append(&bundle_buf, sdnv->bytes, sdnv->len)

    APPEND(version);
    APPEND(flags);
    APPEND(block_length_sdnv);
    APPEND(dest);
    APPEND(dest_ssp);
    APPEND(src);
    APPEND(src_ssp);
    APPEND(report_to);
    APPEND(report_to_ssp);
    APPEND(custodian);
    APPEND(custodian_ssp);
    APPEND(creation);
    APPEND(creation_seq);
    APPEND(lifetime);
    APPEND(dict_len);
    strbuf_append(&bundle_buf, eid_buf->buf, eid_buf->pos);

    if (params.flags & FLAG_IS_FRAGMENT) {
    }

#undef APPEND

    strbuf_destroy(bundle_buf);

    sdnv_destroy(version);
    sdnv_destroy(flags);

    sdnv_destroy(dest);
    sdnv_destroy(dest_ssp);
    sdnv_destroy(src);
    sdnv_destroy(src_ssp);
    sdnv_destroy(report_to);
    sdnv_destroy(report_to_ssp);
    sdnv_destroy(custodian);
    sdnv_destroy(custodian_ssp);

    sdnv_destroy(creation);
    sdnv_destroy(creation_seq);

    eid_map_destroy(eid_map);
    strbuf_destroy(eid_buf);
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
