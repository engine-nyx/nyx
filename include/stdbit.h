/*
 * stdbit.h -- portable single-header drop-in replacement for the
 * C23 standard header <stdbit.h>.
 *
 * Implements (for unsigned char / unsigned short / unsigned int /
 * unsigned long / unsigned long long, and therefore transparently
 * for uint8_t/uint16_t/uint32_t/uint64_t, which are typedefs of one
 * of those types on virtually every platform):
 *
 *   stdc_leading_zeros        stdc_leading_ones
 *   stdc_trailing_zeros       stdc_trailing_ones
 *   stdc_first_leading_zero   stdc_first_leading_one
 *   stdc_first_trailing_zero  stdc_first_trailing_one
 *   stdc_count_zeros          stdc_count_ones
 *   stdc_has_single_bit
 *   stdc_bit_width
 *   stdc_bit_floor            stdc_bit_ceil
 *
 * Each has type-suffixed variants (_uc, _us, _ui, _ul, _ull) plus a
 * type-generic C11 _Generic macro with the bare name, exactly like
 * the real header. Endianness macros are provided too.
 *
 * On GCC and Clang, every operation is implemented as a macro that
 * expands directly to a compiler builtin (__builtin_clz/ctz/popcount
 * and friends) wrapped in a GNU statement expression. This is a
 * deliberate design choice: macros and builtins have no linkage, so
 * they can safely be called from plain `inline` (external-linkage)
 * functions -- e.g. `inline square lsb(bitboard bb) { return
 * stdc_first_trailing_one(bb) - 1; }` -- without ever tripping
 * Clang's -Wstatic-in-inline. (An earlier revision of this header
 * used `static inline` helper functions for this, which is exactly
 * the pattern that warning exists to catch: a static-linkage
 * function referenced from a function that may get an out-of-line,
 * externally-linked copy emitted in another translation unit.)
 *
 * On any other compiler, a portable loop-based `static inline`
 * function implementation is used instead (no builtins required).
 * That fallback is not affected by -Wstatic-in-inline since that
 * diagnostic is Clang-specific.
 *
 * Drop this file next to your sources (or in an include path) and
 * `#include "stdbit.h"`.
 */

#ifndef STDBIT_REPLACEMENT_H_INCLUDED
#define STDBIT_REPLACEMENT_H_INCLUDED

#include <limits.h>
#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/* Endianness                                                          */
/* ------------------------------------------------------------------ */

#define __STDC_ENDIAN_LITTLE__ 1234
#define __STDC_ENDIAN_BIG__    4321

