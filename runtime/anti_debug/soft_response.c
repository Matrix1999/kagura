/*===-- runtime/soft_response.c - Delayed/soft anti-cheat response --------===
 *
 * Delayed and graduated response to detected tampering.
 *
 * An immediate crash or ban on first detection:
 *   1. Teaches the attacker what triggers the protection.
 *   2. Creates false positives if the detection fires on legitimate devices.
 *
 * A soft response instead:
 *   1. Accumulates suspicion score over time.
 *   2. Applies graduated responses: ignore → degrade → penalise → disconnect.
 *   3. Introduces a random delay (jitter) before responding so that timing
 *      correlation attacks can't identify the trigger.
 *
 * Thresholds and response levels can be configured per application.
 *
 * Public API
 * ----------
 *   void kagura_soft_response_add(int score);   // add to suspicion score
 *   int  kagura_soft_response_level(void);      // 0=ok 1=warn 2=penalise 3=kick
 *   void kagura_soft_response_check(
 *           void (*respond)(int level, void *ctx), void *ctx);
 *   void kagura_soft_response_reset(void);
 *
 *===----------------------------------------------------------------------===*/

#include <stdint.h>
#include <stdlib.h>
#include <time.h>

/* Soft response levels */
#define KAGURA_RESPONSE_OK        0
#define KAGURA_RESPONSE_WARN      1
#define KAGURA_RESPONSE_PENALISE  2
#define KAGURA_RESPONSE_KICK      3

/* Score thresholds for each response level */
#ifndef KAGURA_THRESHOLD_WARN
#define KAGURA_THRESHOLD_WARN     10
#endif
#ifndef KAGURA_THRESHOLD_PENALISE
#define KAGURA_THRESHOLD_PENALISE 30
#endif
#ifndef KAGURA_THRESHOLD_KICK
#define KAGURA_THRESHOLD_KICK     60
#endif

static int kSuspicionScore = 0;
static int kJitterEnabled  = 1;

/* Add suspicion score, optionally with a randomised timing jitter */
void kagura_soft_response_add(int score) {
    kSuspicionScore += score;

    if (kJitterEnabled) {
        /* Introduce a small random delay (0-50ms via busy-wait on clock)
         * to prevent timing side-channel correlation. */
        struct timespec start, now;
        clock_gettime(CLOCK_MONOTONIC, &start);
        long target_ns = (long)(rand() % 50) * 1000000L; /* 0-49 ms */
        do {
            clock_gettime(CLOCK_MONOTONIC, &now);
        } while ((now.tv_sec - start.tv_sec) * 1000000000L +
                 (now.tv_nsec - start.tv_nsec) < target_ns);
    }
}

int kagura_soft_response_level(void) {
    if (kSuspicionScore >= KAGURA_THRESHOLD_KICK)     return KAGURA_RESPONSE_KICK;
    if (kSuspicionScore >= KAGURA_THRESHOLD_PENALISE) return KAGURA_RESPONSE_PENALISE;
    if (kSuspicionScore >= KAGURA_THRESHOLD_WARN)     return KAGURA_RESPONSE_WARN;
    return KAGURA_RESPONSE_OK;
}

void kagura_soft_response_check(
        void (*respond)(int level, void *ctx), void *ctx) {
    int level = kagura_soft_response_level();
    if (level > KAGURA_RESPONSE_OK && respond)
        respond(level, ctx);
}

void kagura_soft_response_reset(void) {
    kSuspicionScore = 0;
}
