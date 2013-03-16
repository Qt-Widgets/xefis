/* vim:ts=4
 *
 * Copyleft 2012…2013  Michał Gawron
 * Marduk Unix Labs, http://mulabs.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Visit http://www.gnu.org/licenses/gpl-3.0.html for more information on licensing.
 */

#ifndef XEFIS__UTILITY__NUMERIC_H__INCLUDED
#define XEFIS__UTILITY__NUMERIC_H__INCLUDED

// Standard:
#include <cstddef>
#include <complex>
#include <cmath>

// Xefis:
#include <xefis/config/all.h>
#include <xefis/utility/range.h>


inline constexpr float
renormalize (float value, float a1, float b1, float a2, float b2) noexcept
{
	return b1 == a1
		? a2
		: (b2 - a2) / (b1 - a1) * value + (-(b2 - a2) / (b1 - a1) * a1 + a2);
}


inline constexpr float
renormalize (float value, Range<float> range1, Range<float> range2) noexcept
{
	return renormalize (value, range1.min(), range1.max(), range2.min(), range2.max());
}


/**
 * Return x > 0.0 ? 1.0 : -1.0.
 */
inline float
sgn (float const& v) noexcept
{
	union {
		float f;
		uint32_t i;
	} u = {v};
	const uint32_t s = u.i & (1 << 31);
	u.f = 1.0f;
	u.i |= s;
	return u.f;
}


/**
 * Works properly only on floats.
 * \param	n - dividend
 * \param	d - divisor
 */
template<class Number>
	constexpr Number
	floored_mod (Number n, Number d)
	{
		return n - (d * std::floor (n / d));
	}


template<>
	constexpr int
	floored_mod<int> (int n, int d)
	{
		return (n % d) >= 0 ? (n % d) : (n % d) + std::abs (d);
	}


template<class Number>
	constexpr Number
	floored_mod (Number n, Number min, Number max)
	{
		return floored_mod (n - min, max - min) + min;
	}


template<class Value>
	inline constexpr Value
	limit (Value value, Value min, Value max) noexcept
	{
		return value < min
			? min
			: value > max
				? max
				: value;
	}


template<class Value>
	inline constexpr Value
	limit (Value value, Range<Value> range) noexcept
	{
		return range.min() <= range.max()
			? limit (value, range.min(), range.max())
			: limit (value, range.max(), range.min());
	}

#endif

