// intx: extended precision integer library.
// Copyright 2019 Pawel Bylica.
// Licensed under the Apache License, Version 2.0.

#pragma once

#include <algorithm>
#include <cassert>
#include <climits>
#include <cstdint>
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

#ifdef _MSC_VER
    #include <intrin.h>
#endif

#if !defined(__has_builtin)
    #define __has_builtin(NAME) 0
#endif

#if !defined(__has_feature)
    #define __has_feature(NAME) 0
#endif

#if !defined(NDEBUG)
    #define INTX_UNREACHABLE() assert(false)
#elif __has_builtin(__builtin_unreachable)
    #define INTX_UNREACHABLE() __builtin_unreachable()
#elif defined(_MSC_VER)
    #define INTX_UNREACHABLE() __assume(0)
#else
    #define INTX_UNREACHABLE() (void)0
#endif


#if __has_builtin(__builtin_expect)
    #define INTX_UNLIKELY(EXPR) __builtin_expect(bool{EXPR}, false)
#else
    #define INTX_UNLIKELY(EXPR) (bool{EXPR})
#endif

#if !defined(NDEBUG)
    #define INTX_REQUIRE assert
#else
    #define INTX_REQUIRE(X) (X) ? (void)0 : INTX_UNREACHABLE()
#endif


// Detect compiler support for 128-bit integer __int128
#if defined(__SIZEOF_INT128__)
    #define INTX_HAS_BUILTIN_INT128 1
#else
    #define INTX_HAS_BUILTIN_INT128 0
#endif

namespace intx
{
#if INTX_HAS_BUILTIN_INT128
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpedantic"  // Usage of __int128 triggers a pedantic warning.

/// Alias for the compiler supported unsigned __int128 type.
using builtin_uint128 = unsigned __int128;

    #pragma GCC diagnostic pop
#endif

constexpr bool byte_order_is_little_endian =
#if defined(__BYTE_ORDER__)
    (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__);
#elif defined(_WIN32)
    true;  // On Windows assume little endian.
#else
    #error "Unknown endianness"
#endif

template <unsigned N>
struct uint;

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

    template <typename T,
        typename = typename std::enable_if_t<std::is_convertible<T, uint64_t>::value>>
    constexpr uint(T x) noexcept : words_{static_cast<uint64_t>(x), 0}  // NOLINT
    {}

#if INTX_HAS_BUILTIN_INT128
    constexpr uint(builtin_uint128 x) noexcept  // NOLINT
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
    template <typename Int, typename = typename std::enable_if<std::is_integral<Int>::value>::type>
    constexpr explicit operator Int() const noexcept
    {
        return static_cast<Int>(words_[0]);
    }
};

using uint128 = uint<128>;


inline constexpr bool is_constant_evaluated() noexcept
{
#if __has_builtin(__builtin_is_constant_evaluated) || (defined(_MSC_VER) && _MSC_VER >= 1925)
    return __builtin_is_constant_evaluated();
#else
    return true;
#endif
}


/// Contains result of add/sub/etc with a carry flag.
template <typename T>
struct result_with_carry
{
    T value;
    bool carry;

    /// Conversion to tuple of references, to allow usage with std::tie().
    constexpr operator std::tuple<T&, bool&>() noexcept { return {value, carry}; }
};


/// Linear arithmetic operators.
/// @{

