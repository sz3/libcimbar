// intx: extended precision integer library.
// Copyright 2019 Pawel Bylica.
// Licensed under the Apache License, Version 2.0.

#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <climits>
#include <compare>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdio>   // fputs
#include <cstdlib>  // abort
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>

#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable : 5030)  // Allow unknown attributes.
#endif

#ifndef __has_builtin
    #define __has_builtin(NAME) 0
#endif

#ifndef __has_feature
    #define __has_feature(NAME) 0
#endif

#ifdef _MSC_VER
    #include <intrin.h>
#endif

#if __has_builtin(__builtin_expect)
    #define INTX_UNLIKELY(EXPR) __builtin_expect(bool{EXPR}, false)
#else
    #define INTX_UNLIKELY(EXPR) (bool{EXPR})
#endif

#ifndef NDEBUG
    #define INTX_REQUIRE assert
#else
    #define INTX_REQUIRE(X) (X) ? (void)0 : intx::unreachable()
#endif


// Detect compiler support for 128-bit integer __int128
#ifdef __SIZEOF_INT128__
    #define INTX_HAS_BUILTIN_INT128 1
#else
    #define INTX_HAS_BUILTIN_INT128 0
#endif

namespace intx
{
/// Mark a possible code path as unreachable (invokes undefined behavior).
/// TODO(C++23): Use std::unreachable().
[[noreturn]] inline void unreachable() noexcept
{
#if __has_builtin(__builtin_unreachable)
    __builtin_unreachable();
#elif defined(_MSC_VER)
    __assume(false);
#endif
}

#if INTX_HAS_BUILTIN_INT128
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpedantic"  // Usage of __int128 triggers a pedantic warning.

/// Alias for the compiler supported unsigned __int128 type.
using builtin_uint128 = unsigned __int128;

    #pragma GCC diagnostic pop
#endif


template <unsigned N>
struct uint;

/// Contains result of add/sub/etc with a carry flag.
template <typename T>
struct result_with_carry
{
    T value;
    bool carry;

    /// Conversion to tuple of references, to allow usage with std::tie().
    constexpr explicit(false) operator std::tuple<T&, bool&>() noexcept { return {value, carry}; }
};

template <typename QuotT, typename RemT = QuotT>
struct div_result
{
    QuotT quot;
    RemT rem;

    bool operator==(const div_result&) const = default;

    /// Conversion to tuple of references, to allow usage with std::tie().
    constexpr explicit(false) operator std::tuple<QuotT&, RemT&>() noexcept { return {quot, rem}; }
};

/// Addition with carry.
constexpr result_with_carry<uint64_t> addc(uint64_t x, uint64_t y, bool carry = false) noexcept
{
#if __has_builtin(__builtin_addcll)
    if (!std::is_constant_evaluated())
    {
        unsigned long long carryout = 0;  // NOLINT(google-runtime-int)
        const auto s = __builtin_addcll(x, y, carry, &carryout);
        static_assert(sizeof(s) == sizeof(uint64_t));
        return {s, static_cast<bool>(carryout)};
    }
#elif __has_builtin(__builtin_ia32_addcarryx_u64)
    if (!std::is_constant_evaluated())
    {
        unsigned long long s = 0;  // NOLINT(google-runtime-int)
        static_assert(sizeof(s) == sizeof(uint64_t));
        const auto carryout = __builtin_ia32_addcarryx_u64(carry, x, y, &s);
        return {s, static_cast<bool>(carryout)};
    }
#endif

    const auto s = x + y;
    const auto carry1 = s < x;
    const auto t = s + carry;
    const auto carry2 = t < s;
    return {t, carry1 || carry2};
}

/// Subtraction with carry (borrow).
constexpr result_with_carry<uint64_t> subc(uint64_t x, uint64_t y, bool carry = false) noexcept
{
// Use __builtin_subcll if available (except buggy Xcode 14.3.1 on arm64).
#if __has_builtin(__builtin_subcll) && __apple_build_version__ != 14030022
    if (!std::is_constant_evaluated())
    {
        unsigned long long carryout = 0;  // NOLINT(google-runtime-int)
        const auto d = __builtin_subcll(x, y, carry, &carryout);
        static_assert(sizeof(d) == sizeof(uint64_t));
        return {d, static_cast<bool>(carryout)};
    }
#elif __has_builtin(__builtin_ia32_sbb_u64)
    if (!std::is_constant_evaluated())
    {
        unsigned long long d = 0;  // NOLINT(google-runtime-int)
        static_assert(sizeof(d) == sizeof(uint64_t));
        const auto carryout = __builtin_ia32_sbb_u64(carry, x, y, &d);
        return {d, static_cast<bool>(carryout)};
    }
#endif

    const auto d = x - y;
    const auto carry1 = x < y;
    const auto e = d - carry;
    const auto carry2 = d < uint64_t{carry};
    return {e, carry1 || carry2};
}

/// Addition with carry.
template <unsigned N>
constexpr result_with_carry<uint<N>> addc(
    const uint<N>& x, const uint<N>& y, bool carry = false) noexcept
{
    uint<N> s;
    bool k = carry;
    for (size_t i = 0; i < uint<N>::num_words; ++i)
    {
        auto t = addc(x[i], y[i], k);
        s[i] = t.value;
        k = t.carry;
    }
    return {s, k};
}

/// Performs subtraction of two unsigned numbers and returns the difference
/// and the carry bit (aka borrow, overflow).
template <unsigned N>
constexpr result_with_carry<uint<N>> subc(
    const uint<N>& x, const uint<N>& y, bool carry = false) noexcept
{
    uint<N> z;
    bool k = carry;
    for (size_t i = 0; i < uint<N>::num_words; ++i)
    {
        auto t = subc(x[i], y[i], k);
        z[i] = t.value;
        k = t.carry;
    }
    return {z, k};
}

constexpr uint<128> umul(uint64_t x, uint64_t y) noexcept;


/// The 128-bit unsigned integer.
///
/// This type is defined as a specialization of uint<> to easier integration with full intx package,
/// however, uint128 may be used independently.
template <>
struct uint<128>
{
    using word_type = uint64_t;
    static constexpr auto word_num_bits = sizeof(word_type) * 8;
    static constexpr unsigned num_bits = 128;
    static constexpr auto num_words = num_bits / word_num_bits;

private:
    uint64_t words_[2]{};

public:
    constexpr uint() noexcept = default;

    constexpr uint(uint64_t low, uint64_t high) noexcept : words_{low, high} {}

    template <typename T>
    constexpr explicit(false) uint(T x) noexcept
        requires std::is_convertible_v<T, uint64_t>
      : words_{static_cast<uint64_t>(x), 0}
    {}

#if INTX_HAS_BUILTIN_INT128
    constexpr explicit(false) uint(builtin_uint128 x) noexcept
      : words_{uint64_t(x), uint64_t(x >> 64)}
    {}

    constexpr explicit operator builtin_uint128() const noexcept
    {
        return (builtin_uint128{words_[1]} << 64) | words_[0];
    }
#endif

    constexpr uint64_t& operator[](size_t i) noexcept { return words_[i]; }
    constexpr const uint64_t& operator[](size_t i) const noexcept { return words_[i]; }

    constexpr explicit operator bool() const noexcept { return (words_[0] | words_[1]) != 0; }

    /// Explicit converting operator for all builtin integral types.
    template <typename Int>
    constexpr explicit operator Int() const noexcept
        requires std::is_integral_v<Int>
    {
        return static_cast<Int>(words_[0]);
    }

