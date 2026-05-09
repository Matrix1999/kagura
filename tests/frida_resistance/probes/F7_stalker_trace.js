/**
 * F7: Stalker trace to evaluate CFG complexity after FLA.
 *
 * A non-obfuscated function with N basic blocks generates a predictable
 * execution trace.  After FLA, all blocks share a single switch dispatcher,
 * making the trace much longer and non-trivially analysable.
 *
 * Metric: unique block addresses visited in the first 10,000 instructions.
 * More unique addresses = higher attacker analysis cost.
 *
 * Verdict:
 *   RESISTANT  — >= 50 unique blocks (FLA significantly increased complexity)
 *   PARTIAL    — 10-49 unique blocks
 *   VULNERABLE — < 10 unique blocks (FLA had little effect)
 */

'use strict';

const mainThread = Process.enumerateThreadsSync()[0];
const uniqueBlocks = new Set();
let instrCount = 0;
const MAX_INSTRS = 10000;

Stalker.follow(mainThread.id, {
    events: {
        block: true,
    },
    onReceive(events) {
        const reader = Stalker.parse(events, { annotate: false, stringify: false });
        for (const ev of reader) {
            if (instrCount >= MAX_INSTRS) break;
            uniqueBlocks.add(ev[0].toString()); // block start address
            instrCount++;
        }
        if (instrCount >= MAX_INSTRS) {
            Stalker.unfollow(mainThread.id);
            report();
        }
    }
});

function report() {
    const count = uniqueBlocks.size;
    let verdict;
    if (count >= 50)     verdict = 'RESISTANT';
    else if (count >= 10) verdict = 'PARTIAL';
    else                  verdict = 'VULNERABLE';
    console.log(`[F7] Unique blocks in ${instrCount} instrs: ${count} → ${verdict}`);
    send({ probe: 'F7', verdict, unique_blocks: count, instrs_traced: instrCount });
}

// Fallback: report after 5s even if we didn't hit MAX_INSTRS
setTimeout(() => {
    Stalker.unfollow(mainThread.id);
    report();
}, 5000);
