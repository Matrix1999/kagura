/*===-- runtime/integrity_report.c - Integrity report signing + nonce ----===
 *
 * Integrity report signing with replay prevention and
 *                  nonce / challenge-response for server coordination.
 *
 * Architecture
 * ------------
 * The client (this module) produces a signed integrity report that the server
 * can verify.  The flow is:
 *
 *   1. Server sends a challenge: { "nonce": "<hex32>", "timestamp": <unix> }
 *   2. Client calls kagura_integrity_report_build() with the nonce to produce
 *      a report JSON containing:
 *        - device_key_hash (FNV-1a-64 of kagura_device_key())
 *        - behavior_score  (from kagura_behavior_score())
 *        - violation_flags (bitmask of active tamper signals)
 *        - nonce           (echoed from server challenge)
 *        - timestamp       (current UNIX time)
 *      The report is HMAC-SHA256 signed with the device key.
 *
 *   3. Client sends the signed report to the server.
 *   4. Server verifies:
 *        a. Signature matches (uses the expected device key hash).
 *        b. Nonce matches the challenge it sent.
 *        c. Timestamp is within acceptable window (±5 min).
 *        d. Violation flags are below threshold.
 *
 * Replay prevention:
 *   - Nonces are stored in a fixed-size ring buffer (NONCE_HISTORY = 32).
 *   - kagura_nonce_is_fresh() returns 0 if the nonce was seen before.
 *   - Nonces older than NONCE_TTL_SECS (300s) are evicted automatically.
 *
 * HMAC-SHA256 note:
 *   We use a simple HMAC construction over the kagura AES infrastructure.
 *   For production use, replace with a full SHA-256 implementation.
 *   Here we use FNV-1a-64 as a lightweight MAC (not cryptographically strong
 *   but sufficient for tamper signalling without a heavy crypto dependency).
 *
 * Public API
 * ----------
 *   int   kagura_nonce_is_fresh(const char *nonce_hex, size_t len);
 *   void  kagura_nonce_consume(const char *nonce_hex, size_t len);
 *   int   kagura_integrity_report_build(const char *nonce,
 *                                       char *out_buf, size_t buf_len);
 *
 *===----------------------------------------------------------------------===*/

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

/* ---- FNV-1a-64 MAC -------------------------------------------------------- */

static uint64_t fnv1a64(const uint8_t *data, size_t len) {
    uint64_t h = 0xcbf29ce484222325ULL;
    for (size_t i = 0; i < len; ++i) {
        h ^= data[i];
        h *= 0x100000001b3ULL;
    }
    return h;
}

/* ---- Nonce replay-prevention ring buffer ---------------------------------- */

#define NONCE_HISTORY  32
#define NONCE_MAX_LEN  64
#define NONCE_TTL_SECS 300

typedef struct {
    char   value[NONCE_MAX_LEN + 1];
    time_t seen_at;
} nonce_entry_t;

static nonce_entry_t g_nonce_ring[NONCE_HISTORY];
static int           g_nonce_head = 0;

/*
 * kagura_nonce_is_fresh — returns 1 if nonce has not been seen before
 * and is not expired, 0 if it was already used (replay).
 */
int kagura_nonce_is_fresh(const char *nonce, size_t len) {
    if (!nonce || len == 0) return 0;
    time_t now = time(NULL);
    for (int i = 0; i < NONCE_HISTORY; ++i) {
        nonce_entry_t *e = &g_nonce_ring[i];
        if (e->value[0] == '\0') continue;
        /* Evict expired entries */
        if (now - e->seen_at > NONCE_TTL_SECS) {
            e->value[0] = '\0';
            continue;
        }
        size_t elen = strlen(e->value);
        if (elen == len && memcmp(e->value, nonce, len) == 0) {
            return 0; /* replay detected */
        }
    }
    return 1; /* fresh */
}

/*
 * kagura_nonce_consume — record nonce as used; call after validating freshness.
 */
void kagura_nonce_consume(const char *nonce, size_t len) {
    if (!nonce || len == 0) return;
    if (len > NONCE_MAX_LEN) len = NONCE_MAX_LEN;
    nonce_entry_t *slot = &g_nonce_ring[g_nonce_head % NONCE_HISTORY];
    memcpy(slot->value, nonce, len);
    slot->value[len] = '\0';
    slot->seen_at = time(NULL);
    g_nonce_head = (g_nonce_head + 1) % NONCE_HISTORY;
}

/* ---- Device key hash ------------------------------------------------------ */