    friend constexpr uint operator+(uint x, uint y) noexcept { return addc(x, y).value; }

    constexpr uint operator+() const noexcept { return *this; }

    friend constexpr uint operator-(uint x, uint y) noexcept { return subc(x, y).value; }

    constexpr uint operator-() const noexcept
    {
        // Implementing as subtraction is better than ~x + 1.
        // Clang9: Perfect.
        // GCC8: Does something weird.
        return 0 - *this;
    }

    constexpr uint& operator+=(uint y) noexcept { return *this = *this + y; }

    constexpr uint& operator-=(uint y) noexcept { return *this = *this - y; }

    constexpr uint& operator++() noexcept { return *this += 1; }

    constexpr uint& operator--() noexcept { return *this -= 1; }

    constexpr const uint operator++(int) noexcept  // NOLINT(*-const-return-type)
    {
        const auto ret = *this;
        *this += 1;
        return ret;
    }

    constexpr const uint operator--(int) noexcept  // NOLINT(*-const-return-type)
    {
        const auto ret = *this;
        *this -= 1;
        return ret;
    }

    friend constexpr bool operator==(uint x, uint y) noexcept
    {
        return ((x[0] ^ y[0]) | (x[1] ^ y[1])) == 0;
    }

    friend constexpr bool operator<(uint x, uint y) noexcept
    {
        // OPT: This should be implemented by checking the borrow of x - y,
        //      but compilers (GCC8, Clang7)
        //      have problem with properly optimizing subtraction.

#if INTX_HAS_BUILTIN_INT128
        return builtin_uint128{x} < builtin_uint128{y};
#else
        return (unsigned{x[1] < y[1]} | (unsigned{x[1] == y[1]} & unsigned{x[0] < y[0]})) != 0;
#endif
    }
    friend constexpr bool operator<=(uint x, uint y) noexcept { return !(y < x); }
    friend constexpr bool operator>(uint x, uint y) noexcept { return y < x; }
    friend constexpr bool operator>=(uint x, uint y) noexcept { return !(x < y); }

    friend constexpr std::strong_ordering operator<=>(uint x, uint y) noexcept
    {
        if (x == y)
            return std::strong_ordering::equal;

        return (x < y) ? std::strong_ordering::less : std::strong_ordering::greater;
    }

    friend constexpr uint operator~(uint x) noexcept { return {~x[0], ~x[1]}; }
    friend constexpr uint operator|(uint x, uint y) noexcept { return {x[0] | y[0], x[1] | y[1]}; }
    friend constexpr uint operator&(uint x, uint y) noexcept { return {x[0] & y[0], x[1] & y[1]}; }
    friend constexpr uint operator^(uint x, uint y) noexcept { return {x[0] ^ y[0], x[1] ^ y[1]}; }

    friend constexpr uint operator<<(uint x, uint64_t shift) noexcept
    {
        if (shift < 64)
        {
            // Find the part moved from lo to hi.
            // For shift == 0 right shift by (64 - shift) is invalid so
            // split it into 2 shifts by 1 and (63 - shift).
            return {x[0] << shift, (x[1] << shift) | ((x[0] >> 1) >> (63 - shift))};
        }
        if (shift < 128)
        {
            // The lo part becomes the shifted hi part.
            return {0, x[0] << (shift - 64)};
        }

        // Guarantee "defined" behavior for shifts larger than 128.
        return 0;
    }

    friend constexpr uint operator<<(uint x, std::integral auto shift) noexcept
    {
        static_assert(sizeof(shift) <= sizeof(uint64_t));
        return x << static_cast<uint64_t>(shift);
    }

    friend constexpr uint operator<<(uint x, uint shift) noexcept
    {
        if (shift[1] != 0) [[unlikely]]
            return 0;

        return x << shift[0];
    }

    friend constexpr uint operator>>(uint x, uint64_t shift) noexcept
    {
        if (shift < 64)
        {
            // Find the part moved from lo to hi.
            // For shift == 0 left shift by (64 - shift) is invalid so
            // split it into 2 shifts by 1 and (63 - shift).
            return {(x[0] >> shift) | ((x[1] << 1) << (63 - shift)), x[1] >> shift};
        }
        if (shift < 128)
        {
            // The lo part becomes the shifted hi part.
            return {x[1] >> (shift - 64), 0};
        }

        // Guarantee "defined" behavior for shifts larger than 128.
        return 0;
    }

    friend constexpr uint operator>>(uint x, std::integral auto shift) noexcept
    {
        static_assert(sizeof(shift) <= sizeof(uint64_t));
        return x >> static_cast<uint64_t>(shift);
    }

    friend constexpr uint operator>>(uint x, uint shift) noexcept
    {
        if (shift[1] != 0) [[unlikely]]
            return 0;

        return x >> shift[0];
    }

    friend constexpr uint operator*(uint x, uint y) noexcept
    {
        auto p = umul(x[0], y[0]);
        p[1] += (x[0] * y[1]) + (x[1] * y[0]);
        return {p[0], p[1]};
    }

    friend constexpr div_result<uint> udivrem(uint x, uint y) noexcept;
    friend constexpr uint operator/(uint x, uint y) noexcept { return udivrem(x, y).quot; }
    friend constexpr uint operator%(uint x, uint y) noexcept { return udivrem(x, y).rem; }