#if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && \
    (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#  define __STDC_ENDIAN_NATIVE__ __STDC_ENDIAN_BIG__
#elif defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && \
    (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#  define __STDC_ENDIAN_NATIVE__ __STDC_ENDIAN_LITTLE__
#else
#  if defined(STDBIT_FORCE_BIG_ENDIAN)
#    define __STDC_ENDIAN_NATIVE__ __STDC_ENDIAN_BIG__
#  else
#    define __STDC_ENDIAN_NATIVE__ __STDC_ENDIAN_LITTLE__
#  endif
#endif

#define __STDC_VERSION_STDBIT_H__ 202311L

/* ------------------------------------------------------------------ */
/* Per-type implementations                                            */

#if defined(__GNUC__) || defined(__clang__)
/* Builtin/macro implementation: nothing here has linkage, so it can be
   freely called from plain `inline` (external-linkage) functions without
   tripping -Wstatic-in-inline. Each op uses its OWN uniquely-named local
   inside its statement expression (stdbit_lz_, stdbit_tz_, stdbit_flz_,
   ...) rather than a shared name, because composed ops (e.g.
   first_trailing_one -> trailing_zeros) nest one statement expression
   inside another; reusing a name across that nesting would make the
   inner declaration shadow the outer one under -Wshadow. */

/* ---- unsigned char ---- */
#define stdc_leading_zeros_uc(x) __extension__ ({ \
    unsigned char stdbit_lz_ = (unsigned char)(x); \
    (unsigned int)(stdbit_lz_ == 0 ? (unsigned int)(sizeof(unsigned char) * CHAR_BIT) \
        : (unsigned int)(__builtin_clz(stdbit_lz_) - (int)((unsigned)(sizeof(unsigned int)*CHAR_BIT) - sizeof(unsigned char) * CHAR_BIT))); \
})
#define stdc_trailing_zeros_uc(x) __extension__ ({ \
    unsigned char stdbit_tz_ = (unsigned char)(x); \
    (unsigned int)(stdbit_tz_ == 0 ? (unsigned int)(sizeof(unsigned char) * CHAR_BIT) \
        : (unsigned int)__builtin_ctz(stdbit_tz_)); \
})
#define stdc_leading_ones_uc(x) stdc_leading_zeros_uc((unsigned char)~(unsigned char)(x))
#define stdc_trailing_ones_uc(x) stdc_trailing_zeros_uc((unsigned char)~(unsigned char)(x))
#define stdc_count_ones_uc(x) ((unsigned int)__builtin_popcount((unsigned char)(x)))
#define stdc_count_zeros_uc(x) ((unsigned int)(sizeof(unsigned char) * CHAR_BIT) - stdc_count_ones_uc(x))
#define stdc_first_leading_zero_uc(x) __extension__ ({ \
    unsigned char stdbit_flz_ = (unsigned char)(x); \
    unsigned char stdbit_flz_ao_ = (unsigned char)~(unsigned char)0; \
    (unsigned int)(stdbit_flz_ == stdbit_flz_ao_ ? 0u : stdc_leading_ones_uc(stdbit_flz_) + 1u); \
})
#define stdc_first_leading_one_uc(x) __extension__ ({ \
    unsigned char stdbit_flo_ = (unsigned char)(x); \
    (unsigned int)(stdbit_flo_ == 0 ? 0u : stdc_leading_zeros_uc(stdbit_flo_) + 1u); \
})
#define stdc_first_trailing_zero_uc(x) __extension__ ({ \
    unsigned char stdbit_ftz_ = (unsigned char)(x); \
    unsigned char stdbit_ftz_ao_ = (unsigned char)~(unsigned char)0; \
    (unsigned int)(stdbit_ftz_ == stdbit_ftz_ao_ ? 0u : stdc_trailing_ones_uc(stdbit_ftz_) + 1u); \
})
#define stdc_first_trailing_one_uc(x) __extension__ ({ \
    unsigned char stdbit_fto_ = (unsigned char)(x); \
    (unsigned int)(stdbit_fto_ == 0 ? 0u : stdc_trailing_zeros_uc(stdbit_fto_) + 1u); \
})
#define stdc_has_single_bit_uc(x) __extension__ ({ \
    unsigned char stdbit_hsb_ = (unsigned char)(x); \
    (int)(stdbit_hsb_ != 0 && (unsigned char)(stdbit_hsb_ & (unsigned char)(stdbit_hsb_ - 1)) == 0); \
})
#define stdc_bit_width_uc(x) ((unsigned int)(sizeof(unsigned char) * CHAR_BIT) - stdc_leading_zeros_uc(x))
#define stdc_bit_floor_uc(x) __extension__ ({ \
    unsigned char stdbit_bf_ = (unsigned char)(x); \
    (stdbit_bf_ == 0 ? (unsigned char)0 : (unsigned char)((unsigned char)1 << (stdc_bit_width_uc(stdbit_bf_) - 1))); \
})
#define stdc_bit_ceil_uc(x) __extension__ ({ \
    unsigned char stdbit_bc_ = (unsigned char)(x); \
    (stdbit_bc_ <= 1 ? (unsigned char)1 : (unsigned char)((unsigned char)1 << stdc_bit_width_uc((unsigned char)(stdbit_bc_ - 1)))); \
})

