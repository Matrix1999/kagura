/*===-- runtime/windows/tamper_response.c - Windows tamper response -------===
 *
 * A.1: Windows-specific tamper response callbacks.
 *
 * kagura_on_tamper_detected() is declared __attribute__((weak)) in
 * anti_debug.c so that application code can override it.  This file provides
 * a secondary Windows-specific entry point that performs a clean process
 * termination via ExitProcess() rather than abort() (which can show a
 * crash dialog on Windows).
 *
 * The soft-response score helpers follow the same API as the Android/iOS
 * implementations in soft_response.c.
 *
 *===----------------------------------------------------------------------===*/

#ifdef _WIN32

#include <stdint.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

/* ---- Soft-response score state ------------------------------------------ */

#define KAGURA_THRESHOLD_WARN      10
#define KAGURA_THRESHOLD_PENALISE  30
#define KAGURA_THRESHOLD_KICK      60

static volatile LONG g_score = 0;

void kagura_soft_response_add(int score) {
    InterlockedAdd(&g_score, (LONG)score);
}

int kagura_soft_response_level(void) {
    LONG s = InterlockedCompareExchange(&g_score, 0, 0); /* atomic read */
    if (s >= KAGURA_THRESHOLD_KICK)      return 3;
    if (s >= KAGURA_THRESHOLD_PENALISE)  return 2;
    if (s >= KAGURA_THRESHOLD_WARN)      return 1;
    return 0;
}

void kagura_soft_response_reset(void) {
    InterlockedExchange(&g_score, 0);
}

void kagura_soft_response_check(int threshold) {
    extern void kagura_on_tamper_detected(void);
    if (InterlockedCompareExchange(&g_score, 0, 0) >= (LONG)threshold)
        kagura_on_tamper_detected();
}

#endif /* _WIN32 */
