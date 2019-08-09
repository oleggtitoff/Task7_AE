/*
 * InternalTypesArithmetic.h
 *
 *  Created on: Jul 23, 2019
 *      Author: Intern_2
 */

#ifndef INTERNAL_TYPES_ARITHMETIC
#define INTERNAL_TYPES_ARITHMETIC

#include "InternalTypesDefinitions.h"
#include "InternalTypesConverters.h"
#include "PredicatesOperations.h"
#include "Tables.h"

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

ALWAYS_INLINE F32x2 F32x2LeftShiftAS_Apart(const F32x2 x, const uint8_t shiftH, const uint8_t shiftL)
{
	return AE_SEL32_LL(
						F32x2LeftShiftAS(AE_SEL32_HH(x, x), shiftH),
						F32x2LeftShiftAS(AE_SEL32_LL(x, x), shiftL));
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

ALWAYS_INLINE F32x2 F32x2RightShiftA_Apart(const F32x2 x, const uint8_t shiftH, const uint8_t shiftL)
{
	return AE_SEL32_LL(
						F32x2RightShiftA(AE_SEL32_HH(x, x), shiftH),
						F32x2RightShiftA(AE_SEL32_LL(x, x), shiftL));
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

ALWAYS_INLINE F32x2 F32x2BuiltInDiv(F32x2 x, F32x2 y)
{
	return F32x2Join(
					(int32_t)F32x2ToI32Extract_h(x) / (int32_t)F32x2ToI32Extract_h(y),
					(int32_t)F32x2ToI32Extract_l(x) / (int32_t)F32x2ToI32Extract_l(y));
}

ALWAYS_INLINE F32x2 F32x2Log2(F32x2 x)
{
	// Input/Output in Q27

	F32x2 index = 0;
	F32x2 res = F32x2Zero();
	Boolx2 isZero = F32x2Equal(x, F32x2Zero());
	Boolx2 isCalculated = isZero;
	F32x2MovIfTrue(&res, F32x2Set(INT32_MIN), isZero);

	x = F32x2Abs(x);

	int8_t clsH = AE_NSAZ32_L(AE_SEL32_LH(x, x));
	int8_t clsL = AE_NSAZ32_L(x);
	F32x2 cls = F32x2Join(clsH, clsL);
	Boolx2 isBigger = Boolx2AND(F32x2LessThan(cls, F32x2Set(4)), Boolx2NOT(isCalculated));
	Boolx2 isSmaller = Boolx2AND(F32x2LessThan(F32x2Set(4), cls), Boolx2NOT(isCalculated));

	F32x2MovIfTrue(&x, F32x2RightShiftA_Apart(x, 4 - clsH, 4 - clsL), isBigger);
	F32x2MovIfTrue(&res, F32x2LeftShiftAS(F32x2Sub(F32x2Set(4), cls), 27), isBigger);

	F32x2MovIfTrue(&x, F32x2LeftShiftAS_Apart(x, clsH - 4, clsL - 4), isSmaller);
	F32x2MovIfTrue(&res, F32x2LeftShiftAS(F32x2Sub(F32x2Zero(), F32x2Sub(cls, F32x2Set(4))), 27), isSmaller);

	// Here 0x4000000 is min (first) value in log2InputsTable and 0x81020 is the step between values
	index = F32x2BuiltInDiv(F32x2Sub(x, F32x2Set(0x4000000)), F32x2Set(0x81020));

	return F32x2Add(res, F32x2Join(
									log2OutputsTable[(int)F32x2ToI32Extract_h(index)],
									log2OutputsTable[(int)F32x2ToI32Extract_l(index)]));
}

ALWAYS_INLINE F32x2 F32x2PowOf2(F32x2 x)
{
	// Input/Output in Q27

	F32x2 index = 0;
	F32x2 res = 0x8000000;
	F32x2 mask = F32x2Set(0x78000000);
	Boolx2 isNegative = F32x2LessThan(x, F32x2Zero());
	F32x2 count = F32x2AND(F32x2Abs(x), mask);
	F32x2 countShifted = F32x2RightShiftA(count, 27);

	F32x2MovIfTrue(&x, F32x2Sub(x, count), Boolx2NOT(isNegative));
	F32x2MovIfTrue(&res,
			F32x2LeftShiftAS_Apart(res, (int)F32x2ToI32Extract_h(countShifted), (int)F32x2ToI32Extract_l(countShifted)),
			Boolx2NOT(isNegative));

	F32x2MovIfTrue(&x, F32x2Add(x, count), isNegative);
	F32x2MovIfTrue(&res,
			F32x2RightShiftA_Apart(res, (int)F32x2ToI32Extract_h(countShifted), (int)F32x2ToI32Extract_l(countShifted)),
			isNegative);


	index = F32x2BuiltInDiv(x, F32x2Set(0x102040));
	F32x2MovIfTrue(&index, F32x2Add(F32x2Abs(index), F32x2Set(127)), F32x2LessThan(index, F32x2Zero()));

	return F32x2LeftShiftAS(F32x2Mul(res, F32x2Join(
										powOf2OutputsTable[(int)F32x2ToI32Extract_h(index)],
										powOf2OutputsTable[(int)F32x2ToI32Extract_l(index)])), 4);
}

ALWAYS_INLINE F32x2 F32x2Pow(F32x2 x, F32x2 y)
{
	// Input/Output in Q27

	return F32x2PowOf2(F32x2LeftShiftAS(F32x2Mul(y, F32x2Log2(x)), 4));
}

//F32x2 F32x2DivExp(F32x2 x, F32x2 y, int8_t *expH, int8_t *expL)
//{
//	int8_t expX_H = AE_NSAZ32_L(AE_SEL32_LH(x, x));
//	int8_t expX_L = AE_NSAZ32_L(x);
//	int8_t expY_H = AE_NSAZ32_L(AE_SEL32_LH(y, y));
//	int8_t expY_L = AE_NSAZ32_L(y);
//
//	x = F32x2LeftShiftAS_Apart(x, expX_H - 1, expX_L - 1);
//	y = F32x2LeftShiftAS_Apart(y, expY_H, expY_L);
//
//	*expH = expX_H - expY_H - 1;
//	*expL = expX_L - expY_L - 1;
//
//	return F32x2Div(x, y);
//}
//
//F32x2 F32x2DivBinarySearch_Slow(F32x2 x, F32x2 y, const int8_t Q)
//{
//	F32x2 maxValue = F32x2Set(INT32_MAX);
//	F32x2 precision = F32x2Set(DIV_PRECISION << (31 - Q));
//
//	F32x2 low = F32x2Zero();				//low boundary
//	F32x2 high = maxValue;					//high boundary
//	F32x2 mid;								//middle value
//	F32x2 midY;								//mid * y
//
//	Boolx2 resultIsNegative = F32x2LessThan(F32x2XOR(x, y), F32x2Zero());
//	Boolx2 precisionAchieved;
//
//	//flags for mid value:
//	Boolx2 isLimitValue;
//	Boolx2 ifLessThanX;
//	Boolx2 ifBiggerThanX;
//
//	Boolx2 xIs0 = F32x2Equal(x, F32x2Zero());			//if x == 0
//	Boolx2 yIs0 = F32x2Equal(y, F32x2Zero());			//if y == 0
//
//	Boolx2 isCalculated = Boolx2OR(yIs0, xIs0);
//
//	F32x2MovIfTrue(&mid, low, xIs0);			//if x == 0, mid = 0
//	F32x2MovIfTrue(&mid, high, yIs0);		//if y == 0, mid = MAX
//
//	if ((int8_t)isCalculated == 3)
//	{
//		return mid;
//	}
//
//	x = F32x2Abs(x);
//	y = F32x2Abs(y);
//
//	while (1)
//	{
//		F32x2MovIfTrue(	&mid,
//						F32x2Add(low, F32x2RightShiftA(F32x2Sub(high, low), 1)),
//						Boolx2NOT(isCalculated));
//
//		midY = F32x2LeftShiftAS(F32x2Mul(mid, y), 31 - Q);		//mid * y
//
//		precisionAchieved = F32x2LessEqual(F32x2Abs(F32x2Sub(midY, x)), precision);
//		isLimitValue = F32x2LessEqual(F32x2Sub(maxValue, F32x2Abs(mid)), precision);
//
//		isCalculated = Boolx2OR(isCalculated, Boolx2OR(precisionAchieved, isLimitValue));
//
//		if ((int8_t)isCalculated == 3)
//		{
//			F32x2MovIfTrue(&mid, F32x2Sub(F32x2Zero(), mid), resultIsNegative);
//			return mid;
//		}
//
//		ifLessThanX = F32x2LessThan(midY, x);
//		ifBiggerThanX = F32x2LessEqual(x, midY);
//
//		F32x2MovIfTrue(&low, mid, ifLessThanX);
//		F32x2MovIfTrue(&high, mid, ifBiggerThanX);
//	}
//}

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