/* ---- unsigned short ---- */
#define stdc_leading_zeros_us(x) __extension__ ({ \
    unsigned short stdbit_lz_ = (unsigned short)(x); \
    (unsigned int)(stdbit_lz_ == 0 ? (unsigned int)(sizeof(unsigned short) * CHAR_BIT) \
        : (unsigned int)(__builtin_clz(stdbit_lz_) - (int)((unsigned)(sizeof(unsigned int)*CHAR_BIT) - sizeof(unsigned short) * CHAR_BIT))); \
})
#define stdc_trailing_zeros_us(x) __extension__ ({ \
    unsigned short stdbit_tz_ = (unsigned short)(x); \
    (unsigned int)(stdbit_tz_ == 0 ? (unsigned int)(sizeof(unsigned short) * CHAR_BIT) \
        : (unsigned int)__builtin_ctz(stdbit_tz_)); \
})
#define stdc_leading_ones_us(x) stdc_leading_zeros_us((unsigned short)~(unsigned short)(x))
#define stdc_trailing_ones_us(x) stdc_trailing_zeros_us((unsigned short)~(unsigned short)(x))
#define stdc_count_ones_us(x) ((unsigned int)__builtin_popcount((unsigned short)(x)))
#define stdc_count_zeros_us(x) ((unsigned int)(sizeof(unsigned short) * CHAR_BIT) - stdc_count_ones_us(x))
#define stdc_first_leading_zero_us(x) __extension__ ({ \
    unsigned short stdbit_flz_ = (unsigned short)(x); \
    unsigned short stdbit_flz_ao_ = (unsigned short)~(unsigned short)0; \
    (unsigned int)(stdbit_flz_ == stdbit_flz_ao_ ? 0u : stdc_leading_ones_us(stdbit_flz_) + 1u); \
})
#define stdc_first_leading_one_us(x) __extension__ ({ \
    unsigned short stdbit_flo_ = (unsigned short)(x); \
    (unsigned int)(stdbit_flo_ == 0 ? 0u : stdc_leading_zeros_us(stdbit_flo_) + 1u); \
})
#define stdc_first_trailing_zero_us(x) __extension__ ({ \
    unsigned short stdbit_ftz_ = (unsigned short)(x); \
    unsigned short stdbit_ftz_ao_ = (unsigned short)~(unsigned short)0; \
    (unsigned int)(stdbit_ftz_ == stdbit_ftz_ao_ ? 0u : stdc_trailing_ones_us(stdbit_ftz_) + 1u); \
})
#define stdc_first_trailing_one_us(x) __extension__ ({ \
    unsigned short stdbit_fto_ = (unsigned short)(x); \
    (unsigned int)(stdbit_fto_ == 0 ? 0u : stdc_trailing_zeros_us(stdbit_fto_) + 1u); \
})
#define stdc_has_single_bit_us(x) __extension__ ({ \
    unsigned short stdbit_hsb_ = (unsigned short)(x); \
    (int)(stdbit_hsb_ != 0 && (unsigned short)(stdbit_hsb_ & (unsigned short)(stdbit_hsb_ - 1)) == 0); \
})
#define stdc_bit_width_us(x) ((unsigned int)(sizeof(unsigned short) * CHAR_BIT) - stdc_leading_zeros_us(x))
#define stdc_bit_floor_us(x) __extension__ ({ \
    unsigned short stdbit_bf_ = (unsigned short)(x); \
    (stdbit_bf_ == 0 ? (unsigned short)0 : (unsigned short)((unsigned short)1 << (stdc_bit_width_us(stdbit_bf_) - 1))); \
})
#define stdc_bit_ceil_us(x) __extension__ ({ \
    unsigned short stdbit_bc_ = (unsigned short)(x); \
    (stdbit_bc_ <= 1 ? (unsigned short)1 : (unsigned short)((unsigned short)1 << stdc_bit_width_us((unsigned short)(stdbit_bc_ - 1)))); \
})

