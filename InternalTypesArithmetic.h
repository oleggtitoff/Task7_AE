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

ALWAYS_INLINE F64x2 F64x2AND(const F64x2 x, const F64x2 y)
{
	F64x2 res;
	res.h = AE_AND64(x.h, y.h);
	res.l = AE_AND64(x.l, y.l);
	return res;
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

//ALWAYS_INLINE
static F32x2 F32x2InterpolL(const F32x2 x, const F32x2 y, const F32x2 alpha)
{
	F32x2 xValue = F32x2Mul(x, F32x2Sub(0x7fffffff, alpha));
	F32x2 yValue = F32x2Mul(y, alpha);
	F32x2 res = F32x2Add(xValue, yValue);

	return res;
}

ALWAYS_INLINE F32x2 F32x2Log2(F32x2 x)
{
	// Input/Output in Q27

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

	// Here 0x4000000 is min (first) value in log2InputsTable and 0x80000 is the step between values
	F32x2 alpha = F32x2LeftShiftAS(F32x2AND(x, F32x2Set(0x7ffff)), 12);			// Q31
	F32x2 lowIndex = F32x2RightShiftA(F32x2Sub(x, F32x2Set(0x4000000)), 19);
	F32x2 highIndex = F32x2Add(lowIndex, F32x2Set(0x1));

	F32x2 lowValue = F32x2Join(
								log2OutputsTable[(int)F32x2ToI32Extract_h(lowIndex)],
								log2OutputsTable[(int)F32x2ToI32Extract_l(lowIndex)]);
	F32x2 highValue = F32x2Join(
								log2OutputsTable[(int)F32x2ToI32Extract_h(highIndex)],
								log2OutputsTable[(int)F32x2ToI32Extract_l(highIndex)]);

	F32x2 tableRes = F32x2InterpolL(lowValue, highValue, alpha);

	return F32x2Add(res, tableRes);
}

ALWAYS_INLINE F32x2 F32x2PowOf2(F32x2 x)
{
	// Input/Output in Q27

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

	F32x2 alphaMask = F32x2AND(x, F32x2Set(0xfffff));
	F32x2 alpha = F32x2LeftShiftAS(alphaMask, 11);
	F32x2 lowIndex = F32x2RightShiftA(x, 20);
	lowIndex = F32x2Add(lowIndex, F32x2Set(128));
	F32x2 highIndex = F32x2Add(lowIndex, F32x2Set(0x1));

	F32x2 lowValue = F32x2Join(
								powOf2OutputsTable[(int)F32x2ToI32Extract_h(lowIndex)],
								powOf2OutputsTable[(int)F32x2ToI32Extract_l(lowIndex)]);
	F32x2 highValue = F32x2Join(
								powOf2OutputsTable[(int)F32x2ToI32Extract_h(highIndex)],
								powOf2OutputsTable[(int)F32x2ToI32Extract_l(highIndex)]);

	F32x2 tableRes = F32x2InterpolL(lowValue, highValue, alpha);

	return F32x2LeftShiftAS(F32x2Mul(res, tableRes), 4);
}

ALWAYS_INLINE F32x2 F32x2Pow(F32x2 x, F32x2 y)
{
	// Input/Output in Q27

	return F32x2PowOf2(F32x2LeftShiftAS(F32x2Mul(y, F32x2Log2(x)), 4));
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