    constexpr uint& operator*=(uint y) noexcept { return *this = *this * y; }
    constexpr uint& operator|=(uint y) noexcept { return *this = *this | y; }
    constexpr uint& operator&=(uint y) noexcept { return *this = *this & y; }
    constexpr uint& operator^=(uint y) noexcept { return *this = *this ^ y; }
    constexpr uint& operator<<=(uint shift) noexcept { return *this = *this << shift; }
    constexpr uint& operator>>=(uint shift) noexcept { return *this = *this >> shift; }
    constexpr uint& operator/=(uint y) noexcept { return *this = *this / y; }
    constexpr uint& operator%=(uint y) noexcept { return *this = *this % y; }
};

using uint128 = uint<128>;


/// Optimized addition.
///
/// This keeps the multiprecision addition until CodeGen so the pattern is not
/// broken during other optimizations.
constexpr uint128 fast_add(uint128 x, uint128 y) noexcept
{
#if INTX_HAS_BUILTIN_INT128
    return builtin_uint128{x} + builtin_uint128{y};
#else
    return x + y;  // Fallback to generic addition.
#endif
}

/// Full unsigned multiplication 64 x 64 -> 128.
constexpr uint128 umul(uint64_t x, uint64_t y) noexcept
{
#if INTX_HAS_BUILTIN_INT128
    return builtin_uint128{x} * builtin_uint128{y};
#elif defined(_MSC_VER) && _MSC_VER >= 1925 && defined(_M_X64)
    if (!std::is_constant_evaluated())
    {
        unsigned __int64 hi = 0;
        const auto lo = _umul128(x, y, &hi);
        return {lo, hi};
    }
    // For constexpr fallback to the portable variant.
#endif

    // Portable full unsigned multiplication 64 x 64 -> 128.
    uint64_t xlo = x & 0xffffffff;
    uint64_t xhi = x >> 32;
    uint64_t ylo = y & 0xffffffff;
    uint64_t yhi = y >> 32;

    uint64_t t0 = xlo * ylo;
    uint64_t t1 = xhi * ylo;
    uint64_t t2 = xlo * yhi;
    uint64_t t3 = xhi * yhi;

    uint64_t u1 = t1 + (t0 >> 32);
    uint64_t u2 = t2 + (u1 & 0xffffffff);

    uint64_t lo = (u2 << 32) | (t0 & 0xffffffff);
    uint64_t hi = t3 + (u2 >> 32) + (u1 >> 32);
    return {lo, hi};
}

constexpr uint64_t bit_test(uint64_t x, size_t bit_index) noexcept
{
    // This pattern matches BT instruction on x86.
    // On architectures without dedicated instruction,
    // this is likely converted to (x >> bit_index) & 1.
    return (x & (uint64_t{1} << bit_index)) != 0;
}

constexpr unsigned clz(std::unsigned_integral auto x) noexcept
{
    return static_cast<unsigned>(std::countl_zero(x));
}

constexpr unsigned ctz(std::unsigned_integral auto x) noexcept
{
    return static_cast<unsigned>(std::countr_zero(x));
}

constexpr unsigned clz(uint128 x) noexcept
{
    // In this order `h == 0` we get fewer instructions than in the case of `h != 0`.
    return x[1] == 0 ? clz(x[0]) + 64 : clz(x[1]);
}

template <typename T>
T bswap(T x) noexcept = delete;  // Disable type auto promotion

constexpr uint8_t bswap(uint8_t x) noexcept
{
    return x;
}

constexpr uint16_t bswap(uint16_t x) noexcept
{
#if __has_builtin(__builtin_bswap16)
    return __builtin_bswap16(x);
#else
    #ifdef _MSC_VER
    if (!std::is_constant_evaluated())
        return _byteswap_ushort(x);
    #endif
    return static_cast<uint16_t>((x << 8) | (x >> 8));
#endif
}

constexpr uint32_t bswap(uint32_t x) noexcept
{
#if __has_builtin(__builtin_bswap32)
    return __builtin_bswap32(x);
#else
    #ifdef _MSC_VER
    if (!std::is_constant_evaluated())
        return _byteswap_ulong(x);
    #endif
    const auto a = ((x << 8) & 0xFF00FF00) | ((x >> 8) & 0x00FF00FF);
    return (a << 16) | (a >> 16);
#endif
}

constexpr uint64_t bswap(uint64_t x) noexcept
{
#if __has_builtin(__builtin_bswap64)
    return __builtin_bswap64(x);
#else
    #ifdef _MSC_VER
    if (!std::is_constant_evaluated())
        return _byteswap_uint64(x);
    #endif
    const auto a = ((x << 8) & 0xFF00FF00FF00FF00) | ((x >> 8) & 0x00FF00FF00FF00FF);
    const auto b = ((a << 16) & 0xFFFF0000FFFF0000) | ((a >> 16) & 0x0000FFFF0000FFFF);
    return (b << 32) | (b >> 32);
#endif
}

constexpr uint128 bswap(uint128 x) noexcept
{
    return {bswap(x[1]), bswap(x[0])};
}


/// Division.
/// @{

namespace internal
{
/// Reciprocal lookup table.
constexpr auto reciprocal_table = []() noexcept {
    std::array<uint16_t, 256> table{};
    for (size_t i = 0; i < table.size(); ++i)
        table[i] = static_cast<uint16_t>(0x7fd00 / (i + 256));
    return table;
}();
}  // namespace internal

/// Computes the reciprocal (2^128 - 1) / d - 2^64 for normalized d.
///
/// Based on Algorithm 2 from "Improved division by invariant integers".
constexpr uint64_t reciprocal_2by1(uint64_t d) noexcept
{
    INTX_REQUIRE(d & 0x8000000000000000);  // Must be normalized.

    const uint64_t d9 = d >> 55;
    const uint32_t v0 = internal::reciprocal_table[static_cast<size_t>(d9 - 256)];

    const uint64_t d40 = (d >> 24) + 1;
    const uint64_t v1 = (v0 << 11) - uint32_t(uint32_t{v0 * v0} * d40 >> 40) - 1;

    const uint64_t v2 = (v1 << 13) + (v1 * (0x1000000000000000 - v1 * d40) >> 47);

    const uint64_t d0 = d & 1;
    const uint64_t d63 = (d >> 1) + d0;  // ceil(d/2)
    const uint64_t e = ((v2 >> 1) & (0 - d0)) - (v2 * d63);
    const uint64_t v3 = (umul(v2, e)[1] >> 1) + (v2 << 31);

    const uint64_t v4 = v3 - (umul(v3, d) + d)[1] - d;
    return v4;
}

constexpr uint64_t reciprocal_3by2(uint128 d) noexcept
{
    auto v = reciprocal_2by1(d[1]);
    auto p = d[1] * v;
    p += d[0];
    if (p < d[0])
    {
        --v;
        if (p >= d[1])
        {
            --v;
            p -= d[1];
        }
        p -= d[1];
    }

    const auto t = umul(v, d[0]);

    p += t[1];
    if (p < t[1])
    {
        --v;
        if (p >= d[1])
        {
            if (p > d[1] || t[0] >= d[0])
                --v;
        }
    }
    return v;
}

constexpr div_result<uint64_t> udivrem_2by1(uint128 u, uint64_t d, uint64_t v) noexcept
{
    auto q = umul(v, u[1]);
    q = fast_add(q, u);

    ++q[1];

    auto r = u[0] - (q[1] * d);

    if (r > q[0])
    {
        --q[1];
        r += d;
    }

    if (r >= d)
    {
        ++q[1];
        r -= d;
    }

    return {q[1], r};
}

constexpr div_result<uint64_t, uint128> udivrem_3by2(
    uint64_t u2, uint64_t u1, uint64_t u0, uint128 d, uint64_t v) noexcept
{
    auto q = umul(v, u2);
    q = fast_add(q, {u1, u2});

    auto r1 = u1 - (q[1] * d[1]);

    auto t = umul(d[0], q[1]);

    auto r = uint128{u0, r1} - t - d;
    r1 = r[1];

    ++q[1];

    if (r1 >= q[0])
    {
        --q[1];
        r += d;
    }

    if (r >= d)
    {
        ++q[1];
        r -= d;
    }

    return {q[1], r};
}

constexpr div_result<uint128> udivrem(uint128 x, uint128 y) noexcept
{
    if (y[1] == 0)
    {
        INTX_REQUIRE(y[0] != 0);  // Division by 0.

        const auto lsh = clz(y[0]);
        const auto rsh = (64 - lsh) % 64;
        const auto rsh_mask = uint64_t{lsh == 0} - 1;

        const auto yn = y[0] << lsh;
        const auto xn_lo = x[0] << lsh;
        const auto xn_hi = (x[1] << lsh) | ((x[0] >> rsh) & rsh_mask);
        const auto xn_ex = (x[1] >> rsh) & rsh_mask;

        const auto v = reciprocal_2by1(yn);
        const auto res1 = udivrem_2by1({xn_hi, xn_ex}, yn, v);
        const auto res2 = udivrem_2by1({xn_lo, res1.rem}, yn, v);
        return {{res2.quot, res1.quot}, res2.rem >> lsh};
    }

    if (y[1] > x[1])
        return {0, x};

    const auto lsh = clz(y[1]);
    if (lsh == 0)
    {
        const auto q = unsigned{y[1] < x[1]} | unsigned{y[0] <= x[0]};
        return {q, x - (q ? y : 0)};
    }

    const auto rsh = 64 - lsh;

    const auto yn_lo = y[0] << lsh;
    const auto yn_hi = (y[1] << lsh) | (y[0] >> rsh);
    const auto xn_lo = x[0] << lsh;
    const auto xn_hi = (x[1] << lsh) | (x[0] >> rsh);
    const auto xn_ex = x[1] >> rsh;

    const auto v = reciprocal_3by2({yn_lo, yn_hi});
    const auto res = udivrem_3by2(xn_ex, xn_hi, xn_lo, {yn_lo, yn_hi}, v);

    return {res.quot, res.rem >> lsh};
}

constexpr div_result<uint128> sdivrem(uint128 x, uint128 y) noexcept
{
    constexpr auto sign_mask = uint128{1} << 127;
    const auto x_is_neg = (x & sign_mask) != 0;
    const auto y_is_neg = (y & sign_mask) != 0;

    const auto x_abs = x_is_neg ? -x : x;
    const auto y_abs = y_is_neg ? -y : y;

    const auto q_is_neg = x_is_neg ^ y_is_neg;

    const auto res = udivrem(x_abs, y_abs);

    return {q_is_neg ? -res.quot : res.quot, x_is_neg ? -res.rem : res.rem};
}

/// @}

}  // namespace intx