/* ---- unsigned int ---- */
#define stdc_leading_zeros_ui(x) __extension__ ({ \
    unsigned int stdbit_lz_ = (unsigned int)(x); \
    (unsigned int)(stdbit_lz_ == 0 ? (unsigned int)(sizeof(unsigned int) * CHAR_BIT) \
        : (unsigned int)(__builtin_clz(stdbit_lz_) - (int)((unsigned)(sizeof(unsigned int)*CHAR_BIT) - sizeof(unsigned int) * CHAR_BIT))); \
})
#define stdc_trailing_zeros_ui(x) __extension__ ({ \
    unsigned int stdbit_tz_ = (unsigned int)(x); \
    (unsigned int)(stdbit_tz_ == 0 ? (unsigned int)(sizeof(unsigned int) * CHAR_BIT) \
        : (unsigned int)__builtin_ctz(stdbit_tz_)); \
})
#define stdc_leading_ones_ui(x) stdc_leading_zeros_ui((unsigned int)~(unsigned int)(x))
#define stdc_trailing_ones_ui(x) stdc_trailing_zeros_ui((unsigned int)~(unsigned int)(x))
#define stdc_count_ones_ui(x) ((unsigned int)__builtin_popcount((unsigned int)(x)))
#define stdc_count_zeros_ui(x) ((unsigned int)(sizeof(unsigned int) * CHAR_BIT) - stdc_count_ones_ui(x))
#define stdc_first_leading_zero_ui(x) __extension__ ({ \
    unsigned int stdbit_flz_ = (unsigned int)(x); \
    unsigned int stdbit_flz_ao_ = (unsigned int)~(unsigned int)0; \
    (unsigned int)(stdbit_flz_ == stdbit_flz_ao_ ? 0u : stdc_leading_ones_ui(stdbit_flz_) + 1u); \
})
#define stdc_first_leading_one_ui(x) __extension__ ({ \
    unsigned int stdbit_flo_ = (unsigned int)(x); \
    (unsigned int)(stdbit_flo_ == 0 ? 0u : stdc_leading_zeros_ui(stdbit_flo_) + 1u); \
})
#define stdc_first_trailing_zero_ui(x) __extension__ ({ \
    unsigned int stdbit_ftz_ = (unsigned int)(x); \
    unsigned int stdbit_ftz_ao_ = (unsigned int)~(unsigned int)0; \
    (unsigned int)(stdbit_ftz_ == stdbit_ftz_ao_ ? 0u : stdc_trailing_ones_ui(stdbit_ftz_) + 1u); \
})
#define stdc_first_trailing_one_ui(x) __extension__ ({ \
    unsigned int stdbit_fto_ = (unsigned int)(x); \
    (unsigned int)(stdbit_fto_ == 0 ? 0u : stdc_trailing_zeros_ui(stdbit_fto_) + 1u); \
})
#define stdc_has_single_bit_ui(x) __extension__ ({ \
    unsigned int stdbit_hsb_ = (unsigned int)(x); \
    (int)(stdbit_hsb_ != 0 && (unsigned int)(stdbit_hsb_ & (unsigned int)(stdbit_hsb_ - 1)) == 0); \
})
#define stdc_bit_width_ui(x) ((unsigned int)(sizeof(unsigned int) * CHAR_BIT) - stdc_leading_zeros_ui(x))
#define stdc_bit_floor_ui(x) __extension__ ({ \
    unsigned int stdbit_bf_ = (unsigned int)(x); \
    (stdbit_bf_ == 0 ? (unsigned int)0 : (unsigned int)((unsigned int)1 << (stdc_bit_width_ui(stdbit_bf_) - 1))); \
})
#define stdc_bit_ceil_ui(x) __extension__ ({ \
    unsigned int stdbit_bc_ = (unsigned int)(x); \
    (stdbit_bc_ <= 1 ? (unsigned int)1 : (unsigned int)((unsigned int)1 << stdc_bit_width_ui((unsigned int)(stdbit_bc_ - 1)))); \
})

