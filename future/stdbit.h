#ifndef FUTURE_STDBIT_H
#define FUTURE_STDBIT_H

#include <nyx/types.h>
#include <stdbit.h>

#if defined(__BMI2__) || (defined(__x86_64__) && defined(__has_include) && __has_include(<immintrin.h>))
#include <immintrin.h>
#define HAS_BMI2 true
#else
#define HAS_BMI2 false
#endif

#if HAS_BMI2

static inline bitboard
stdc_bit_compress(bitboard value, bitboard mask)
{
	return _pext_u64(value, mask);
}

static inline bitboard
stdc_bit_expand(bitboard value, bitboard mask)
{
	return _pdep_u64(value, mask);
}

#else // HAS_BMI2

static inline bitboard
stdc_bit_compress(bitboard value, bitboard mask)
{
	bitboard res, bb, m;

	res = 0;
	for (bb = 1; mask; bb <<= 1)
	{
		m = mask & -mask;
		mask ^= m;
		if (val & m)
			res |= bb;
	}

	return res;
}

static inline bitboard
stdc_bit_expand(bitboard value, bitboard mask)
{
	bitboard res, bb, m;

	res = 0;
	for (bb = 1; value; bb <<= 1)
	{
		if (!(bb & val)) continue;

		m = mask & -mask;
		mask ^= m;
		res |= m;
		value ^= bb;
	}

	return res;
}

#endif // HAS_BMI2

#endif // FUTURE_STDBIT_H