namespace std
{
template <unsigned N>
struct numeric_limits<intx::uint<N>>  // NOLINT(cert-dcl58-cpp)
{
    using type = intx::uint<N>;

    static constexpr bool is_specialized = true;
    static constexpr bool is_integer = true;
    static constexpr bool is_signed = false;
    static constexpr bool is_exact = true;
    static constexpr bool has_infinity = false;
    static constexpr bool has_quiet_NaN = false;
    static constexpr bool has_signaling_NaN = false;
    static constexpr float_round_style round_style = round_toward_zero;
    static constexpr bool is_iec559 = false;
    static constexpr bool is_bounded = true;
    static constexpr bool is_modulo = true;
    static constexpr int digits = CHAR_BIT * sizeof(type);
    static constexpr int digits10 = int(0.3010299956639812 * digits);
    static constexpr int max_digits10 = 0;
    static constexpr int radix = 2;
    static constexpr int min_exponent = 0;
    static constexpr int min_exponent10 = 0;
    static constexpr int max_exponent = 0;
    static constexpr int max_exponent10 = 0;
    static constexpr bool traps = std::numeric_limits<unsigned>::traps;
    static constexpr bool tinyness_before = false;

    static constexpr type min() noexcept { return 0; }
    static constexpr type lowest() noexcept { return min(); }
    static constexpr type max() noexcept { return ~type{0}; }
    static constexpr type epsilon() noexcept { return 0; }
    static constexpr type round_error() noexcept { return 0; }
    static constexpr type infinity() noexcept { return 0; }
    static constexpr type quiet_NaN() noexcept { return 0; }
    static constexpr type signaling_NaN() noexcept { return 0; }
    static constexpr type denorm_min() noexcept { return 0; }
};
}  // namespace std

namespace intx
{
template <typename T>
[[noreturn]] inline void throw_(const char* what)
{
#if __cpp_exceptions
    throw T{what};
#else
    std::fputs(what, stderr);
    std::abort();
#endif
}

constexpr int from_dec_digit(char c)
{
    if (c < '0' || c > '9')
        throw_<std::invalid_argument>("invalid digit");
    return c - '0';
}

constexpr int from_hex_digit(char c)
{
    if (c >= 'a' && c <= 'f')
        return c - ('a' - 10);
    if (c >= 'A' && c <= 'F')
        return c - ('A' - 10);
    return from_dec_digit(c);
}

template <typename Int>
constexpr Int from_string(const char* str)
{
    auto s = str;
    auto x = Int{};
    int num_digits = 0;

    if (s[0] == '0' && s[1] == 'x')
    {
        s += 2;
        while (const auto c = *s++)
        {
            if (++num_digits > int{sizeof(x) * 2})
                throw_<std::out_of_range>(str);
            x = (x << uint64_t{4}) | from_hex_digit(c);
        }
        return x;
    }

    while (const auto c = *s++)
    {
        if (num_digits++ > std::numeric_limits<Int>::digits10)
            throw_<std::out_of_range>(str);

        const auto d = from_dec_digit(c);
        x = x * Int{10} + d;
        if (x < d)
            throw_<std::out_of_range>(str);
    }
    return x;
}

template <typename Int>
constexpr Int from_string(const std::string& s)
{
    return from_string<Int>(s.c_str());
}

template <unsigned N>
inline std::string to_string(uint<N> x, int base = 10)
{
    if (base < 2 || base > 36)
        throw_<std::invalid_argument>("invalid base");

    if (x == 0)
        return "0";

    auto s = std::string{};
    while (x != 0)
    {
        // TODO: Use constexpr udivrem_1?
        const auto res = udivrem(x, uint<N>{base});
        const auto d = int(res.rem);
        const auto c = d < 10 ? '0' + d : 'a' + d - 10;
        s.push_back(char(c));
        x = res.quot;
    }
    std::ranges::reverse(s);
    return s;
}

template <unsigned N>
inline std::string hex(uint<N> x)
{
    return to_string(x, 16);
}

template <unsigned N>
struct uint
{
    using word_type = uint64_t;
    static constexpr auto word_num_bits = sizeof(word_type) * 8;
    static constexpr auto num_bits = N;
    static constexpr auto num_words = num_bits / word_num_bits;

    static_assert(N >= 2 * word_num_bits, "Number of bits must be at lest 128");
    static_assert(N % word_num_bits == 0, "Number of bits must be a multiply of 64");

private:
    uint64_t words_[num_words]{};

public:
    constexpr uint() noexcept = default;

    /// Implicit converting constructor for any smaller uint type.
    template <unsigned M>
    constexpr explicit(false) uint(const uint<M>& x) noexcept
        requires(M < N)
    {
        for (size_t i = 0; i < uint<M>::num_words; ++i)
            words_[i] = x[i];
    }

    template <typename... T>
    constexpr explicit(false) uint(T... v) noexcept
        requires std::conjunction_v<std::is_convertible<T, uint64_t>...>
      : words_{static_cast<uint64_t>(v)...}
    {}

    constexpr uint64_t& operator[](size_t i) noexcept { return words_[i]; }

    constexpr const uint64_t& operator[](size_t i) const noexcept { return words_[i]; }

    constexpr explicit operator bool() const noexcept { return *this != uint{}; }

    /// Explicit converting operator to smaller uint types.
    template <unsigned M>
    constexpr explicit operator uint<M>() const noexcept
        requires(M < N)
    {
        uint<M> r;
        for (size_t i = 0; i < uint<M>::num_words; ++i)
            r[i] = words_[i];
        return r;
    }

    /// Explicit converting operator for all builtin integral types.
    template <typename Int>
    constexpr explicit operator Int() const noexcept
        requires(std::is_integral_v<Int>)
    {
        static_assert(sizeof(Int) <= sizeof(uint64_t));
        return static_cast<Int>(words_[0]);
    }

    friend constexpr uint operator+(const uint& x, const uint& y) noexcept
    {
        return addc(x, y).value;
    }

    constexpr uint& operator+=(const uint& y) noexcept { return *this = *this + y; }

    constexpr uint operator-() const noexcept { return ~*this + uint{1}; }

    friend constexpr uint operator-(const uint& x, const uint& y) noexcept
    {
        return subc(x, y).value;
    }