/* ---- unsigned long ---- */
#define stdc_leading_zeros_ul(x) __extension__ ({ \
    unsigned long stdbit_lz_ = (unsigned long)(x); \
    (unsigned int)(stdbit_lz_ == 0 ? (unsigned int)(sizeof(unsigned long) * CHAR_BIT) \
        : (unsigned int)(__builtin_clzl(stdbit_lz_) - (int)((unsigned)(sizeof(unsigned long)*CHAR_BIT) - sizeof(unsigned long) * CHAR_BIT))); \
})
#define stdc_trailing_zeros_ul(x) __extension__ ({ \
    unsigned long stdbit_tz_ = (unsigned long)(x); \
    (unsigned int)(stdbit_tz_ == 0 ? (unsigned int)(sizeof(unsigned long) * CHAR_BIT) \
        : (unsigned int)__builtin_ctzl(stdbit_tz_)); \
})
#define stdc_leading_ones_ul(x) stdc_leading_zeros_ul((unsigned long)~(unsigned long)(x))
#define stdc_trailing_ones_ul(x) stdc_trailing_zeros_ul((unsigned long)~(unsigned long)(x))
#define stdc_count_ones_ul(x) ((unsigned int)__builtin_popcountl((unsigned long)(x)))
#define stdc_count_zeros_ul(x) ((unsigned int)(sizeof(unsigned long) * CHAR_BIT) - stdc_count_ones_ul(x))
#define stdc_first_leading_zero_ul(x) __extension__ ({ \
    unsigned long stdbit_flz_ = (unsigned long)(x); \
    unsigned long stdbit_flz_ao_ = (unsigned long)~(unsigned long)0; \
    (unsigned int)(stdbit_flz_ == stdbit_flz_ao_ ? 0u : stdc_leading_ones_ul(stdbit_flz_) + 1u); \
})
#define stdc_first_leading_one_ul(x) __extension__ ({ \
    unsigned long stdbit_flo_ = (unsigned long)(x); \
    (unsigned int)(stdbit_flo_ == 0 ? 0u : stdc_leading_zeros_ul(stdbit_flo_) + 1u); \
})
#define stdc_first_trailing_zero_ul(x) __extension__ ({ \
    unsigned long stdbit_ftz_ = (unsigned long)(x); \
    unsigned long stdbit_ftz_ao_ = (unsigned long)~(unsigned long)0; \
    (unsigned int)(stdbit_ftz_ == stdbit_ftz_ao_ ? 0u : stdc_trailing_ones_ul(stdbit_ftz_) + 1u); \
})
#define stdc_first_trailing_one_ul(x) __extension__ ({ \
    unsigned long stdbit_fto_ = (unsigned long)(x); \
    (unsigned int)(stdbit_fto_ == 0 ? 0u : stdc_trailing_zeros_ul(stdbit_fto_) + 1u); \
})
#define stdc_has_single_bit_ul(x) __extension__ ({ \
    unsigned long stdbit_hsb_ = (unsigned long)(x); \
    (int)(stdbit_hsb_ != 0 && (unsigned long)(stdbit_hsb_ & (unsigned long)(stdbit_hsb_ - 1)) == 0); \
})
#define stdc_bit_width_ul(x) ((unsigned int)(sizeof(unsigned long) * CHAR_BIT) - stdc_leading_zeros_ul(x))
#define stdc_bit_floor_ul(x) __extension__ ({ \
    unsigned long stdbit_bf_ = (unsigned long)(x); \
    (stdbit_bf_ == 0 ? (unsigned long)0 : (unsigned long)((unsigned long)1 << (stdc_bit_width_ul(stdbit_bf_) - 1))); \
})
#define stdc_bit_ceil_ul(x) __extension__ ({ \
    unsigned long stdbit_bc_ = (unsigned long)(x); \
    (stdbit_bc_ <= 1 ? (unsigned long)1 : (unsigned long)((unsigned long)1 << stdc_bit_width_ul((unsigned long)(stdbit_bc_ - 1)))); \
})

