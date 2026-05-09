/*
 * pointer_test.c — Test that memory/pointer operations survive obfuscation.
 *
 * Exercises pointer arithmetic, struct field access, and a small singly-linked
 * list (insert, find, count, sum).  All allocation is from a fixed-size arena
 * on the stack so that no dynamic memory allocation is required and the test
 * stays self-contained and deterministic.
 *
 * Intended to be compiled with all passes except -kagura-vm (which does not
 * support pointer-heavy functions) to verify that passes like -kagura-fla,
 * -kagura-bcf, -kagura-sub, -kagura-ibr, and -kagura-lt do not corrupt pointer
 * semantics.
 */

#include <stdio.h>
#include <stddef.h>

/* ─── Linked-list node ───────────────────────────────────────────────────── */

typedef struct Node {
    int value;
    struct Node *next;
} Node;

/* Simple arena: a fixed pool of nodes avoids malloc. */
#define ARENA_SIZE 32
static Node arena[ARENA_SIZE];
static int  arena_top = 0;

static Node *arena_alloc(void) {
    if (arena_top >= ARENA_SIZE)
        return 0;
    Node *n = &arena[arena_top++];
    n->value = 0;
    n->next  = 0;
    return n;
}

/* ─── List operations ────────────────────────────────────────────────────── */

/* Prepend a new node with `value` to the list headed by *head. */
static void list_insert(Node **head, int value) {
    Node *n = arena_alloc();
    if (!n) return;
    n->value = value;
    n->next  = *head;
    *head    = n;
}

/* Return the number of nodes in the list. */
static int list_count(const Node *head) {
    int c = 0;
    for (const Node *n = head; n != 0; n = n->next)
        ++c;
    return c;
}

/* Return the sum of all node values. */
static int list_sum(const Node *head) {
    int s = 0;
    for (const Node *n = head; n != 0; n = n->next)
        s += n->value;
    return s;
}

/* Return pointer to the first node whose value equals `target`, else NULL. */
static const Node *list_find(const Node *head, int target) {
    for (const Node *n = head; n != 0; n = n->next)
        if (n->value == target)
            return n;
    return 0;
}

/* Reverse the list in place; return the new head. */
static Node *list_reverse(Node *head) {
    Node *prev = 0;
    Node *cur  = head;
    while (cur != 0) {
        Node *nxt = cur->next;
        cur->next = prev;
        prev      = cur;
        cur       = nxt;
    }
    return prev;
}

/* ─── Struct + pointer arithmetic helpers ────────────────────────────────── */

typedef struct {
    int x;
    int y;
    int z;
} Vec3;

static Vec3 vec3_add(Vec3 a, Vec3 b) {
    Vec3 r;
    r.x = a.x + b.x;
    r.y = a.y + b.y;
    r.z = a.z + b.z;
    return r;
}

static int vec3_dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

/* Swap two ints via pointer. */
static void swap_int(int *a, int *b) {
    int t = *a;
    *a = *b;
    *b = t;
}

/* In-place selection sort of arr[0..len). */
static void selection_sort(int *arr, int len) {
    for (int i = 0; i < len - 1; ++i) {
        int min_idx = i;
        for (int j = i + 1; j < len; ++j)
            if (arr[j] < arr[min_idx])
                min_idx = j;
        swap_int(&arr[i], &arr[min_idx]);
    }
}

/* ─── main ────────────────────────────────────────────────────────────────── */

int main(void) {
    /* Build a linked list: 5 → 4 → 3 → 2 → 1 (prepend order) */
    Node *head = 0;
    for (int i = 1; i <= 5; ++i)
        list_insert(&head, i);

    printf("count    = %d\n", list_count(head));   /* 5  */
    printf("sum      = %d\n", list_sum(head));     /* 15 */

    const Node *found = list_find(head, 3);
    printf("find(3)  = %d\n", found ? found->value : -1);  /* 3 */
    printf("find(9)  = %d\n", list_find(head, 9) ? 1 : 0); /* 0 */

    /* After prepend order 1..5 the head value is 5. */
    printf("head val = %d\n", head->value);                 /* 5 */

    head = list_reverse(head);
    printf("rev head = %d\n", head->value);                 /* 1 */
    printf("sum chk  = %d\n", list_sum(head));             /* 15 (unchanged) */

    /* Vec3 */
    Vec3 a = {1, 2, 3};
    Vec3 b = {4, 5, 6};
    Vec3 c = vec3_add(a, b);
    printf("add.x    = %d\n", c.x);                        /* 5  */
    printf("add.y    = %d\n", c.y);                        /* 7  */
    printf("add.z    = %d\n", c.z);                        /* 9  */
    printf("dot      = %d\n", vec3_dot(a, b));             /* 32 */

    /* Selection sort */
    int arr[] = {64, 25, 12, 22, 11};
    selection_sort(arr, 5);
    printf("sort[0]  = %d\n", arr[0]);                     /* 11 */
    printf("sort[4]  = %d\n", arr[4]);                     /* 64 */

    /* Pointer swap */
    int x = 42, y = 7;
    swap_int(&x, &y);
    printf("swap x   = %d\n", x);                          /* 7  */
    printf("swap y   = %d\n", y);                          /* 42 */

    return 0;
}
