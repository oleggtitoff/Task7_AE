/*
 * InternalTypesArithmetic.h
 *
 *  Created on: Jul 23, 2019
 *      Author: Intern_2
 */

#ifndef INTERNAL_TYPES_ARITHMETIC
#define INTERNAL_TYPES_ARITHMETIC

#include "InternalTypesDefinitions.h"

//
// INTERNAL TYPES ARITHMETIC
//
ALWAYS_INLINE F64 F64LeftShiftA(F64 x, uint8_t shift)
{
	return AE_SLAA64S(x, shift);
}

ALWAYS_INLINE F32x2 F32x2LeftShiftA(F32x2 x, uint8_t shift)
{
	return AE_SLAA32S(x, shift);
}

ALWAYS_INLINE F64x2 F64x2LeftShiftA(F64x2 x, uint8_t shift)
{
	x.h = F64LeftShiftA(x.h, shift);
	x.l = F64LeftShiftA(x.l, shift);
	return x;
}

ALWAYS_INLINE F64 F64RightShiftA(F64 x, uint8_t shift)
{
	return AE_SRAA64(x, shift);
}

ALWAYS_INLINE F64 F64RightShiftL(F64 x, uint8_t shift)
{
	return AE_SRLA64(x, shift);
}

ALWAYS_INLINE F32x2 F32x2RightShiftA(F32x2 x, uint8_t shift)
{
	return AE_SRAA32S(x, shift);
}

ALWAYS_INLINE F64x2 F64x2RightshiftA(F64x2 x, uint8_t shift)
{
	x.h = F64RightShiftA(x.h, shift);
	x.l = F64RightShiftA(x.l, shift);
	return x;
}

ALWAYS_INLINE F32x2 F32x2Add(F32x2 x, F32x2 y)
{
	return AE_ADD32S(x, y);
}

ALWAYS_INLINE F32x2 F32x2Sub(F32x2 x, F32x2 y)
{
	return AE_SUB32S(x, y);
}

ALWAYS_INLINE F32x2 F32x2Mul(F32x2 x, F32x2 y)
{
	return AE_MULFP32X2RS(x, y);
}

//TODO

#endif /* INTERNAL_TYPES_ARITHMETIC */