/* ---- unsigned long long ---- */
#define stdc_leading_zeros_ull(x) __extension__ ({ \
    unsigned long long stdbit_lz_ = (unsigned long long)(x); \
    (unsigned int)(stdbit_lz_ == 0 ? (unsigned int)(sizeof(unsigned long long) * CHAR_BIT) \
        : (unsigned int)(__builtin_clzll(stdbit_lz_) - (int)((unsigned)(sizeof(unsigned long long)*CHAR_BIT) - sizeof(unsigned long long) * CHAR_BIT))); \
})
#define stdc_trailing_zeros_ull(x) __extension__ ({ \
    unsigned long long stdbit_tz_ = (unsigned long long)(x); \
    (unsigned int)(stdbit_tz_ == 0 ? (unsigned int)(sizeof(unsigned long long) * CHAR_BIT) \
        : (unsigned int)__builtin_ctzll(stdbit_tz_)); \
})
#define stdc_leading_ones_ull(x) stdc_leading_zeros_ull((unsigned long long)~(unsigned long long)(x))
#define stdc_trailing_ones_ull(x) stdc_trailing_zeros_ull((unsigned long long)~(unsigned long long)(x))
#define stdc_count_ones_ull(x) ((unsigned int)__builtin_popcountll((unsigned long long)(x)))
#define stdc_count_zeros_ull(x) ((unsigned int)(sizeof(unsigned long long) * CHAR_BIT) - stdc_count_ones_ull(x))
#define stdc_first_leading_zero_ull(x) __extension__ ({ \
    unsigned long long stdbit_flz_ = (unsigned long long)(x); \
    unsigned long long stdbit_flz_ao_ = (unsigned long long)~(unsigned long long)0; \
    (unsigned int)(stdbit_flz_ == stdbit_flz_ao_ ? 0u : stdc_leading_ones_ull(stdbit_flz_) + 1u); \
})
#define stdc_first_leading_one_ull(x) __extension__ ({ \
    unsigned long long stdbit_flo_ = (unsigned long long)(x); \
    (unsigned int)(stdbit_flo_ == 0 ? 0u : stdc_leading_zeros_ull(stdbit_flo_) + 1u); \
})
#define stdc_first_trailing_zero_ull(x) __extension__ ({ \
    unsigned long long stdbit_ftz_ = (unsigned long long)(x); \
    unsigned long long stdbit_ftz_ao_ = (unsigned long long)~(unsigned long long)0; \
    (unsigned int)(stdbit_ftz_ == stdbit_ftz_ao_ ? 0u : stdc_trailing_ones_ull(stdbit_ftz_) + 1u); \
})
#define stdc_first_trailing_one_ull(x) __extension__ ({ \
    unsigned long long stdbit_fto_ = (unsigned long long)(x); \
    (unsigned int)(stdbit_fto_ == 0 ? 0u : stdc_trailing_zeros_ull(stdbit_fto_) + 1u); \
})
#define stdc_has_single_bit_ull(x) __extension__ ({ \
    unsigned long long stdbit_hsb_ = (unsigned long long)(x); \
    (int)(stdbit_hsb_ != 0 && (unsigned long long)(stdbit_hsb_ & (unsigned long long)(stdbit_hsb_ - 1)) == 0); \
})
#define stdc_bit_width_ull(x) ((unsigned int)(sizeof(unsigned long long) * CHAR_BIT) - stdc_leading_zeros_ull(x))
#define stdc_bit_floor_ull(x) __extension__ ({ \
    unsigned long long stdbit_bf_ = (unsigned long long)(x); \
    (stdbit_bf_ == 0 ? (unsigned long long)0 : (unsigned long long)((unsigned long long)1 << (stdc_bit_width_ull(stdbit_bf_) - 1))); \
})
#define stdc_bit_ceil_ull(x) __extension__ ({ \
    unsigned long long stdbit_bc_ = (unsigned long long)(x); \
    (stdbit_bc_ <= 1 ? (unsigned long long)1 : (unsigned long long)((unsigned long long)1 << stdc_bit_width_ull((unsigned long long)(stdbit_bc_ - 1)))); \
})

