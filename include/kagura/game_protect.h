/*===-- kagura/game_protect.h - Game logic value protection template ------===
 *
 * Provides C++ template wrappers for protecting game-critical values
 * (HP, damage, currency, speed, etc.) from memory scanners and freeze tools
 * such as GameGuardian, Cheat Engine, and memory editors.
 *
 * Protection strategy:
 *   Each protected value is stored XOR-encrypted with a per-instance runtime
 *   key.  The plaintext value is never resident in memory — it is derived
 *   on demand and immediately discarded.  Reads and writes go through
 *   accessor methods that perform the XOR transparently.
 *
 * Additional hardening:
 *   - A shadow copy stores the value XOR'd with a different key.  On every
 *     read the two copies are compared; a mismatch triggers the tamper
 *     callback (external memory write detected).
 *   - The key itself is stored in a volatile field to prevent the compiler
 *     from caching it in a register and exposing the plain value in a tight
 *     loop.
 *
 * Usage:
 *
 *   #include "kagura/game_protect.h"
 *
 *   kagura::Protected<int> hp(100);      // HP = 100, stored encrypted
 *   kagura::Protected<float> speed(5.5f);
 *
 *   hp -= 30;           // operator overloads work naturally
 *   if (hp <= 0) die();
 *
 *   // Custom tamper callback (optional):
 *   kagura::Protected<int>::setTamperCallback([]{ abort(); });
 *
 * Supported types: any trivially-copyable numeric type (int, float, etc.)
 *
 * Platform:  C++17, header-only, no LLVM dependency at runtime.
 *
 *===----------------------------------------------------------------------===*/

#pragma once

#include <cstdint>
#include <cstring>
#include <functional>
#include <type_traits>

namespace kagura {

// ---- Runtime key source ----------------------------------------------------

/// Generates a simple runtime key from ASLR + time entropy.
/// Not cryptographic — intended to make the key different on every run so
/// memory scanners cannot predict the XOR pattern.
namespace detail {

inline uint64_t make_runtime_key() noexcept {
    // Combine a stack address (ASLR contribution) with the address of this
    // function (ASLR of the code segment).
    volatile uint64_t stack_probe = 0;
    uint64_t addr1 = reinterpret_cast<uint64_t>(&stack_probe);
    uint64_t addr2 = reinterpret_cast<uint64_t>(&make_runtime_key);
    // Mix via splitmix64 finaliser
    uint64_t z = (addr1 ^ addr2) + UINT64_C(0x9e3779b97f4a7c15);
    z = (z ^ (z >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    z = (z ^ (z >> 27)) * UINT64_C(0x94d049bb133111eb);
    return z ^ (z >> 31);
}

// Convert T <-> uint64_t without undefined behaviour.
template<typename T>
inline uint64_t to_u64(T v) noexcept {
    static_assert(sizeof(T) <= sizeof(uint64_t), "T too large");
    uint64_t u = 0;
    std::memcpy(&u, &v, sizeof(T));
    return u;
}

template<typename T>
inline T from_u64(uint64_t u) noexcept {
    T v;
    std::memcpy(&v, &u, sizeof(T));
    return v;
}

} // namespace detail

// ---- Tamper callback --------------------------------------------------------

namespace detail {
using TamperCb = std::function<void()>;
inline TamperCb &global_tamper_cb() {
    static TamperCb cb;
    return cb;
}
inline void on_tamper() {
    auto &cb = global_tamper_cb();
    if (cb) cb();
    // Default: spin to avoid giving attacker a clean crash point.
    for (;;) { volatile int x = 0; (void)x; }
}
} // namespace detail

// ---- Protected<T> template -------------------------------------------------

/// A numeric value stored XOR-encrypted with a runtime key.
/// Shadow copy enables external-write detection.
template<typename T>
class Protected {
    static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
    static_assert(sizeof(T) <= 8, "T must fit in 64 bits");

    volatile uint64_t key_;      // runtime XOR key
    volatile uint64_t primary_;  // T value XOR key
    volatile uint64_t shadow_;   // T value XOR (key ^ SHADOW_MASK)

    static constexpr uint64_t SHADOW_MASK = UINT64_C(0xDEADBEEFCAFEBABE);

    uint64_t encode_primary(T v) const noexcept {
        return detail::to_u64(v) ^ key_;
    }
    uint64_t encode_shadow(T v) const noexcept {
        return detail::to_u64(v) ^ (key_ ^ SHADOW_MASK);
    }

    T decode() const noexcept {
        uint64_t p = detail::to_u64(decode_primary());
        uint64_t s = detail::to_u64(decode_shadow());
        if (p != s)
            detail::on_tamper();
        return detail::from_u64<T>(p);
    }

    T decode_primary() const noexcept {
        return detail::from_u64<T>(primary_ ^ key_);
    }
    T decode_shadow() const noexcept {
        return detail::from_u64<T>(shadow_ ^ (key_ ^ SHADOW_MASK));
    }

public:
    explicit Protected(T init = T{}) noexcept {
        key_     = detail::make_runtime_key();
        primary_ = encode_primary(init);
        shadow_  = encode_shadow(init);
    }

    // Copy / assign
    Protected(const Protected &o) noexcept : Protected(o.get()) {}
    Protected &operator=(const Protected &o) noexcept {
        set(o.get()); return *this;
    }
    Protected &operator=(T v) noexcept { set(v); return *this; }

    T get() const noexcept { return decode(); }

    void set(T v) noexcept {
        primary_ = encode_primary(v);
        shadow_  = encode_shadow(v);
    }

    // Implicit conversion for seamless use in expressions
    operator T() const noexcept { return get(); }

    // Arithmetic operators
    Protected &operator+=(T rhs) noexcept { set(get() + rhs); return *this; }
    Protected &operator-=(T rhs) noexcept { set(get() - rhs); return *this; }
    Protected &operator*=(T rhs) noexcept { set(get() * rhs); return *this; }
    Protected &operator/=(T rhs) noexcept { set(get() / rhs); return *this; }
    Protected &operator%=(T rhs) noexcept { set(get() % rhs); return *this; }

    // Prefix increment/decrement
    Protected &operator++() noexcept { set(get() + T{1}); return *this; }
    Protected &operator--() noexcept { set(get() - T{1}); return *this; }
    // Postfix
    T operator++(int) noexcept { T v = get(); set(v + T{1}); return v; }
    T operator--(int) noexcept { T v = get(); set(v - T{1}); return v; }

    // Module-wide tamper callback (optional; default spins forever)
    static void setTamperCallback(detail::TamperCb cb) noexcept {
        detail::global_tamper_cb() = std::move(cb);
    }
};

// ---- Convenience type aliases ----------------------------------------------

using ProtectedInt    = Protected<int>;
using ProtectedUInt   = Protected<unsigned int>;
using ProtectedInt64  = Protected<int64_t>;
using ProtectedUInt64 = Protected<uint64_t>;
using ProtectedFloat  = Protected<float>;
using ProtectedDouble = Protected<double>;

} // namespace kagura