    constexpr uint& operator-=(const uint& y) noexcept { return *this = *this - y; }

    /// Multiplication implementation using word access
    /// and discarding the high part of the result product.
    friend constexpr uint operator*(const uint& x, const uint& y) noexcept
    {
        uint<N> p;
        for (size_t j = 0; j < num_words; j++)
        {
            uint64_t k = 0;
            for (size_t i = 0; i < (num_words - j - 1); i++)
            {
                auto a = addc(p[i + j], k);
                auto t = umul(x[i], y[j]) + uint128{a.value, a.carry};
                p[i + j] = t[0];
                k = t[1];
            }
            p[num_words - 1] += x[num_words - j - 1] * y[j] + k;
        }
        return p;
    }

    constexpr uint& operator*=(const uint& y) noexcept { return *this = *this * y; }

    friend constexpr uint operator/(const uint& x, const uint& y) noexcept
    {
        return udivrem(x, y).quot;
    }

    friend constexpr uint operator%(const uint& x, const uint& y) noexcept
    {
        return udivrem(x, y).rem;
    }

    constexpr uint& operator/=(const uint& y) noexcept { return *this = *this / y; }

    constexpr uint& operator%=(const uint& y) noexcept { return *this = *this % y; }


    constexpr uint operator~() const noexcept
    {
        uint z;
        for (size_t i = 0; i < num_words; ++i)
            z[i] = ~words_[i];
        return z;
    }

    friend constexpr uint operator|(const uint& x, const uint& y) noexcept
    {
        uint z;
        for (size_t i = 0; i < num_words; ++i)
            z[i] = x[i] | y[i];
        return z;
    }

    constexpr uint& operator|=(const uint& y) noexcept { return *this = *this | y; }

    friend constexpr uint operator&(const uint& x, const uint& y) noexcept
    {
        uint z;
        for (size_t i = 0; i < num_words; ++i)
            z[i] = x[i] & y[i];
        return z;
    }

    constexpr uint& operator&=(const uint& y) noexcept { return *this = *this & y; }

    friend constexpr uint operator^(const uint& x, const uint& y) noexcept
    {
        uint z;
        for (size_t i = 0; i < num_words; ++i)
            z[i] = x[i] ^ y[i];
        return z;
    }

    constexpr uint& operator^=(const uint& y) noexcept { return *this = *this ^ y; }

    friend constexpr bool operator==(const uint& x, const uint& y) noexcept
    {
        uint64_t folded = 0;
        for (size_t i = 0; i < num_words; ++i)
            folded |= (x[i] ^ y[i]);
        return folded == 0;
    }

    friend constexpr bool operator<(const uint& x, const uint& y) noexcept
    {
        if constexpr (N == 256)
        {
            auto xp = uint128{x[2], x[3]};
            auto yp = uint128{y[2], y[3]};
            if (xp == yp)
            {
                xp = uint128{x[0], x[1]};
                yp = uint128{y[0], y[1]};
            }
            return xp < yp;
        }
        else
            return subc(x, y).carry;
    }
    friend constexpr bool operator>(const uint& x, const uint& y) noexcept { return y < x; }
    friend constexpr bool operator>=(const uint& x, const uint& y) noexcept { return !(x < y); }
    friend constexpr bool operator<=(const uint& x, const uint& y) noexcept { return !(y < x); }

    friend constexpr std::strong_ordering operator<=>(const uint& x, const uint& y) noexcept
    {
        if (x == y)
            return std::strong_ordering::equal;

        return (x < y) ? std::strong_ordering::less : std::strong_ordering::greater;
    }

    friend constexpr uint operator<<(const uint& x, uint64_t shift) noexcept
    {
        if (shift >= num_bits) [[unlikely]]
            return 0;

        if constexpr (N == 256)
        {
            constexpr auto half_bits = num_bits / 2;

            const auto xlo = uint128{x[0], x[1]};

            if (shift < half_bits)
            {
                const auto lo = xlo << shift;

                const auto xhi = uint128{x[2], x[3]};

                // Find the part moved from lo to hi.
                // The shift right here can be invalid:
                // for shift == 0 => rshift == half_bits.
                // Split it into 2 valid shifts by (rshift - 1) and 1.
                const auto rshift = half_bits - shift;
                const auto lo_overflow = (xlo >> (rshift - 1)) >> 1;
                const auto hi = (xhi << shift) | lo_overflow;
                return {lo[0], lo[1], hi[0], hi[1]};
            }

            const auto hi = xlo << (shift - half_bits);
            return {0, 0, hi[0], hi[1]};
        }
        else
        {
            constexpr auto word_bits = sizeof(uint64_t) * 8;

            const auto s = shift % word_bits;
            const auto skip = static_cast<size_t>(shift / word_bits);

            uint r;
            uint64_t carry = 0;
            for (size_t i = 0; i < (num_words - skip); ++i)
            {
                r[i + skip] = (x[i] << s) | carry;
                carry = (x[i] >> (word_bits - s - 1)) >> 1;
            }
            return r;
        }
    }

    friend constexpr uint operator<<(const uint& x, std::integral auto shift) noexcept
    {
        static_assert(sizeof(shift) <= sizeof(uint64_t));
        return x << static_cast<uint64_t>(shift);
    }

    friend constexpr uint operator<<(const uint& x, const uint& shift) noexcept
    {
        // TODO: This optimisation should be handled by operator<.
        uint64_t high_words_fold = 0;
        for (size_t i = 1; i < num_words; ++i)
            high_words_fold |= shift[i];

        if (high_words_fold != 0) [[unlikely]]
            return 0;

        return x << shift[0];
    }

    friend constexpr uint operator>>(const uint& x, uint64_t shift) noexcept
    {
        if (shift >= num_bits) [[unlikely]]
            return 0;

        if constexpr (N == 256)
        {
            constexpr auto half_bits = num_bits / 2;

            const auto xhi = uint128{x[2], x[3]};

            if (shift < half_bits)
            {
                const auto hi = xhi >> shift;

                const auto xlo = uint128{x[0], x[1]};

                // Find the part moved from hi to lo.
                // The shift left here can be invalid:
                // for shift == 0 => lshift == half_bits.
                // Split it into 2 valid shifts by (lshift - 1) and 1.
                const auto lshift = half_bits - shift;
                const auto hi_overflow = (xhi << (lshift - 1)) << 1;
                const auto lo = (xlo >> shift) | hi_overflow;
                return {lo[0], lo[1], hi[0], hi[1]};
            }

            const auto lo = xhi >> (shift - half_bits);
            return {lo[0], lo[1], 0, 0};
        }
        else
        {
            constexpr auto word_bits = sizeof(uint64_t) * 8;

            const auto s = shift % word_bits;
            const auto skip = static_cast<size_t>(shift / word_bits);

            uint r;
            uint64_t carry = 0;
            for (size_t i = 0; i < (num_words - skip); ++i)
            {
                r[num_words - 1 - i - skip] = (x[num_words - 1 - i] >> s) | carry;
                carry = (x[num_words - 1 - i] << (word_bits - s - 1)) << 1;
            }
            return r;
        }
    }

    friend constexpr uint operator>>(const uint& x, std::integral auto shift) noexcept
    {
        static_assert(sizeof(shift) <= sizeof(uint64_t));
        return x >> static_cast<uint64_t>(shift);
    }