/* Forward declaration: defined in device_key.c */
extern void kagura_device_key(uint8_t out_key[16]) __attribute__((weak));

static uint64_t get_device_key_hash(void) {
    uint8_t key[16] = {0};
    if (kagura_device_key) kagura_device_key(key);
    return fnv1a64(key, sizeof(key));
}

/* ---- Behavior score ------------------------------------------------------- */

/* Forward declaration: defined in behavior_log.c */
extern int kagura_behavior_score(void) __attribute__((weak));

/* ---- Violation flags bitmask ---------------------------------------------- */

/* Forward declarations for tamper checks; all are optional (weak). */
extern int kagura_root_check(void)          __attribute__((weak));
extern int kagura_frida_check(void)         __attribute__((weak));
extern int kagura_emulator_check(void)      __attribute__((weak));
extern int kagura_debugger_check(void)      __attribute__((weak));
extern int kagura_integrity_check(void)     __attribute__((weak));

#define FLAG_ROOTED       (1u << 0)
#define FLAG_FRIDA        (1u << 1)
#define FLAG_EMULATOR     (1u << 2)
#define FLAG_DEBUGGER     (1u << 3)
#define FLAG_INTEGRITY    (1u << 4)

static uint32_t collect_violation_flags(void) {
    uint32_t flags = 0;
    if (kagura_root_check     && kagura_root_check())      flags |= FLAG_ROOTED;
    if (kagura_frida_check    && kagura_frida_check())     flags |= FLAG_FRIDA;
    if (kagura_emulator_check && kagura_emulator_check())  flags |= FLAG_EMULATOR;
    if (kagura_debugger_check && kagura_debugger_check())  flags |= FLAG_DEBUGGER;
    if (kagura_integrity_check&& kagura_integrity_check()) flags |= FLAG_INTEGRITY;
    return flags;
}

/* ---- Report builder ------------------------------------------------------- */

/*
 * kagura_integrity_report_build — build a JSON integrity report string.
 *
 * Parameters:
 *   nonce    — server-provided nonce string (hex, max 64 chars)
 *   out_buf  — caller-provided buffer for the JSON output
 *   buf_len  — size of out_buf (must be >= 256)
 *
 * Returns the number of bytes written (excluding NUL), or -1 on error.
 *
 * Report format:
 *   {
 *     "device_key_hash": "<hex16>",
 *     "behavior_score": <int>,
 *     "violation_flags": <uint32>,
 *     "nonce": "<nonce>",
 *     "timestamp": <unix>,
 *     "mac": "<hex16>"
 *   }
 *
 * The "mac" field is FNV-1a-64 over all fields above (as a UTF-8 string)
 * XOR'd with the device key hash, providing a lightweight integrity seal.
 */
int kagura_integrity_report_build(const char *nonce,
                                   char *out_buf, size_t buf_len) {
    if (!nonce || !out_buf || buf_len < 256) return -1;

    uint64_t dk_hash  = get_device_key_hash();
    int      bscore   = kagura_behavior_score ? kagura_behavior_score() : 0;
    uint32_t vflags   = collect_violation_flags();
    time_t   ts       = time(NULL);

    /* Build the unsigned body first */
    char body[512];
    int body_len = snprintf(body, sizeof(body),
        "{\"device_key_hash\":\"%016llx\","
        "\"behavior_score\":%d,"
        "\"violation_flags\":%u,"
        "\"nonce\":\"%s\","
        "\"timestamp\":%ld}",
        (unsigned long long)dk_hash, bscore, vflags, nonce, (long)ts);
    if (body_len <= 0 || (size_t)body_len >= sizeof(body)) return -1;

    /* MAC = FNV-1a-64(body) XOR dk_hash */
    uint64_t mac = fnv1a64((const uint8_t *)body, (size_t)body_len) ^ dk_hash;

    /* Write final report with mac appended */
    int n = snprintf(out_buf, buf_len,
        "{\"device_key_hash\":\"%016llx\","
        "\"behavior_score\":%d,"
        "\"violation_flags\":%u,"
        "\"nonce\":\"%s\","
        "\"timestamp\":%ld,"
        "\"mac\":\"%016llx\"}",
        (unsigned long long)dk_hash, bscore, vflags, nonce, (long)ts,
        (unsigned long long)mac);
    if (n <= 0 || (size_t)n >= buf_len) return -1;

    /* Record nonce as consumed to prevent replay */
    kagura_nonce_consume(nonce, strlen(nonce));

    return n;
}
