/**
 * F1: Try to intercept string decryption stubs.
 *
 * kagura injects a unique stub per encrypted string.  Without FLA, the stub
 * address is predictable.  With FLA the CFG is opaque.  This probe tries
 * to find and hook every function with "decrypt" in its name or with the
 * kagura stub signature pattern (short function, single XOR loop).
 *
 * Verdict:
 *   RESISTANT — no hooks fired on string access
 *   PARTIAL   — some hooks fired but only on internal kagura stubs, not data
 *   VULNERABLE — hooks fired and plaintext strings were captured
 */

'use strict';

const results = [];

const mainModule = Process.enumerateModulesSync()[0];
const exports = mainModule.enumerateExports();

// Hook any exported symbol whose name suggests a decrypt stub
const targets = exports.filter(e =>
    e.name.toLowerCase().includes('decrypt') ||
    e.name.toLowerCase().includes('deobf') ||
    e.name.startsWith('kagura_') // kagura runtime functions
);

console.log(`[F1] Found ${targets.length} potential decrypt targets`);

let hooksHit = 0;
let stringsCapture = [];

targets.forEach(target => {
    try {
        Interceptor.attach(target.address, {
            onEnter(args) {
                hooksHit++;
                // Try to read the first arg as a string pointer
                try {
                    const s = args[0].readUtf8String(128);
                    if (s && s.length > 3) stringsCapture.push(s);
                } catch(e) { /* not a string */ }
            }
        });
    } catch(e) {
        // Function too short to hook or not executable
    }
});

// Wait 2 seconds for hooks to fire
setTimeout(() => {
    let verdict;
    if (stringsCapture.length > 0) {
        verdict = 'VULNERABLE';
        console.log(`[F1] Captured strings: ${JSON.stringify(stringsCapture.slice(0, 5))}`);
    } else if (hooksHit > 0) {
        verdict = 'PARTIAL';
    } else {
        verdict = 'RESISTANT';
    }
    console.log(`[F1] Verdict: ${verdict} (hooks_hit=${hooksHit}, strings_captured=${stringsCapture.length})`);
    send({ probe: 'F1', verdict, hooks_hit: hooksHit, strings_captured: stringsCapture.length });
}, 2000);
