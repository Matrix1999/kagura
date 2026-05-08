/*
 * anti_debug.c - Runtime anti-debug helpers for kagura
 *
 * These are linked into the target application alongside the obfuscated code.
 * The LLVM pass (AntiDebug.cpp) generates calls to these functions inside
 * a module constructor that runs before any user code.
 *
 * Targets: iOS (Darwin) and Android (Linux)
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// ---- Default tamper response ----

__attribute__((weak)) void kagura_on_tamper_detected(void) {
    abort();
}

// ---- TracerPid check (Android / Linux) ----

#ifdef __linux__
#include <stdio.h>

int kagura_check_tracer_pid(void) {
    FILE *f = fopen("/proc/self/status", "r");
    if (!f) return 0;

    char line[128];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "TracerPid:", 10) == 0) {
            int pid = atoi(line + 10);
            fclose(f);
            return pid != 0 ? 1 : 0;
        }
    }
    fclose(f);
    return 0;
}

// Check /proc/self/maps for known analysis framework strings
int kagura_check_maps(void) {
    static const char *Patterns[] = {
        "frida-agent",
        "frida-gadget",
        "libsubstrate",
        "libcycript",
        "xposed",
        NULL
    };

    FILE *f = fopen("/proc/self/maps", "r");
    if (!f) return 0;

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        for (int i = 0; Patterns[i]; ++i) {
            if (strstr(line, Patterns[i])) {
                fclose(f);
                return 1;
            }
        }
    }
    fclose(f);
    return 0;
}

// Check if Frida's default port (27042) is open on localhost
int kagura_check_frida_port(void) {
#include <sys/socket.h>
#include <netinet/in.h>
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return 0;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_port        = __builtin_bswap16(27042);
    addr.sin_addr.s_addr = 0x0100007F; // 127.0.0.1

    int result = connect(sock, (struct sockaddr *)&addr, sizeof(addr));
    close(sock);
    return result == 0 ? 1 : 0; // connected = Frida is running
}

#endif // __linux__

// ---- iOS / macOS checks ----

#ifdef __APPLE__
#include <dlfcn.h>

// Scan loaded dylibs for known injection frameworks
int kagura_check_loaded_dylibs(void) {
    // Use weak-linked dyld APIs to avoid hard dependency
    extern int _dyld_image_count(void) __attribute__((weak_import));
    extern const char *_dyld_get_image_name(unsigned) __attribute__((weak_import));

    static const char *Patterns[] = {
        "FridaGadget",
        "frida-gadget",
        "cynject",
        "libsubstrate",
        "cycript",
        "libhooker",
        NULL
    };

    if (!_dyld_image_count || !_dyld_get_image_name)
        return 0;

    int count = _dyld_image_count();
    for (int i = 0; i < count; ++i) {
        const char *name = _dyld_get_image_name((unsigned)i);
        if (!name) continue;
        for (int j = 0; Patterns[j]; ++j) {
            if (strstr(name, Patterns[j]))
                return 1;
        }
    }
    return 0;
}

#endif // __APPLE__