/// Addition with carry.
inline constexpr result_with_carry<uint64_t> addc(
    uint64_t x, uint64_t y, bool carry = false) noexcept
{
#if __has_builtin(__builtin_addcll)
    if (!is_constant_evaluated())
    {
        unsigned long long carryout = 0;  // NOLINT(google-runtime-int)
        const auto s = __builtin_addcll(x, y, carry, &carryout);
        static_assert(sizeof(s) == sizeof(uint64_t));
        return {s, static_cast<bool>(carryout)};
    }
#elif __has_builtin(__builtin_ia32_addcarryx_u64)
    if (!is_constant_evaluated())
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
inline constexpr result_with_carry<uint64_t> subc(
    uint64_t x, uint64_t y, bool carry = false) noexcept
{
#if __has_builtin(__builtin_subcll)
    if (!is_constant_evaluated())
    {
        unsigned long long carryout = 0;  // NOLINT(google-runtime-int)
        const auto d = __builtin_subcll(x, y, carry, &carryout);
        static_assert(sizeof(d) == sizeof(uint64_t));
        return {d, static_cast<bool>(carryout)};
    }
#elif __has_builtin(__builtin_ia32_sbb_u64)
    if (!is_constant_evaluated())
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
inline constexpr result_with_carry<uint<N>> addc(
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

inline constexpr uint128 operator+(uint128 x, uint128 y) noexcept
{
    return addc(x, y).value;
}

inline constexpr uint128 operator+(uint128 x) noexcept
{
    return x;
}

/// Performs subtraction of two unsigned numbers and returns the difference
/// and the carry bit (aka borrow, overflow).
template <unsigned N>
inline constexpr result_with_carry<uint<N>> subc(
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

inline constexpr uint128 operator-(uint128 x, uint128 y) noexcept
{
    return subc(x, y).value;
}

inline constexpr uint128 operator-(uint128 x) noexcept
{
    // Implementing as subtraction is better than ~x + 1.
    // Clang9: Perfect.
    // GCC8: Does something weird.
    return 0 - x;
}

inline uint128& operator++(uint128& x) noexcept
{
    return x = x + 1;
}

inline uint128& operator--(uint128& x) noexcept
{
    return x = x - 1;
}

inline const uint128 operator++(uint128& x, int) noexcept  // NOLINT(readability-const-return-type)
{
    const auto ret = x;
    ++x;
    return ret;
}

inline const uint128 operator--(uint128& x, int) noexcept  // NOLINT(readability-const-return-type)
{
    const auto ret = x;
    --x;
    return ret;
}

/// Optimized addition.
///
/// This keeps the multiprecision addition until CodeGen so the pattern is not
/// broken during other optimizations.
inline constexpr uint128 fast_add(uint128 x, uint128 y) noexcept
{
#if INTX_HAS_BUILTIN_INT128
    return builtin_uint128{x} + builtin_uint128{y};
#else
    return x + y;  // Fallback to generic addition.
#endif
}

/// @}


/// Comparison operators.
///
/// In all implementations bitwise operators are used instead of logical ones
/// to avoid branching.
///
/// @{

inline constexpr bool operator==(uint128 x, uint128 y) noexcept
{
    return ((x[0] ^ y[0]) | (x[1] ^ y[1])) == 0;
}

inline constexpr bool operator!=(uint128 x, uint128 y) noexcept
{
    return !(x == y);
}

inline constexpr bool operator<(uint128 x, uint128 y) noexcept
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

inline constexpr bool operator<=(uint128 x, uint128 y) noexcept
{
    return !(y < x);
}

inline constexpr bool operator>(uint128 x, uint128 y) noexcept
{
    return y < x;
}

inline constexpr bool operator>=(uint128 x, uint128 y) noexcept
{
    return !(x < y);
}

/// @}


/// Bitwise operators.
/// @{

inline constexpr uint128 operator~(uint128 x) noexcept
{
    return {~x[0], ~x[1]};
}

inline constexpr uint128 operator|(uint128 x, uint128 y) noexcept
{
    // Clang: perfect.
    // GCC 8-12: stupidly uses a vector instruction in all bitwise operators.
    return {x[0] | y[0], x[1] | y[1]};
}

inline constexpr uint128 operator&(uint128 x, uint128 y) noexcept
{
    return {x[0] & y[0], x[1] & y[1]};
}

inline constexpr uint128 operator^(uint128 x, uint128 y) noexcept
{
    return {x[0] ^ y[0], x[1] ^ y[1]};
}

inline constexpr uint128 operator<<(uint128 x, uint64_t shift) noexcept
{
    return (shift < 64) ?
               // Find the part moved from lo to hi.
               // For shift == 0 right shift by (64 - shift) is invalid so
               // split it into 2 shifts by 1 and (63 - shift).
               uint128{x[0] << shift, (x[1] << shift) | ((x[0] >> 1) >> (63 - shift))} :

               // Guarantee "defined" behavior for shifts larger than 128.
               (shift < 128) ? uint128{0, x[0] << (shift - 64)} : 0;
}

inline constexpr uint128 operator<<(uint128 x, uint128 shift) noexcept
{
    if (INTX_UNLIKELY(shift[1] != 0))
        return 0;

    return x << shift[0];
}

inline constexpr uint128 operator>>(uint128 x, uint64_t shift) noexcept
{
    return (shift < 64) ?
               // Find the part moved from lo to hi.
               // For shift == 0 left shift by (64 - shift) is invalid so
               // split it into 2 shifts by 1 and (63 - shift).
               uint128{(x[0] >> shift) | ((x[1] << 1) << (63 - shift)), x[1] >> shift} :

               // Guarantee "defined" behavior for shifts larger than 128.
               (shift < 128) ? uint128{x[1] >> (shift - 64)} : 0;
}

inline constexpr uint128 operator>>(uint128 x, uint128 shift) noexcept
{
    if (INTX_UNLIKELY(shift[1] != 0))
        return 0;

    return x >> static_cast<uint64_t>(shift);
}

/// @}


/// Multiplication
/// @{

/// Full unsigned multiplication 64 x 64 -> 128.
inline constexpr uint128 umul(uint64_t x, uint64_t y) noexcept
{
#if INTX_HAS_BUILTIN_INT128
    return builtin_uint128{x} * builtin_uint128{y};
#elif defined(_MSC_VER) && _MSC_VER >= 1925 && defined(_M_X64)
    if (!is_constant_evaluated())
    {
        unsigned __int64 hi = 0;
        const auto lo = _umul128(x, y, &hi);
        return {lo, hi};
    }
    // For constexpr fallback to portable variant.
#endif

    // Portable full unsigned multiplication 64 x 64 -> 128.
    uint64_t xl = x & 0xffffffff;
    uint64_t xh = x >> 32;
    uint64_t yl = y & 0xffffffff;
    uint64_t yh = y >> 32;

    uint64_t t0 = xl * yl;
    uint64_t t1 = xh * yl;
    uint64_t t2 = xl * yh;
    uint64_t t3 = xh * yh;

    uint64_t u1 = t1 + (t0 >> 32);
    uint64_t u2 = t2 + (u1 & 0xffffffff);

    uint64_t lo = (u2 << 32) | (t0 & 0xffffffff);
    uint64_t hi = t3 + (u2 >> 32) + (u1 >> 32);
    return {lo, hi};
}

inline constexpr uint128 operator*(uint128 x, uint128 y) noexcept
{
    auto p = umul(x[0], y[0]);
    p[1] += (x[0] * y[1]) + (x[1] * y[0]);
    return {p[0], p[1]};
}

/// @}


/// Assignment operators.
/// @{

inline constexpr uint128& operator+=(uint128& x, uint128 y) noexcept
{
    return x = x + y;
}

inline constexpr uint128& operator-=(uint128& x, uint128 y) noexcept
{
    return x = x - y;
}

inline uint128& operator*=(uint128& x, uint128 y) noexcept
{
    return x = x * y;
}

inline constexpr uint128& operator|=(uint128& x, uint128 y) noexcept
{
    return x = x | y;
}

inline constexpr uint128& operator&=(uint128& x, uint128 y) noexcept
{
    return x = x & y;
}

inline constexpr uint128& operator^=(uint128& x, uint128 y) noexcept
{
    return x = x ^ y;
}

inline constexpr uint128& operator<<=(uint128& x, uint64_t shift) noexcept
{
    return x = x << shift;
}

inline constexpr uint128& operator>>=(uint128& x, uint64_t shift) noexcept
{
    return x = x >> shift;
}

/// @}


inline constexpr unsigned clz_generic(uint32_t x) noexcept
{
    unsigned n = 32;
    for (int i = 4; i >= 0; --i)
    {
        const auto s = unsigned{1} << i;
        const auto hi = x >> s;
        if (hi != 0)
        {
            n -= s;
            x = hi;
        }
    }
    return n - x;
}

inline constexpr unsigned clz_generic(uint64_t x) noexcept
{
    unsigned n = 64;
    for (int i = 5; i >= 0; --i)
    {
        const auto s = unsigned{1} << i;
        const auto hi = x >> s;
        if (hi != 0)
        {
            n -= s;
            x = hi;
        }
    }
    return n - static_cast<unsigned>(x);
}

inline constexpr unsigned clz(uint32_t x) noexcept
{
#ifdef _MSC_VER
    return clz_generic(x);
#else
    return x != 0 ? unsigned(__builtin_clz(x)) : 32;
#endif
}

inline constexpr unsigned clz(uint64_t x) noexcept
{
#ifdef _MSC_VER
    return clz_generic(x);
#else
    return x != 0 ? unsigned(__builtin_clzll(x)) : 64;
#endif
}

inline constexpr unsigned clz(uint128 x) noexcept
{
    // In this order `h == 0` we get less instructions than in case of `h != 0`.
    return x[1] == 0 ? clz(x[0]) + 64 : clz(x[1]);
}

template <typename T>
T bswap(T x) noexcept = delete;  // Disable type auto promotion

inline constexpr uint8_t bswap(uint8_t x) noexcept
{
    return x;
}

inline constexpr uint16_t bswap(uint16_t x) noexcept
{
#if __has_builtin(__builtin_bswap16)
    return __builtin_bswap16(x);
#else
    #ifdef _MSC_VER
    if (!is_constant_evaluated())
        return _byteswap_ushort(x);
    #endif
    return static_cast<uint16_t>((x << 8) | (x >> 8));
#endif
}

inline constexpr uint32_t bswap(uint32_t x) noexcept
{
#if __has_builtin(__builtin_bswap32)
    return __builtin_bswap32(x);
#else
    #ifdef _MSC_VER
    if (!is_constant_evaluated())
        return _byteswap_ulong(x);
    #endif
    const auto a = ((x << 8) & 0xFF00FF00) | ((x >> 8) & 0x00FF00FF);
    return (a << 16) | (a >> 16);
#endif
}

inline constexpr uint64_t bswap(uint64_t x) noexcept
{
#if __has_builtin(__builtin_bswap64)
    return __builtin_bswap64(x);
#else
    #ifdef _MSC_VER
    if (!is_constant_evaluated())
        return _byteswap_uint64(x);
    #endif
    const auto a = ((x << 8) & 0xFF00FF00FF00FF00) | ((x >> 8) & 0x00FF00FF00FF00FF);
    const auto b = ((a << 16) & 0xFFFF0000FFFF0000) | ((a >> 16) & 0x0000FFFF0000FFFF);
    return (b << 32) | (b >> 32);
#endif
}

inline constexpr uint128 bswap(uint128 x) noexcept
{
    return {bswap(x[1]), bswap(x[0])};
}


/// Division.
/// @{

template <typename QuotT, typename RemT = QuotT>
struct div_result
{
    QuotT quot;
    RemT rem;

    /// Conversion to tuple of references, to allow usage with std::tie().
    constexpr operator std::tuple<QuotT&, RemT&>() noexcept { return {quot, rem}; }
};

namespace internal
{
inline constexpr uint16_t reciprocal_table_item(uint8_t d9) noexcept
{
    return uint16_t(0x7fd00 / (0x100 | d9));
}

#define REPEAT4(x)                                                  \
    reciprocal_table_item((x) + 0), reciprocal_table_item((x) + 1), \
        reciprocal_table_item((x) + 2), reciprocal_table_item((x) + 3)

#define REPEAT32(x)                                                                         \
    REPEAT4((x) + 4 * 0), REPEAT4((x) + 4 * 1), REPEAT4((x) + 4 * 2), REPEAT4((x) + 4 * 3), \
        REPEAT4((x) + 4 * 4), REPEAT4((x) + 4 * 5), REPEAT4((x) + 4 * 6), REPEAT4((x) + 4 * 7)

#define REPEAT256()                                                                           \
    REPEAT32(32 * 0), REPEAT32(32 * 1), REPEAT32(32 * 2), REPEAT32(32 * 3), REPEAT32(32 * 4), \
        REPEAT32(32 * 5), REPEAT32(32 * 6), REPEAT32(32 * 7)

/// Reciprocal lookup table.
constexpr uint16_t reciprocal_table[] = {REPEAT256()};

#undef REPEAT4
#undef REPEAT32
#undef REPEAT256
}  // namespace internal

/// Computes the reciprocal (2^128 - 1) / d - 2^64 for normalized d.
///
/// Based on Algorithm 2 from "Improved division by invariant integers".
inline uint64_t reciprocal_2by1(uint64_t d) noexcept
{
    INTX_REQUIRE(d & 0x8000000000000000);  // Must be normalized.

    const uint64_t d9 = d >> 55;
    const uint32_t v0 = internal::reciprocal_table[d9 - 256];

    const uint64_t d40 = (d >> 24) + 1;
    const uint64_t v1 = (v0 << 11) - uint32_t(uint32_t{v0 * v0} * d40 >> 40) - 1;

    const uint64_t v2 = (v1 << 13) + (v1 * (0x1000000000000000 - v1 * d40) >> 47);

    const uint64_t d0 = d & 1;
    const uint64_t d63 = (d >> 1) + d0;  // ceil(d/2)
    const uint64_t e = ((v2 >> 1) & (0 - d0)) - v2 * d63;
    const uint64_t v3 = (umul(v2, e)[1] >> 1) + (v2 << 31);

    const uint64_t v4 = v3 - (umul(v3, d) + d)[1] - d;
    return v4;
}

inline uint64_t reciprocal_3by2(uint128 d) noexcept
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

inline div_result<uint64_t> udivrem_2by1(uint128 u, uint64_t d, uint64_t v) noexcept
{
    auto q = umul(v, u[1]);
    q = fast_add(q, u);

    ++q[1];

    auto r = u[0] - q[1] * d;

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

inline div_result<uint64_t, uint128> udivrem_3by2(
    uint64_t u2, uint64_t u1, uint64_t u0, uint128 d, uint64_t v) noexcept
{
    auto q = umul(v, u2);
    q = fast_add(q, {u1, u2});

    auto r1 = u1 - q[1] * d[1];

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

inline div_result<uint128> udivrem(uint128 x, uint128 y) noexcept
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

inline div_result<uint128> sdivrem(uint128 x, uint128 y) noexcept
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

inline uint128 operator/(uint128 x, uint128 y) noexcept
{
    return udivrem(x, y).quot;
}

inline uint128 operator%(uint128 x, uint128 y) noexcept
{
    return udivrem(x, y).rem;
}

inline uint128& operator/=(uint128& x, uint128 y) noexcept
{
    return x = x / y;
}

inline uint128& operator%=(uint128& x, uint128 y) noexcept
{
    return x = x % y;
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
    static constexpr float_denorm_style has_denorm = denorm_absent;
    static constexpr bool has_denorm_loss = false;
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

inline constexpr int from_dec_digit(char c)
{
    if (c < '0' || c > '9')
        throw_<std::invalid_argument>("invalid digit");
    return c - '0';
}

inline constexpr int from_hex_digit(char c)
{
    if (c >= 'a' && c <= 'f')
        return c - ('a' - 10);
    if (c >= 'A' && c <= 'F')
        return c - ('A' - 10);
    return from_dec_digit(c);
}

template <typename Int>
inline constexpr Int from_string(const char* str)
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
inline constexpr Int from_string(const std::string& s)
{
    return from_string<Int>(s.c_str());
}

inline constexpr uint128 operator""_u128(const char* s)
{
    return from_string<uint128>(s);
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
    std::reverse(s.begin(), s.end());
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
    template <unsigned M, typename = typename std::enable_if_t<(M < N)>>
    constexpr uint(const uint<M>& x) noexcept
    {
        for (size_t i = 0; i < uint<M>::num_words; ++i)
            words_[i] = x[i];
    }

    template <typename... T,
        typename = std::enable_if_t<std::conjunction_v<std::is_convertible<T, uint64_t>...>>>
    constexpr uint(T... v) noexcept : words_{static_cast<uint64_t>(v)...}
    {}

    constexpr uint64_t& operator[](size_t i) noexcept { return words_[i]; }

    constexpr const uint64_t& operator[](size_t i) const noexcept { return words_[i]; }

    constexpr explicit operator bool() const noexcept { return *this != uint{}; }

    template <unsigned M, typename = typename std::enable_if_t<(M < N)>>
    explicit operator uint<M>() const noexcept
    {
        uint<M> r;
        for (size_t i = 0; i < uint<M>::num_words; ++i)
            r[i] = words_[i];
        return r;
    }

    /// Explicit converting operator for all builtin integral types.
    template <typename Int, typename = typename std::enable_if_t<std::is_integral_v<Int>>>
    explicit operator Int() const noexcept
    {
        static_assert(sizeof(Int) <= sizeof(uint64_t));
        return static_cast<Int>(words_[0]);
    }

    friend inline constexpr uint operator+(const uint& x, const uint& y) noexcept
    {
        return addc(x, y).value;
    }

    inline constexpr uint& operator+=(const uint& y) noexcept { return *this = *this + y; }

    inline constexpr uint operator-() const noexcept { return ~*this + uint{1}; }

    friend inline constexpr uint operator-(const uint& x, const uint& y) noexcept
    {
        return subc(x, y).value;
    }

    inline constexpr uint& operator-=(const uint& y) noexcept { return *this = *this - y; }

    /// Multiplication implementation using word access
    /// and discarding the high part of the result product.
    friend inline constexpr uint operator*(const uint& x, const uint& y) noexcept
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

    inline constexpr uint& operator*=(const uint& y) noexcept { return *this = *this * y; }

    friend inline constexpr uint operator/(const uint& x, const uint& y) noexcept
    {
        return udivrem(x, y).quot;
    }

    friend inline constexpr uint operator%(const uint& x, const uint& y) noexcept
    {
        return udivrem(x, y).rem;
    }

    inline constexpr uint& operator/=(const uint& y) noexcept { return *this = *this / y; }

    inline constexpr uint& operator%=(const uint& y) noexcept { return *this = *this % y; }


    inline constexpr uint operator~() const noexcept
    {
        uint z;
        for (size_t i = 0; i < num_words; ++i)
            z[i] = ~words_[i];
        return z;
    }

    friend inline constexpr uint operator|(const uint& x, const uint& y) noexcept
    {
        uint z;
        for (size_t i = 0; i < num_words; ++i)
            z[i] = x[i] | y[i];
        return z;
    }

    inline constexpr uint& operator|=(const uint& y) noexcept { return *this = *this | y; }

    friend inline constexpr uint operator&(const uint& x, const uint& y) noexcept
    {
        uint z;
        for (size_t i = 0; i < num_words; ++i)
            z[i] = x[i] & y[i];
        return z;
    }

    inline constexpr uint& operator&=(const uint& y) noexcept { return *this = *this & y; }

    friend inline constexpr uint operator^(const uint& x, const uint& y) noexcept
    {
        uint z;
        for (size_t i = 0; i < num_words; ++i)
            z[i] = x[i] ^ y[i];
        return z;
    }

    inline constexpr uint& operator^=(const uint& y) noexcept { return *this = *this ^ y; }
};

using uint192 = uint<192>;
using uint256 = uint<256>;
using uint320 = uint<320>;
using uint384 = uint<384>;
using uint512 = uint<512>;

template <unsigned N>
inline constexpr bool operator==(const uint<N>& x, const uint<N>& y) noexcept
{
    uint64_t folded = 0;
    for (size_t i = 0; i < uint<N>::num_words; ++i)
        folded |= (x[i] ^ y[i]);
    return folded == 0;
}

template <unsigned N, typename T,
    typename = typename std::enable_if<std::is_convertible<T, uint<N>>::value>::type>
inline constexpr bool operator==(const uint<N>& x, const T& y) noexcept
{
    return x == uint<N>(y);
}

template <unsigned N, typename T,
    typename = typename std::enable_if<std::is_convertible<T, uint<N>>::value>::type>
inline constexpr bool operator==(const T& x, const uint<N>& y) noexcept
{
    return uint<N>(y) == x;
}


template <unsigned N>
inline constexpr bool operator!=(const uint<N>& x, const uint<N>& y) noexcept
{
    return !(x == y);
}

template <unsigned N, typename T,
    typename = typename std::enable_if<std::is_convertible<T, uint<N>>::value>::type>
inline constexpr bool operator!=(const uint<N>& x, const T& y) noexcept
{
    return x != uint<N>(y);
}

template <unsigned N, typename T,
    typename = typename std::enable_if<std::is_convertible<T, uint<N>>::value>::type>
inline constexpr bool operator!=(const T& x, const uint<N>& y) noexcept
{
    return uint<N>(x) != y;
}

#if !defined(_MSC_VER) || _MSC_VER < 1916  // This kills MSVC 2017 compiler.
inline constexpr bool operator<(const uint256& x, const uint256& y) noexcept
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
#endif

template <unsigned N>
inline constexpr bool operator<(const uint<N>& x, const uint<N>& y) noexcept
{
    return subc(x, y).carry;
}

template <unsigned N, typename T,
    typename = typename std::enable_if<std::is_convertible<T, uint<N>>::value>::type>
inline constexpr bool operator<(const uint<N>& x, const T& y) noexcept
{
    return x < uint<N>(y);
}

template <unsigned N, typename T,
    typename = typename std::enable_if<std::is_convertible<T, uint<N>>::value>::type>
inline constexpr bool operator<(const T& x, const uint<N>& y) noexcept
{
    return uint<N>(x) < y;
}


template <unsigned N>
inline constexpr bool operator>(const uint<N>& x, const uint<N>& y) noexcept
{
    return y < x;
}

template <unsigned N, typename T,
    typename = typename std::enable_if<std::is_convertible<T, uint<N>>::value>::type>
inline constexpr bool operator>(const uint<N>& x, const T& y) noexcept
{
    return x > uint<N>(y);
}

template <unsigned N, typename T,
    typename = typename std::enable_if<std::is_convertible<T, uint<N>>::value>::type>
inline constexpr bool operator>(const T& x, const uint<N>& y) noexcept
{
    return uint<N>(x) > y;
}


template <unsigned N>
inline constexpr bool operator>=(const uint<N>& x, const uint<N>& y) noexcept
{
    return !(x < y);
}

template <unsigned N, typename T,
    typename = typename std::enable_if<std::is_convertible<T, uint<N>>::value>::type>
inline constexpr bool operator>=(const uint<N>& x, const T& y) noexcept
{
    return x >= uint<N>(y);
}

template <unsigned N, typename T,
    typename = typename std::enable_if<std::is_convertible<T, uint<N>>::value>::type>
inline constexpr bool operator>=(const T& x, const uint<N>& y) noexcept
{
    return uint<N>(x) >= y;
}


template <unsigned N>
inline constexpr bool operator<=(const uint<N>& x, const uint<N>& y) noexcept
{
    return !(y < x);
}

template <unsigned N, typename T,
    typename = typename std::enable_if<std::is_convertible<T, uint<N>>::value>::type>
inline constexpr bool operator<=(const uint<N>& x, const T& y) noexcept
{
    return x <= uint<N>(y);
}

template <unsigned N, typename T,
    typename = typename std::enable_if<std::is_convertible<T, uint<N>>::value>::type>
inline constexpr bool operator<=(const T& x, const uint<N>& y) noexcept
{
    return uint<N>(x) <= y;
}

/// Signed less than comparison.
///
/// Interprets the arguments as two's complement signed integers
/// and checks the "less than" relation.
template <unsigned N>
inline constexpr bool slt(const uint<N>& x, const uint<N>& y) noexcept
{
    constexpr auto top_word_idx = uint<N>::num_words - 1;
    const auto x_neg = static_cast<int64_t>(x[top_word_idx]) < 0;
    const auto y_neg = static_cast<int64_t>(y[top_word_idx]) < 0;
    return ((x_neg ^ y_neg) != 0) ? x_neg : x < y;
}


inline constexpr uint256 operator<<(const uint256& x, uint64_t shift) noexcept
{
    if (INTX_UNLIKELY(shift >= uint256::num_bits))
        return 0;

    constexpr auto num_bits = uint256::num_bits;
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

template <unsigned N>
inline constexpr uint<N> operator<<(const uint<N>& x, uint64_t shift) noexcept
{
    if (INTX_UNLIKELY(shift >= uint<N>::num_bits))
        return 0;

    constexpr auto word_bits = sizeof(uint64_t) * 8;

    const auto s = shift % word_bits;
    const auto skip = static_cast<size_t>(shift / word_bits);

    uint<N> r;
    uint64_t carry = 0;
    for (size_t i = 0; i < (uint<N>::num_words - skip); ++i)
    {
        r[i + skip] = (x[i] << s) | carry;
        carry = (x[i] >> (word_bits - s - 1)) >> 1;
    }
    return r;
}


inline constexpr uint256 operator>>(const uint256& x, uint64_t shift) noexcept
{
    if (INTX_UNLIKELY(shift >= uint256::num_bits))
        return 0;

    constexpr auto num_bits = uint256::num_bits;
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

template <unsigned N>
inline constexpr uint<N> operator>>(const uint<N>& x, uint64_t shift) noexcept
{
    if (INTX_UNLIKELY(shift >= uint<N>::num_bits))
        return 0;

    constexpr auto num_words = uint<N>::num_words;
    constexpr auto word_bits = sizeof(uint64_t) * 8;

    const auto s = shift % word_bits;
    const auto skip = static_cast<size_t>(shift / word_bits);

    uint<N> r;
    uint64_t carry = 0;
    for (size_t i = 0; i < (num_words - skip); ++i)
    {
        r[num_words - 1 - i - skip] = (x[num_words - 1 - i] >> s) | carry;
        carry = (x[num_words - 1 - i] << (word_bits - s - 1)) << 1;
    }
    return r;
}

template <unsigned N>
inline constexpr uint<N> operator<<(const uint<N>& x, const uint<N>& shift) noexcept
{
    uint64_t high_words_fold = 0;
    for (size_t i = 1; i < uint<N>::num_words; ++i)
        high_words_fold |= shift[i];

    if (INTX_UNLIKELY(high_words_fold != 0))
        return 0;

    return x << shift[0];
}

template <unsigned N>
inline constexpr uint<N> operator>>(const uint<N>& x, const uint<N>& shift) noexcept
{
    uint64_t high_words_fold = 0;
    for (size_t i = 1; i < uint<N>::num_words; ++i)
        high_words_fold |= shift[i];

    if (INTX_UNLIKELY(high_words_fold != 0))
        return 0;

    return x >> shift[0];
}

template <unsigned N, typename T,
    typename = typename std::enable_if<std::is_convertible<T, uint<N>>::value>::type>
inline constexpr uint<N> operator<<(const uint<N>& x, const T& shift) noexcept
{
    if (shift < T{sizeof(x) * 8})
        return x << static_cast<uint64_t>(shift);
    return 0;
}

template <unsigned N, typename T,
    typename = typename std::enable_if<std::is_convertible<T, uint<N>>::value>::type>
inline constexpr uint<N> operator>>(const uint<N>& x, const T& shift) noexcept
{
    if (shift < T{sizeof(x) * 8})
        return x >> static_cast<uint64_t>(shift);
    return 0;
}

template <unsigned N>
inline constexpr uint<N>& operator>>=(uint<N>& x, uint64_t shift) noexcept
{
    return x = x >> shift;
}


inline constexpr uint64_t* as_words(uint128& x) noexcept
{
    return &x[0];
}

inline constexpr const uint64_t* as_words(const uint128& x) noexcept
{
    return &x[0];
}

template <unsigned N>
inline constexpr uint64_t* as_words(uint<N>& x) noexcept
{
    return &x[0];
}

template <unsigned N>
inline constexpr const uint64_t* as_words(const uint<N>& x) noexcept
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
inline constexpr uint<2 * N> umul(const uint<N>& x, const uint<N>& y) noexcept
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
inline constexpr uint<N> exp(uint<N> base, uint<N> exponent) noexcept
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
inline constexpr unsigned count_significant_words(const uint<N>& x) noexcept
{
    for (size_t i = uint<N>::num_words; i > 0; --i)
    {
        if (x[i - 1] != 0)
            return static_cast<unsigned>(i);
    }
    return 0;
}

inline constexpr unsigned count_significant_bytes(uint64_t x) noexcept
{
    return (64 - clz(x) + 7) / 8;
}

template <unsigned N>
inline constexpr unsigned count_significant_bytes(const uint<N>& x) noexcept
{
    const auto w = count_significant_words(x);
    return (w != 0) ? count_significant_bytes(x[w - 1]) + (w - 1) * 8 : 0;
}

template <unsigned N>
inline constexpr unsigned clz(const uint<N>& x) noexcept
{
    constexpr unsigned num_words = uint<N>::num_words;
    const auto s = count_significant_words(x);
    if (s == 0)
        return num_words * 64;
    return clz(x[s - 1]) + (num_words - s) * 64;
}

namespace internal
{
/// Counts the number of zero leading bits in nonzero argument x.
inline constexpr unsigned clz_nonzero(uint64_t x) noexcept
{
    INTX_REQUIRE(x != 0);
#ifdef _MSC_VER
    return clz_generic(x);
#else
    return unsigned(__builtin_clzll(x));
#endif
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
[[gnu::always_inline]] inline normalized_div_args<M, N> normalize(
    const uint<M>& numerator, const uint<N>& denominator) noexcept
{
    static constexpr auto num_numerator_words = uint<M>::num_words;
    static constexpr auto num_denominator_words = uint<N>::num_words;

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

    // Skip the highest word of numerator if not significant.
    if (un[m] != 0 || un[m - 1] >= vn[n - 1])
        ++m;

    return na;
}

/// Divides arbitrary long unsigned integer by 64-bit unsigned integer (1 word).
/// @param u    The array of a normalized numerator words. It will contain
///             the quotient after execution.
/// @param len  The number of numerator words.
/// @param d    The normalized divisor.
/// @return     The remainder.
inline uint64_t udivrem_by1(uint64_t u[], int len, uint64_t d) noexcept
{
    INTX_REQUIRE(len >= 2);

    const auto reciprocal = reciprocal_2by1(d);

    auto rem = u[len - 1];  // Set the top word as remainder.
    u[len - 1] = 0;         // Reset the word being a part of the result quotient.

    auto it = &u[len - 2];
    do
    {
        std::tie(*it, rem) = udivrem_2by1({*it, rem}, d, reciprocal);
    } while (it-- != &u[0]);

    return rem;
}

/// Divides arbitrary long unsigned integer by 128-bit unsigned integer (2 words).
/// @param u    The array of a normalized numerator words. It will contain the
///             quotient after execution.
/// @param len  The number of numerator words.
/// @param d    The normalized divisor.
/// @return     The remainder.
inline uint128 udivrem_by2(uint64_t u[], int len, uint128 d) noexcept
{
    INTX_REQUIRE(len >= 3);

    const auto reciprocal = reciprocal_3by2(d);

    auto rem = uint128{u[len - 2], u[len - 1]};  // Set the 2 top words as remainder.
    u[len - 1] = u[len - 2] = 0;  // Reset these words being a part of the result quotient.

    auto it = &u[len - 3];
    do
    {
        std::tie(*it, rem) = udivrem_3by2(rem[1], rem[0], *it, d, reciprocal);
    } while (it-- != &u[0]);

    return rem;
}

/// s = x + y.
inline bool add(uint64_t s[], const uint64_t x[], const uint64_t y[], int len) noexcept
{
    // OPT: Add MinLen template parameter and unroll first loop iterations.
    INTX_REQUIRE(len >= 2);

    bool carry = false;
    for (int i = 0; i < len; ++i)
        std::tie(s[i], carry) = addc(x[i], y[i], carry);
    return carry;
}

/// r = x - multiplier * y.
inline uint64_t submul(
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

inline void udivrem_knuth(
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

    if (na.num_numerator_words <= na.num_divisor_words)
        return {0, static_cast<uint<N>>(u)};

    if (na.num_divisor_words == 1)
    {
        const auto r = internal::udivrem_by1(
            as_words(na.numerator), na.num_numerator_words, as_words(na.divisor)[0]);
        return {static_cast<uint<M>>(na.numerator), r >> na.shift};
    }

    if (na.num_divisor_words == 2)
    {
        const auto d = as_words(na.divisor);
        const auto r =
            internal::udivrem_by2(as_words(na.numerator), na.num_numerator_words, {d[0], d[1]});
        return {static_cast<uint<M>>(na.numerator), r >> na.shift};
    }

    auto un = as_words(na.numerator);  // Will be modified.

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
inline constexpr div_result<uint<N>> sdivrem(const uint<N>& u, const uint<N>& v) noexcept
{
    const auto sign_mask = uint<N>{1} << (sizeof(u) * 8 - 1);
    auto u_is_neg = (u & sign_mask) != 0;
    auto v_is_neg = (v & sign_mask) != 0;

    auto u_abs = u_is_neg ? -u : u;
    auto v_abs = v_is_neg ? -v : v;

    auto q_is_neg = u_is_neg ^ v_is_neg;

    auto res = udivrem(u_abs, v_abs);

    return {q_is_neg ? -res.quot : res.quot, u_is_neg ? -res.rem : res.rem};
}

inline constexpr uint256 bswap(const uint256& x) noexcept
{
    return {bswap(x[3]), bswap(x[2]), bswap(x[1]), bswap(x[0])};
}

template <unsigned N>
inline constexpr uint<N> bswap(const uint<N>& x) noexcept
{
    constexpr auto num_words = uint<N>::num_words;
    uint<N> z;
    for (size_t i = 0; i < num_words; ++i)
        z[num_words - 1 - i] = bswap(x[i]);
    return z;
}


// Support for type conversions for binary operators.

template <unsigned N, typename T,
    typename = typename std::enable_if<std::is_convertible<T, uint<N>>::value>::type>
inline constexpr uint<N>& operator<<=(uint<N>& x, const T& y) noexcept
{
    return x = x << y;
}

template <unsigned N, typename T,
    typename = typename std::enable_if<std::is_convertible<T, uint<N>>::value>::type>
inline constexpr uint<N>& operator>>=(uint<N>& x, const T& y) noexcept
{
    return x = x >> y;
}


inline uint256 addmod(const uint256& x, const uint256& y, const uint256& mod) noexcept
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

inline uint256 mulmod(const uint256& x, const uint256& y, const uint256& mod) noexcept
{
    return udivrem(umul(x, y), mod).rem;
}


inline constexpr uint256 operator"" _u256(const char* s)
{
    return from_string<uint256>(s);
}

inline constexpr uint512 operator"" _u512(const char* s)
{
    return from_string<uint512>(s);
}


/// Convert native representation to/from little-endian byte order.
/// intx and built-in integral types are supported.
template <typename T>
inline constexpr T to_little_endian(const T& x) noexcept
{
    if constexpr (byte_order_is_little_endian)
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
inline constexpr T to_big_endian(const T& x) noexcept
{
    if constexpr (byte_order_is_little_endian)
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
/// that the provided buffer is big enough. Therefore marked "unsafe".
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
/// that the provided buffer is big enough to fit the value. Therefore marked "unsafe".
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
