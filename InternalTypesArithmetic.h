/*
 * InternalTypesArithmetic.h
 *
 *  Created on: Jul 23, 2019
 *      Author: Intern_2
 */

#ifndef INTERNAL_TYPES_ARITHMETIC
#define INTERNAL_TYPES_ARITHMETIC

#include "InternalTypesDefinitions.h"
#include "PredicatesOperations.h"

#define DIV_PRECISION 5		//be careful with too small values

//
// INTERNAL TYPES LOGICAL OPERATIONS
//
ALWAYS_INLINE F32x2 F32x2NOT(const F32x2 x)
{
	return AE_NEG32(x);
}

ALWAYS_INLINE F32x2 F32x2AND(const F32x2 x, const F32x2 y)
{
	return AE_AND32(x, y);
}

ALWAYS_INLINE F32x2 F32x2OR(const F32x2 x, const F32x2 y)
{
	return AE_OR32(x, y);
}

ALWAYS_INLINE F32x2 F32x2XOR(const F32x2 x, const F32x2 y)
{
	return AE_XOR32(x, y);
}

//
// MIN/MAX OPERATIONS
//
ALWAYS_INLINE F32x2 F32x2Max(const F32x2 x, const F32x2 y)
{
	return AE_MAX32(x, y);
}

ALWAYS_INLINE F32x2 F32x2Min(const F32x2 x, const F32x2 y)
{
	return AE_MIN32(x, y);
}

//
// INTERNAL TYPES ARITHMETIC
//
ALWAYS_INLINE F32x2 F32x2Zero()
{
	return AE_ZERO32();
}

ALWAYS_INLINE F64 F64LeftShiftAS(const F64 x, const uint8_t shift)
{
	return AE_SLAA64S(x, shift);
}

ALWAYS_INLINE F32x2 F32x2LeftShiftAS(const F32x2 x, const uint8_t shift)
{
	return AE_SLAA32S(x, shift);
}

ALWAYS_INLINE F64x2 F64x2LeftShiftAS(F64x2 x, const uint8_t shift)
{
	x.h = F64LeftShiftAS(x.h, shift);
	x.l = F64LeftShiftAS(x.l, shift);
	return x;
}

ALWAYS_INLINE F64 F64RightShiftA(const F64 x, const uint8_t shift)
{
	return AE_SRAA64(x, shift);
}

ALWAYS_INLINE F64 F64RightShiftL(const F64 x, const uint8_t shift)
{
	return AE_SRLA64(x, shift);
}

ALWAYS_INLINE F32x2 F32x2RightShiftA(const F32x2 x, const uint8_t shift)
{
	return AE_SRAA32S(x, shift);
}

ALWAYS_INLINE F64x2 F64x2RightShiftA(F64x2 x, const uint8_t shift)
{
	x.h = F64RightShiftA(x.h, shift);
	x.l = F64RightShiftA(x.l, shift);
	return x;
}

ALWAYS_INLINE F32x2 F32x2Abs(const F32x2 x)
{
	return AE_ABS32S(x);
}

ALWAYS_INLINE F64x2 F64x2Abs(F64x2 x)
{
	x.h = AE_ABS64S(x.h);
	x.l = AE_ABS64S(x.l);
	return x;
}

ALWAYS_INLINE F32x2 F32x2Add(const F32x2 x, const F32x2 y)
{
	return AE_ADD32S(x, y);
}

ALWAYS_INLINE F32x2 F32x2Sub(const F32x2 x, const F32x2 y)
{
	return AE_SUB32S(x, y);
}

ALWAYS_INLINE F32x2 F32x2Mul(const F32x2 x, const F32x2 y)
{
	return AE_MULFP32X2RS(x, y);
}

