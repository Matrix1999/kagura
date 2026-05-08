/*
 * vm_test.c — Test input for the -kagura-vm (VM obfuscation) pass.
 *
 * Functions annotated __attribute__((annotate("kagura_vm"))) are selected for
 * virtualization.  They must be pure integer functions with no external calls
 * (printf, malloc, etc.) because the VM pass skips functions that call
 * declarations not defined in this translation unit.
 *
 * main() is intentionally left unannotated; it calls printf and exercises
 * every virtualized helper.
 */

#include <stdio.h>

/* ─── Basic arithmetic ────────────────────────────────────────────────────── */

__attribute__((annotate("kagura_vm")))
int vm_add(int a, int b) {
    return a + b;
}

__attribute__((annotate("kagura_vm")))
int vm_sub(int a, int b) {
    return a - b;
}

__attribute__((annotate("kagura_vm")))
int vm_mul(int a, int b) {
    return a * b;
}

/*
 * Integer division.  The pass skips FP, so we stay with int.
 * Guard against divide-by-zero to keep the test deterministic.
 */
__attribute__((annotate("kagura_vm")))
int vm_div(int a, int b) {
    if (b == 0)
        return 0;
    return a / b;
}

__attribute__((annotate("kagura_vm")))
int vm_mod(int a, int b) {
    if (b == 0)
        return 0;
    return a % b;
}

/* ─── Factorial (recursive — tests conditional branches + self-call) ──────── */

/*
 * Note: recursive direct calls to a function defined in the same TU are
 * allowed by canVirtualize(); all other external-declaration calls are not.
 */
__attribute__((annotate("kagura_vm")))
int vm_factorial(int n) {
    if (n <= 1)
        return 1;
    return n * vm_factorial(n - 1);
}

/* ─── Fibonacci (iterative — tests loops + multiple local variables) ──────── */

__attribute__((annotate("kagura_vm")))
int vm_fibonacci(int n) {
    if (n <= 0) return 0;
    if (n == 1) return 1;
    int a = 0, b = 1;
    for (int i = 2; i <= n; ++i) {
        int c = a + b;
        a = b;
        b = c;
    }
    return b;
}

/* ─── Binary search (pure integer array passed by pointer) ───────────────── */

/*
 * Binary search on a sorted array of ints.
 * Returns the index of `target`, or -1 if not found.
 *
 * Pointer arguments are passed as integer-sized values; the VM represents
 * them as uint64_t and the trampoline ZExts them.  LoadInst on int32 arrays
 * is handled by the VM's OP_LOAD32 opcode.
 */
__attribute__((annotate("kagura_vm")))
int vm_bsearch(const int *arr, int len, int target) {
    int lo = 0, hi = len - 1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        int val = arr[mid];
        if (val == target)
            return mid;
        if (val < target)
            lo = mid + 1;
        else
            hi = mid - 1;
    }
    return -1;
}

/* ─── Simple state machine ────────────────────────────────────────────────── */

/*
 * Models a 3-state counter FSM:
 *   IDLE(0) → on event 1 → RUNNING(1)
 *   RUNNING(1) → on event 2 → DONE(2)
 *   DONE(2) → on event 0 → IDLE(0)   (reset)
 *   Any other event keeps the current state.
 *
 * Exercises a switch-like cascade of if/else branches.
 */
__attribute__((annotate("kagura_vm")))
int vm_fsm_step(int state, int event) {
    if (state == 0 && event == 1) return 1;
    if (state == 1 && event == 2) return 2;
    if (state == 2 && event == 0) return 0;
    return state;
}

/* Run `steps` events through the FSM and return the final state. */
__attribute__((annotate("kagura_vm")))
int vm_fsm_run(int initial, const int *events, int steps) {
    int state = initial;
    for (int i = 0; i < steps; ++i)
        state = vm_fsm_step(state, events[i]);
    return state;
}

/* ─── main ────────────────────────────────────────────────────────────────── */

int main(void) {
    /* Arithmetic */
    printf("add(7,3)   = %d\n", vm_add(7, 3));        /* 10 */
    printf("sub(10,4)  = %d\n", vm_sub(10, 4));       /* 6  */
    printf("mul(6,7)   = %d\n", vm_mul(6, 7));        /* 42 */
    printf("div(20,4)  = %d\n", vm_div(20, 4));       /* 5  */
    printf("mod(17,5)  = %d\n", vm_mod(17, 5));       /* 2  */
    printf("div(x,0)   = %d\n", vm_div(99, 0));       /* 0  (guard) */

    /* Factorial */
    printf("fact(0)    = %d\n", vm_factorial(0));     /* 1  */
    printf("fact(5)    = %d\n", vm_factorial(5));     /* 120 */
    printf("fact(10)   = %d\n", vm_factorial(10));    /* 3628800 */

    /* Fibonacci */
    printf("fib(0)     = %d\n", vm_fibonacci(0));     /* 0 */
    printf("fib(1)     = %d\n", vm_fibonacci(1));     /* 1 */
    printf("fib(10)    = %d\n", vm_fibonacci(10));    /* 55 */
    printf("fib(15)    = %d\n", vm_fibonacci(15));    /* 610 */

    /* Binary search */
    static const int sorted[] = {2, 5, 8, 12, 16, 23, 38, 56, 72, 91};
    int len = (int)(sizeof(sorted) / sizeof(sorted[0]));
    printf("bsearch(23)= %d\n", vm_bsearch(sorted, len, 23));  /* 5 */
    printf("bsearch(2) = %d\n", vm_bsearch(sorted, len, 2));   /* 0 */
    printf("bsearch(91)= %d\n", vm_bsearch(sorted, len, 91));  /* 9 */
    printf("bsearch(99)= %d\n", vm_bsearch(sorted, len, 99));  /* -1 */

    /* State machine */
    static const int events[] = {1, 2, 0, 1, 1, 2};
    printf("fsm_step(0,1) = %d\n", vm_fsm_step(0, 1));          /* 1 */
    printf("fsm_step(1,2) = %d\n", vm_fsm_step(1, 2));          /* 2 */
    printf("fsm_step(2,0) = %d\n", vm_fsm_step(2, 0));          /* 0 */
    printf("fsm_run(6ev)  = %d\n", vm_fsm_run(0, events, 6));   /* 2 */

    return 0;
}