    friend constexpr uint operator>>(const uint& x, const uint& shift) noexcept
    {
        uint64_t high_words_fold = 0;
        for (size_t i = 1; i < num_words; ++i)
            high_words_fold |= shift[i];

        if (high_words_fold != 0) [[unlikely]]
            return 0;

        return x >> shift[0];
    }

    constexpr uint& operator<<=(uint shift) noexcept { return *this = *this << shift; }
    constexpr uint& operator>>=(uint shift) noexcept { return *this = *this >> shift; }
};

using uint256 = uint<256>;


/// Signed less than comparison.
///
/// Interprets the arguments as two's complement signed integers
/// and checks the "less than" relation.
template <unsigned N>
constexpr bool slt(const uint<N>& x, const uint<N>& y) noexcept
{
    constexpr auto top_word_idx = uint<N>::num_words - 1;
    const auto x_neg = static_cast<int64_t>(x[top_word_idx]) < 0;
    const auto y_neg = static_cast<int64_t>(y[top_word_idx]) < 0;
    return ((x_neg ^ y_neg) != 0) ? x_neg : x < y;
}


constexpr uint64_t* as_words(uint128& x) noexcept
{
    return &x[0];
}

constexpr const uint64_t* as_words(const uint128& x) noexcept
{
    return &x[0];
}

template <unsigned N>
constexpr uint64_t* as_words(uint<N>& x) noexcept
{
    return &x[0];
}

template <unsigned N>
constexpr const uint64_t* as_words(const uint<N>& x) noexcept
{
    return &x[0];
}

template <typename T>
inline uint8_t* as_bytes(T& x) noexcept
{
    static_assert(std::is_trivially_copyable_v<T>);  // As in bit_cast.
    return reinterpret_cast<uint8_t*>(&x);
}

template <typename T>
inline const uint8_t* as_bytes(const T& x) noexcept
{
    static_assert(std::is_trivially_copyable_v<T>);  // As in bit_cast.
    return reinterpret_cast<const uint8_t*>(&x);
}

template <unsigned N>
constexpr uint<2 * N> umul(const uint<N>& x, const uint<N>& y) noexcept
{
    constexpr auto num_words = uint<N>::num_words;

    uint<2 * N> p;
    for (size_t j = 0; j < num_words; ++j)
    {
        uint64_t k = 0;
        for (size_t i = 0; i < num_words; ++i)
        {
            auto a = addc(p[i + j], k);
            auto t = umul(x[i], y[j]) + uint128{a.value, a.carry};
            p[i + j] = t[0];
            k = t[1];
        }
        p[j + num_words] = k;
    }
    return p;
}

template <unsigned N>
constexpr uint<N> exp(uint<N> base, uint<N> exponent) noexcept
{
    auto result = uint<N>{1};
    if (base == 2)
        return result << exponent;

    while (exponent != 0)
    {
        if ((exponent & 1) != 0)
            result *= base;
        base *= base;
        exponent >>= 1;
    }
    return result;
}

template <unsigned N>
constexpr bool bit_test(const uint<N>& x, size_t bit_index) noexcept
{
    const auto w = x[bit_index / uint<N>::word_num_bits];
    const auto b = bit_index % uint<N>::word_num_bits;
    return bit_test(w, b);
}

template <unsigned N>
constexpr unsigned count_significant_words(const uint<N>& x) noexcept
{
    for (size_t i = uint<N>::num_words; i > 0; --i)
    {
        if (x[i - 1] != 0)
            return static_cast<unsigned>(i);
    }
    return 0;
}

constexpr unsigned count_significant_bytes(uint64_t x) noexcept
{
    return (64 - clz(x) + 7) / 8;
}

template <unsigned N>
constexpr unsigned count_significant_bytes(const uint<N>& x) noexcept
{
    const auto w = count_significant_words(x);
    return (w != 0) ? count_significant_bytes(x[w - 1]) + (w - 1) * 8 : 0;
}

template <unsigned N>
constexpr unsigned clz(const uint<N>& x) noexcept
{
    constexpr unsigned num_words = uint<N>::num_words;
    const auto s = count_significant_words(x);
    if (s == 0)
        return num_words * 64;
    return clz(x[s - 1]) + (num_words - s) * 64;
}

template <unsigned N>
constexpr unsigned ctz(const uint<N>& x) noexcept
{
    for (size_t i = 0; i < uint<N>::num_words; ++i)
    {
        if (x[i] != 0)
            return static_cast<unsigned>(i * uint<N>::word_num_bits) + ctz(x[i]);
    }
    return uint<N>::num_bits;
}

namespace internal
{
/// Counts the number of zero leading bits in nonzero argument x.
constexpr unsigned clz_nonzero(uint64_t x) noexcept
{
    INTX_REQUIRE(x != 0);
    return static_cast<unsigned>(std::countl_zero(x));
}

template <unsigned M, unsigned N>
struct normalized_div_args  // NOLINT(cppcoreguidelines-pro-type-member-init)
{
    uint<N> divisor;
    uint<M + 64> numerator;
    int num_divisor_words;
    int num_numerator_words;
    unsigned shift;
};

template <unsigned M, unsigned N>
[[gnu::always_inline]] constexpr normalized_div_args<M, N> normalize(
    const uint<M>& numerator, const uint<N>& denominator) noexcept
{
    constexpr auto num_numerator_words = uint<M>::num_words;
    constexpr auto num_denominator_words = uint<N>::num_words;

    auto* u = as_words(numerator);
    auto* v = as_words(denominator);

    normalized_div_args<M, N> na;
    auto* un = as_words(na.numerator);
    auto* vn = as_words(na.divisor);

    auto& m = na.num_numerator_words;
    for (m = num_numerator_words; m > 0 && u[m - 1] == 0; --m)
        ;

    auto& n = na.num_divisor_words;
    for (n = num_denominator_words; n > 0 && v[n - 1] == 0; --n)
        ;

    na.shift = clz_nonzero(v[n - 1]);  // Use clz_nonzero() to avoid clang analyzer's warning.
    if (na.shift)
    {
        for (int i = num_denominator_words - 1; i > 0; --i)
            vn[i] = (v[i] << na.shift) | (v[i - 1] >> (64 - na.shift));
        vn[0] = v[0] << na.shift;

        un[num_numerator_words] = u[num_numerator_words - 1] >> (64 - na.shift);
        for (int i = num_numerator_words - 1; i > 0; --i)
            un[i] = (u[i] << na.shift) | (u[i - 1] >> (64 - na.shift));
        un[0] = u[0] << na.shift;
    }
    else
    {
        na.numerator = numerator;
        na.divisor = denominator;
    }

    // Add the highest word of the normalized numerator if significant.
    if (m != 0 && (un[m] != 0 || un[m - 1] >= vn[n - 1]))
        ++m;

    return na;
}

/// Divides arbitrary long unsigned integer by 64-bit unsigned integer (1 word).
/// @param u    The array of a normalized numerator words. It will contain
///             the quotient after execution.
/// @param len  The number of numerator words.
/// @param d    The normalized divisor.
/// @return     The remainder.
constexpr uint64_t udivrem_by1(uint64_t u[], int len, uint64_t d) noexcept
{
    INTX_REQUIRE(len >= 2);

    const auto reciprocal = reciprocal_2by1(d);

    auto rem = u[len - 1];  // Set the top word as remainder.
    u[len - 1] = 0;         // Reset the word being a part of the result quotient.

    auto it = &u[len - 2];
    while (true)
    {
        std::tie(*it, rem) = udivrem_2by1({*it, rem}, d, reciprocal);
        if (it == &u[0])
            break;
        --it;
    }

    return rem;
}

/// Divides arbitrary long unsigned integer by 128-bit unsigned integer (2 words).
/// @param u    The array of a normalized numerator words. It will contain the
///             quotient after execution.
/// @param len  The number of numerator words.
/// @param d    The normalized divisor.
/// @return     The remainder.
constexpr uint128 udivrem_by2(uint64_t u[], int len, uint128 d) noexcept
{
    INTX_REQUIRE(len >= 3);

    const auto reciprocal = reciprocal_3by2(d);

    auto rem = uint128{u[len - 2], u[len - 1]};  // Set the 2 top words as remainder.
    u[len - 1] = u[len - 2] = 0;  // Reset these words being a part of the result quotient.

    auto it = &u[len - 3];
    while (true)
    {
        std::tie(*it, rem) = udivrem_3by2(rem[1], rem[0], *it, d, reciprocal);
        if (it == &u[0])
            break;
        --it;
    }

    return rem;
}

/// s = x + y.
constexpr bool add(uint64_t s[], const uint64_t x[], const uint64_t y[], int len) noexcept
{
    // OPT: Add MinLen template parameter and unroll first loop iterations.
    INTX_REQUIRE(len >= 2);

    bool carry = false;
    for (int i = 0; i < len; ++i)
        std::tie(s[i], carry) = addc(x[i], y[i], carry);
    return carry;
}

/// r = x - multiplier * y.
constexpr uint64_t submul(
    uint64_t r[], const uint64_t x[], const uint64_t y[], int len, uint64_t multiplier) noexcept
{
    // OPT: Add MinLen template parameter and unroll first loop iterations.
    INTX_REQUIRE(len >= 1);

    uint64_t borrow = 0;
    for (int i = 0; i < len; ++i)
    {
        const auto s = x[i] - borrow;
        const auto p = umul(y[i], multiplier);
        borrow = p[1] + (x[i] < s);
        r[i] = s - p[0];
        borrow += (s < r[i]);
    }
    return borrow;
}

constexpr void udivrem_knuth(
    uint64_t q[], uint64_t u[], int ulen, const uint64_t d[], int dlen) noexcept
{
    INTX_REQUIRE(dlen >= 3);
    INTX_REQUIRE(ulen >= dlen);

    const auto divisor = uint128{d[dlen - 2], d[dlen - 1]};
    const auto reciprocal = reciprocal_3by2(divisor);
    for (int j = ulen - dlen - 1; j >= 0; --j)
    {
        const auto u2 = u[j + dlen];
        const auto u1 = u[j + dlen - 1];
        const auto u0 = u[j + dlen - 2];

        uint64_t qhat{};
        if (INTX_UNLIKELY((uint128{u1, u2}) == divisor))  // Division overflows.
        {
            qhat = ~uint64_t{0};

            u[j + dlen] = u2 - submul(&u[j], &u[j], d, dlen, qhat);
        }
        else
        {
            uint128 rhat;
            std::tie(qhat, rhat) = udivrem_3by2(u2, u1, u0, divisor, reciprocal);

            bool carry{};
            const auto overflow = submul(&u[j], &u[j], d, dlen - 2, qhat);
            std::tie(u[j + dlen - 2], carry) = subc(rhat[0], overflow);
            std::tie(u[j + dlen - 1], carry) = subc(rhat[1], carry);

            if (INTX_UNLIKELY(carry))
            {
                --qhat;
                u[j + dlen - 1] += divisor[1] + add(&u[j], &u[j], d, dlen - 1);
            }
        }

        q[j] = qhat;  // Store quotient digit.
    }
}

}  // namespace internal

template <unsigned M, unsigned N>
constexpr div_result<uint<M>, uint<N>> udivrem(const uint<M>& u, const uint<N>& v) noexcept
{
    auto na = internal::normalize(u, v);
    INTX_REQUIRE(na.num_divisor_words > 0);
    INTX_REQUIRE(na.num_divisor_words <= static_cast<int>(uint<N>::num_words));
    INTX_REQUIRE(na.num_numerator_words >= 0);
    INTX_REQUIRE(na.num_numerator_words <= static_cast<int>(uint<M>::num_words) + 1);

    if (na.num_numerator_words <= na.num_divisor_words)
        return {0, static_cast<uint<N>>(u)};

    auto un = as_words(na.numerator);  // Will be modified.

    static_assert(uint<N>::num_words >= 2, "no support for uint<64> yet");
    if (na.num_divisor_words == 1)
    {
        const auto r = internal::udivrem_by1(un, na.num_numerator_words, na.divisor[0]);
        return {static_cast<uint<M>>(na.numerator), r >> na.shift};
    }

    if (na.num_divisor_words == 2)
    {
        const auto r =
            internal::udivrem_by2(un, na.num_numerator_words, static_cast<uint128>(na.divisor));
        return {static_cast<uint<M>>(na.numerator), r >> na.shift};
    }


    uint<M> q;
    internal::udivrem_knuth(
        as_words(q), &un[0], na.num_numerator_words, as_words(na.divisor), na.num_divisor_words);

    uint<N> r;
    auto rw = as_words(r);
    for (int i = 0; i < na.num_divisor_words - 1; ++i)
        rw[i] = na.shift ? (un[i] >> na.shift) | (un[i + 1] << (64 - na.shift)) : un[i];
    rw[na.num_divisor_words - 1] = un[na.num_divisor_words - 1] >> na.shift;

    return {q, r};
}

template <unsigned N>
constexpr div_result<uint<N>> sdivrem(const uint<N>& u, const uint<N>& v) noexcept
{
    const auto sign_mask = uint<N>{1} << (uint<N>::num_bits - 1);
    auto u_is_neg = (u & sign_mask) != 0;
    auto v_is_neg = (v & sign_mask) != 0;

    auto u_abs = u_is_neg ? -u : u;
    auto v_abs = v_is_neg ? -v : v;

    auto q_is_neg = u_is_neg ^ v_is_neg;

    auto res = udivrem(u_abs, v_abs);

    return {q_is_neg ? -res.quot : res.quot, u_is_neg ? -res.rem : res.rem};
}

constexpr uint256 bswap(const uint256& x) noexcept
{
    return {bswap(x[3]), bswap(x[2]), bswap(x[1]), bswap(x[0])};
}

template <unsigned N>
constexpr uint<N> bswap(const uint<N>& x) noexcept
{
    constexpr auto num_words = uint<N>::num_words;
    uint<N> z;
    for (size_t i = 0; i < num_words; ++i)
        z[num_words - 1 - i] = bswap(x[i]);
    return z;
}


constexpr uint256 addmod(const uint256& x, const uint256& y, const uint256& mod) noexcept
{
    // Fast path for mod >= 2^192, with x and y at most slightly bigger than mod.
    // This is always the case when x and y are already reduced modulo mod.
    // Based on https://github.com/holiman/uint256/pull/86.
    if ((mod[3] != 0) && (x[3] <= mod[3]) && (y[3] <= mod[3]))
    {
        // Normalize x in case it is bigger than mod.
        auto xn = x;
        auto xd = subc(x, mod);
        if (!xd.carry)
            xn = xd.value;

        // Normalize y in case it is bigger than mod.
        auto yn = y;
        auto yd = subc(y, mod);
        if (!yd.carry)
            yn = yd.value;

        auto a = addc(xn, yn);
        auto av = a.value;
        auto b = subc(av, mod);
        auto bv = b.value;
        if (a.carry || !b.carry)
            return bv;
        return av;
    }

    auto s = addc(x, y);
    uint<256 + 64> n = s.value;
    n[4] = s.carry;
    return udivrem(n, mod).rem;
}

constexpr uint256 mulmod(const uint256& x, const uint256& y, const uint256& mod) noexcept
{
    return udivrem(umul(x, y), mod).rem;
}

#define INTX_JOIN(X, Y) X##Y
/// Define type alias uintN = uint<N> and the matching literal ""_uN.
/// The literal operators are defined in the intx::literals namespace.
#define DEFINE_ALIAS_AND_LITERAL(N)                               \
    using uint##N = uint<N>;                                      \
    namespace literals                                            \
    {                                                             \
    consteval uint##N INTX_JOIN(operator"", _u##N)(const char* s) \
    {                                                             \
        return from_string<uint##N>(s);                           \
    }                                                             \
    }
DEFINE_ALIAS_AND_LITERAL(128);
DEFINE_ALIAS_AND_LITERAL(192);
DEFINE_ALIAS_AND_LITERAL(256);
DEFINE_ALIAS_AND_LITERAL(320);
DEFINE_ALIAS_AND_LITERAL(384);
DEFINE_ALIAS_AND_LITERAL(448);
DEFINE_ALIAS_AND_LITERAL(512);
#undef DEFINE_ALIAS_AND_LITERAL
#undef INTX_JOIN

using namespace literals;

/// Convert native representation to/from little-endian byte order.
/// intx and built-in integral types are supported.
template <typename T>
constexpr T to_little_endian(const T& x) noexcept
{
    if constexpr (std::endian::native == std::endian::little)
        return x;
    else if constexpr (std::is_integral_v<T>)
        return bswap(x);
    else  // Wordwise bswap.
    {
        T r;
        for (size_t i = 0; i < T::num_words; ++i)
            r[i] = bswap(x[i]);
        return r;
    }
}

/// Convert native representation to/from big-endian byte order.
/// intx and built-in integral types are supported.
template <typename T>
constexpr T to_big_endian(const T& x) noexcept
{
    if constexpr (std::endian::native == std::endian::little)
        return bswap(x);
    else if constexpr (std::is_integral_v<T>)
        return x;
    else  // Swap words.
    {
        T r;
        for (size_t i = 0; i < T::num_words; ++i)
            r[T::num_words - 1 - i] = x[i];
        return r;
    }
}

namespace le  // Conversions to/from LE bytes.
{
template <typename T, unsigned M>
inline T load(const uint8_t (&src)[M]) noexcept
{
    static_assert(
        M == sizeof(T), "the size of source bytes must match the size of the destination uint");
    T x;
    std::memcpy(&x, src, sizeof(x));
    return to_little_endian(x);
}

template <typename T>
inline void store(uint8_t (&dst)[sizeof(T)], const T& x) noexcept
{
    const auto d = to_little_endian(x);
    std::memcpy(dst, &d, sizeof(d));
}

namespace unsafe
{
template <typename T>
inline T load(const uint8_t* src) noexcept
{
    T x;
    std::memcpy(&x, src, sizeof(x));
    return to_little_endian(x);
}

template <typename T>
inline void store(uint8_t* dst, const T& x) noexcept
{
    const auto d = to_little_endian(x);
    std::memcpy(dst, &d, sizeof(d));
}
}  // namespace unsafe
}  // namespace le


namespace be  // Conversions to/from BE bytes.
{
/// Loads an integer value from bytes of big-endian order.
/// If the size of bytes is smaller than the result, the value is zero-extended.
template <typename T, unsigned M>
inline T load(const uint8_t (&src)[M]) noexcept
{
    static_assert(M <= sizeof(T),
        "the size of source bytes must not exceed the size of the destination uint");
    T x{};
    std::memcpy(&as_bytes(x)[sizeof(T) - M], src, M);
    x = to_big_endian(x);
    return x;
}

template <typename IntT, typename T>
inline IntT load(const T& t) noexcept
{
    return load<IntT>(t.bytes);
}

/// Stores an integer value in a bytes array in big-endian order.
template <typename T>
inline void store(uint8_t (&dst)[sizeof(T)], const T& x) noexcept
{
    const auto d = to_big_endian(x);
    std::memcpy(dst, &d, sizeof(d));
}

/// Stores an SrcT value in .bytes field of type DstT. The .bytes must be an array of uint8_t
/// of the size matching the size of uint.
template <typename DstT, typename SrcT>
inline DstT store(const SrcT& x) noexcept
{
    DstT r{};
    store(r.bytes, x);
    return r;
}

/// Stores the truncated value of an uint in a bytes array.
/// Only the least significant bytes from big-endian representation of the uint
/// are stored in the result bytes array up to array's size.
template <unsigned M, unsigned N>
inline void trunc(uint8_t (&dst)[M], const uint<N>& x) noexcept
{
    static_assert(M < N / 8, "destination must be smaller than the source value");
    const auto d = to_big_endian(x);
    std::memcpy(dst, &as_bytes(d)[sizeof(d) - M], M);
}

/// Stores the truncated value of an uint in the .bytes field of an object of type T.
template <typename T, unsigned N>
inline T trunc(const uint<N>& x) noexcept
{
    T r{};
    trunc(r.bytes, x);
    return r;
}

namespace unsafe
{
/// Loads an uint value from a buffer. The user must make sure
/// that the provided buffer is big enough. Therefore, marked "unsafe".
template <typename IntT>
inline IntT load(const uint8_t* src) noexcept
{
    // Align bytes.
    // TODO: Using memcpy() directly triggers this optimization bug in GCC:
    //   https://gcc.gnu.org/bugzilla/show_bug.cgi?id=107837
    alignas(IntT) std::byte aligned_storage[sizeof(IntT)];
    std::memcpy(&aligned_storage, src, sizeof(IntT));
    // TODO(C++23): Use std::start_lifetime_as<uint256>().
    return to_big_endian(*reinterpret_cast<const IntT*>(&aligned_storage));
}

/// Stores an integer value at the provided pointer in big-endian order. The user must make sure
/// that the provided buffer is big enough to fit the value. Therefore, marked "unsafe".
template <typename T>
inline void store(uint8_t* dst, const T& x) noexcept
{
    const auto d = to_big_endian(x);
    std::memcpy(dst, &d, sizeof(d));
}

/// Specialization for uint256.
inline void store(uint8_t* dst, const uint256& x) noexcept
{
    // Store byte-swapped words in primitive temporaries. This helps with memory aliasing
    // and GCC bug https://gcc.gnu.org/bugzilla/show_bug.cgi?id=107837
    // TODO: Use std::byte instead of uint8_t.
    const auto v0 = to_big_endian(x[0]);
    const auto v1 = to_big_endian(x[1]);
    const auto v2 = to_big_endian(x[2]);
    const auto v3 = to_big_endian(x[3]);

    // Store words in reverse (big-endian) order, write addresses are ascending.
    std::memcpy(dst, &v3, sizeof(v3));
    std::memcpy(dst + 8, &v2, sizeof(v2));
    std::memcpy(dst + 16, &v1, sizeof(v1));
    std::memcpy(dst + 24, &v0, sizeof(v0));
}

}  // namespace unsafe

}  // namespace be

}  // namespace intx

#ifdef _MSC_VER
    #pragma warning(pop)
#endif