#else /* !(__GNUC__ || __clang__): portable static-inline fallback */


#define STDBIT_DEFINE(TYPE, SUF)                                            \
static inline unsigned int stdc_leading_zeros_##SUF(TYPE x) {               \
    const unsigned int bits = (unsigned int)(sizeof(TYPE) * CHAR_BIT);      \
    if (x == 0) return bits;                                                \
    unsigned int n = 0;                                                     \
    TYPE mask = (TYPE)((TYPE)1 << (bits - 1));                              \
    while ((TYPE)(x & mask) == 0) { n++; mask = (TYPE)(mask >> 1); }        \
    return n;                                                               \
}                                                                            \
static inline unsigned int stdc_leading_ones_##SUF(TYPE x) {                \
    return stdc_leading_zeros_##SUF((TYPE)(~x));                            \
}                                                                            \
static inline unsigned int stdc_trailing_zeros_##SUF(TYPE x) {              \
    const unsigned int bits = (unsigned int)(sizeof(TYPE) * CHAR_BIT);      \
    if (x == 0) return bits;                                                \
    unsigned int n = 0;                                                     \
    TYPE mask = (TYPE)1;                                                    \
    while ((TYPE)(x & mask) == 0) { n++; mask = (TYPE)(mask << 1); }        \
    return n;                                                               \
}                                                                            \
static inline unsigned int stdc_trailing_ones_##SUF(TYPE x) {               \
    return stdc_trailing_zeros_##SUF((TYPE)(~x));                           \
}                                                                            \
static inline unsigned int stdc_first_leading_zero_##SUF(TYPE x) {          \
    TYPE all_ones = (TYPE)(~(TYPE)0);                                       \
    if (x == all_ones) return 0u;                                          \
    return stdc_leading_ones_##SUF(x) + 1u;                                 \
}                                                                            \
static inline unsigned int stdc_first_leading_one_##SUF(TYPE x) {           \
    if (x == 0) return 0u;                                                  \
    return stdc_leading_zeros_##SUF(x) + 1u;                                \
}                                                                            \
static inline unsigned int stdc_first_trailing_zero_##SUF(TYPE x) {         \
    TYPE all_ones = (TYPE)(~(TYPE)0);                                       \
    if (x == all_ones) return 0u;                                          \
    return stdc_trailing_ones_##SUF(x) + 1u;                                \
}                                                                            \
static inline unsigned int stdc_first_trailing_one_##SUF(TYPE x) {          \
    if (x == 0) return 0u;                                                  \
    return stdc_trailing_zeros_##SUF(x) + 1u;                               \
}                                                                            \
static inline unsigned int stdc_count_ones_##SUF(TYPE x) {                  \
    unsigned int c = 0;                                                     \
    while (x) { x = (TYPE)(x & (TYPE)(x - 1)); c++; }                       \
    return c;                                                               \
}                                                                            \
static inline unsigned int stdc_count_zeros_##SUF(TYPE x) {                 \
    const unsigned int bits = (unsigned int)(sizeof(TYPE) * CHAR_BIT);      \
    return bits - stdc_count_ones_##SUF(x);                                 \
}                                                                            \
static inline int stdc_has_single_bit_##SUF(TYPE x) {                       \
    return x != 0 && (TYPE)(x & (TYPE)(x - 1)) == 0;                        \
}                                                                            \
static inline unsigned int stdc_bit_width_##SUF(TYPE x) {                   \
    const unsigned int bits = (unsigned int)(sizeof(TYPE) * CHAR_BIT);      \
    return bits - stdc_leading_zeros_##SUF(x);                              \
}                                                                            \
static inline TYPE stdc_bit_floor_##SUF(TYPE x) {                           \
    if (x == 0) return (TYPE)0;                                             \
    unsigned int w = stdc_bit_width_##SUF(x);                               \
    return (TYPE)((TYPE)1 << (w - 1));                                      \
}                                                                            \
static inline TYPE stdc_bit_ceil_##SUF(TYPE x) {                            \
    if (x <= 1) return (TYPE)1;                                             \
    unsigned int w = stdc_bit_width_##SUF((TYPE)(x - 1));                   \
    return (TYPE)((TYPE)1 << w);                                            \
}