ALWAYS_INLINE F32x2 F32x2Div(F32x2 x, F32x2 y)
{
	int8_t i;
	F64 x1;
	F64 x2;
	Boolx2 resultIsNegative = F32x2LessThan(F32x2XOR(x, y), F32x2Zero());

	x = F32x2Abs(x);
	y = F32x2Abs(y);

	F32x2MovIfTrue(&x, y, F32x2LessEqual(y, x));

	x1 = AE_MOVF64_FROMF32X2(AE_SEL32_HH(x, F32x2Zero()));
	x2 = AE_MOVF64_FROMF32X2(AE_SEL32_LL(x, F32x2Zero()));

	for (i = 0; i < 31; i++)
	{
		AE_DIV64D32_H(x1, y);
		AE_DIV64D32_L(x2, y);
	}

	x = AE_SEL32_LL(AE_MOVINT32X2_FROMINT64(x1), AE_MOVINT32X2_FROMINT64(x2));
	F32x2MovIfTrue(&x, F32x2Sub(F32x2Zero(), x), resultIsNegative);
	return x;
}

F32x2 F32x2DivExp(F32x2 x, F32x2 y, int8_t *exp)
{
	int8_t expX = AE_NSAZ32_L(x);

	//TODO
}

F32x2 F32x2DivBinarySearch_Slow(F32x2 x, F32x2 y, const int8_t Q)
{
	F32x2 maxValue = F32x2Set(INT32_MAX);
	F32x2 precision = F32x2Set(DIV_PRECISION << (31 - Q));

	F32x2 low = F32x2Zero();				//low boundary
	F32x2 high = maxValue;					//high boundary
	F32x2 mid;								//middle value
	F32x2 midY;								//mid * y

	Boolx2 resultIsNegative = F32x2LessThan(F32x2XOR(x, y), F32x2Zero());
	Boolx2 precisionAchieved;

	//flags for mid value:
	Boolx2 isLimitValue;
	Boolx2 ifLessThanX;
	Boolx2 ifBiggerThanX;

	Boolx2 xIs0 = F32x2Equal(x, F32x2Zero());			//if x == 0
	Boolx2 yIs0 = F32x2Equal(y, F32x2Zero());			//if y == 0

	Boolx2 isCalculated = Boolx2OR(yIs0, xIs0);

	F32x2MovIfTrue(&mid, low, xIs0);			//if x == 0, mid = 0
	F32x2MovIfTrue(&mid, high, yIs0);		//if y == 0, mid = MAX

	if ((int8_t)isCalculated == 3)
	{
		return mid;
	}

	x = F32x2Abs(x);
	y = F32x2Abs(y);

	while (1)
	{
		F32x2MovIfTrue(	&mid,
						F32x2Add(low, F32x2RightShiftA(F32x2Sub(high, low), 1)),
						Boolx2NOT(isCalculated));

		midY = F32x2LeftShiftAS(F32x2Mul(mid, y), 31 - Q);		//mid * y

		precisionAchieved = F32x2LessEqual(F32x2Abs(F32x2Sub(midY, x)), precision);
		isLimitValue = F32x2LessEqual(F32x2Sub(maxValue, F32x2Abs(mid)), precision);

		isCalculated = Boolx2OR(isCalculated, Boolx2OR(precisionAchieved, isLimitValue));

		if ((int8_t)isCalculated == 3)
		{
			F32x2MovIfTrue(&mid, F32x2Sub(F32x2Zero(), mid), resultIsNegative);
			return mid;
		}

		ifLessThanX = F32x2LessThan(midY, x);
		ifBiggerThanX = F32x2LessEqual(x, midY);

		F32x2MovIfTrue(&low, mid, ifLessThanX);
		F32x2MovIfTrue(&high, mid, ifBiggerThanX);
	}
}

ALWAYS_INLINE F64x2 F32x2MulF64x2(const F32x2 x, const F32x2 y)
{
	F64x2 res;
	res.h = AE_MULF32S_HH(x, y);
	res.l = AE_MULF32S_LL(x, y);
	return res;
}

ALWAYS_INLINE F64x2 F32x2MacF64x2(F64x2 acc, const F32x2 x, const F32x2 y)
{
	AE_MULAF32S_LL(acc.l, x, y);
	AE_MULAF32S_HH(acc.h, x, y);
	return acc;
}

ALWAYS_INLINE F64x2 F32x2MSubF64x2(F64x2 acc, const F32x2 x, const F32x2 y)
{
	AE_MULSF32S_LL(acc.l, x, y);
	AE_MULSF32S_HH(acc.h, x, y);
	return acc;
}

#endif /* INTERNAL_TYPES_ARITHMETIC */