STDBIT_DEFINE(unsigned char,      uc)
STDBIT_DEFINE(unsigned short,     us)
STDBIT_DEFINE(unsigned int,       ui)
STDBIT_DEFINE(unsigned long,      ul)
STDBIT_DEFINE(unsigned long long, ull)

#undef STDBIT_DEFINE

#endif /* __GNUC__ || __clang__ */

/* ------------------------------------------------------------------ */
/* Type-generic macros (C11 _Generic, matches uint8/16/32/64_t too     */
/* since they are typedefs of one of the five base types above)        */
/* ------------------------------------------------------------------ */

#if !defined(__cplusplus) && (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L)

/* NOTE: on GCC/Clang the NAME##_SUF tokens are function-like macros,
 * not addressable functions, so each branch must embed the call
 * itself (NAME##_uc(x), not NAME##_uc) rather than selecting a bare
 * function and calling it afterwards. _Generic only evaluates the
 * chosen branch; the other four just need to type-check, and since
 * every per-type macro casts its argument explicitly, converting x
 * to each of the five types is always well-formed. */
#define STDBIT_GENERIC(x, NAME) _Generic((x),                     \
    unsigned char:      NAME##_uc(x),                             \
    unsigned short:      NAME##_us(x),                            \
    unsigned int:        NAME##_ui(x),                            \
    unsigned long:       NAME##_ul(x),                            \
    unsigned long long:  NAME##_ull(x)                            \
)

#define stdc_leading_zeros(x)       STDBIT_GENERIC(x, stdc_leading_zeros)
#define stdc_leading_ones(x)        STDBIT_GENERIC(x, stdc_leading_ones)
#define stdc_trailing_zeros(x)      STDBIT_GENERIC(x, stdc_trailing_zeros)
#define stdc_trailing_ones(x)       STDBIT_GENERIC(x, stdc_trailing_ones)
#define stdc_first_leading_zero(x)  STDBIT_GENERIC(x, stdc_first_leading_zero)
#define stdc_first_leading_one(x)   STDBIT_GENERIC(x, stdc_first_leading_one)
#define stdc_first_trailing_zero(x) STDBIT_GENERIC(x, stdc_first_trailing_zero)
#define stdc_first_trailing_one(x)  STDBIT_GENERIC(x, stdc_first_trailing_one)
#define stdc_count_zeros(x)         STDBIT_GENERIC(x, stdc_count_zeros)
#define stdc_count_ones(x)          STDBIT_GENERIC(x, stdc_count_ones)
#define stdc_has_single_bit(x)      STDBIT_GENERIC(x, stdc_has_single_bit)
#define stdc_bit_width(x)           STDBIT_GENERIC(x, stdc_bit_width)
#define stdc_bit_floor(x)           STDBIT_GENERIC(x, stdc_bit_floor)
#define stdc_bit_ceil(x)            STDBIT_GENERIC(x, stdc_bit_ceil)

#endif /* C11 _Generic available */

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /* STDBIT_REPLACEMENT_H_INCLUDED */
