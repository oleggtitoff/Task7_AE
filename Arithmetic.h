/*
 * Arithmetic.h
 *
 *  Created on: Jul 17, 2019
 *      Author: Intern_2
 */

#ifndef ARITHMETIC
#define ARITHMETIC

#include <stdint.h>
#include <math.h>

#include <xtensa/tie/xt_hifi2.h>

//
// GLOBAL MACRO DEFINITIONS
//
#define ALWAYS_INLINE static inline __attribute__((always_inline))
#define NEVER_INLINE static __attribute__((noinline))

#define DIV_PRECISION 5		//be careful with too small values


// F64x2	: vector 64-bit fractional
typedef struct {
	ae_f64 h;
	ae_f64 l;
}F64x2;

// F32x2	: vector 32-bit fractional
typedef ae_f32x2 F32x2;

// I32x2	: vector 32-bit integer
typedef struct {
	int32_t h;
	int32_t l;
} I32x2;


//
// INTERNAL TYPES CONVERTERS
//



//
// INTERNAL TYPES ARITHMETIC
//

ALWAYS_INLINE double dBtoGain(double dB)
{
	return pow(10, dB / 20.0);
}

ALWAYS_INLINE int32_t doubleToFixed31(double x)
{
	if (x >= 1)
	{
		return INT32_MAX;
	}
	else if (x < -1)
	{
		return INT32_MIN;
	}

	return (int32_t)(x * (double)(1LL << 31));
}

ALWAYS_INLINE int32_t doubleToFixed29(double x)
{
	if (x >= 4)
	{
		return INT32_MAX;
	}
	else if (x < -4)
	{
		return INT32_MIN;
	}

	return (int32_t)(x * (double)(1LL << 29));
}

ALWAYS_INLINE int32_t doubleToFixed27(double x)
{
	if (x >= 16)
	{
		return INT32_MAX;
	}
	else if (x < -16)
	{
		return INT32_MIN;
	}

	return (int32_t)(x * (double)(1LL << 27));
}

ALWAYS_INLINE ae_f32x2 int16ToF32x2(int16_t x, int16_t y)
{
	return AE_MOVF32X2_FROMINT32X2(AE_MOVDA32X2((int32_t)x << 16, (int32_t)y << 16));
}

ALWAYS_INLINE ae_f32x2 int32ToF32x2(int32_t x, int32_t y)
{
	return AE_MOVF32X2_FROMINT32X2(AE_MOVDA32X2(x, y));
}

ALWAYS_INLINE ae_f64 uint32ToF64(uint32_t x)
{
	return AE_MOVF64_FROMINT32X2(AE_MOVDA32X2(0, x));
}

ALWAYS_INLINE ae_f32x2 floatToF32x2(float h, float l)
{
	int32_t x;
	int32_t y;

	if (h >= 1)
	{
		x = INT32_MAX;
	}
	else if (h < -1)
	{
		x = INT32_MIN;
	}

	if (l >= 1)
	{
		y = INT32_MAX;
	}
	else if (h < -1)
	{
		y = INT32_MIN;
	}

	x = (int32_t)(h * (double)(1LL << 31));
	y = (int32_t)(l * (double)(1LL << 31));

	return int32ToF32x2(x, y);
}

ALWAYS_INLINE float Q31ToFloat_h(ae_f32x2 x)
{
	return (float)(AE_MOVAD32_H(AE_MOVINT32X2_FROMF32X2(x)) / (double)(1LL << 31));
}

ALWAYS_INLINE float Q31ToFloat_l(ae_f32x2 x)
{
	return (float)(AE_MOVAD32_L(AE_MOVINT32X2_FROMF32X2(x)) / (double)(1LL << 31));
}

ALWAYS_INLINE F64x2 putF64ToF64x2(ae_f64 h, ae_f64 l)
{
	F64x2 res;
	res.h = h;
	res.l = l;

	return res;
}

ALWAYS_INLINE F64x2 f32x2ToF64x2(ae_f32x2 x)
{
	F64x2 res;
	res.h = AE_MOVF64_FROMF32X2(AE_SEL32_LH(AE_ZERO32(), x));
	res.l = AE_MOVF64_FROMF32X2(AE_SEL32_LL(AE_ZERO32(), x));
	return res;
}

ALWAYS_INLINE ae_f64 LeftShiftA(ae_f64 x, uint8_t shift)
{
	return AE_SLAA64S(x, shift);
}

ALWAYS_INLINE ae_f32x2 LeftShiftA_32x2(ae_f32x2 x, uint8_t shift)
{
	return AE_SLAA32S(x, shift);
}

ALWAYS_INLINE F64x2 LeftShiftA_64x2(F64x2 x, uint8_t shift)
{
	x.h = AE_SLAA64S(x.h, shift);
	x.l = AE_SLAA64S(x.l, shift);
	return x;
}

ALWAYS_INLINE ae_f64 RightShiftA(ae_f64 x, uint8_t shift)
{
	return AE_SRAA64(x, shift);
}

ALWAYS_INLINE F64x2 RightShiftA_64x2(F64x2 x, uint8_t shift)
{
	x.h = AE_SRAA64(x.h, shift);
	x.l = AE_SRAA64(x.l, shift);
	return x;
}

ALWAYS_INLINE ae_f32x2 RightShiftA_32x2(ae_f32x2 x, uint8_t shift)
{
	return AE_SRAA32S(x, shift);
}

ALWAYS_INLINE ae_f64 RightShiftL(ae_f64 x, uint8_t shift)
{
	return AE_SRLA64(x, shift);
}

ALWAYS_INLINE ae_f32x2 Q31ToQ29x2(ae_f32x2 x)
{
	return RightShiftA_32x2(x, 2);
}

ALWAYS_INLINE ae_f32x2 Q31ToQ27x2(ae_f32x2 x)
{
	return RightShiftA_32x2(x, 4);
}

ALWAYS_INLINE ae_f32x2 Q27ToQ31x2(ae_f32x2 x)
{
	return LeftShiftA_32x2(x, 4);
}

ALWAYS_INLINE ae_f32x2 roundF64x2ToF32x2(F64x2 x)
{
	return AE_ROUND32X2F64SSYM(x.h, x.l);
}

ALWAYS_INLINE ae_f16x4 roundF64x2ToF16x4(F64x2 x)
{
	return AE_ROUND16X4F32SSYM(0, roundF64x2ToF32x2(x));
}

ALWAYS_INLINE ae_f32x2 Add(ae_f32x2 x, ae_f32x2 y)
{
	return AE_ADD32S(x, y);
}

ALWAYS_INLINE ae_f32x2 Sub(ae_f32x2 x, ae_f32x2 y)
{
	return AE_SUB32S(x, y);
}

ALWAYS_INLINE F64x2 Sub64x2(F64x2 x, F64x2 y)
{
	F64x2 res;
	res.h = AE_SUB64S(x.h, y.h);
	res.l = AE_SUB64S(x.l, y.l);
	return res;
}

ALWAYS_INLINE ae_f32x2 Mul(ae_f32x2 x, ae_f32x2 y)
{
	return AE_MULFP32X2RS(x, y);
}

ALWAYS_INLINE F64x2 Mul64(ae_f32x2 x, ae_f32x2 y)
{
	F64x2 res;
	res.h = AE_MULF32S_HH(x, y);
	res.l = AE_MULF32S_LL(x, y);
	return res;
}

ALWAYS_INLINE ae_f32x2 MulQ29x2(ae_f32x2 x, ae_f32x2 y)
{
	return roundF64x2ToF32x2(LeftShiftA_64x2(Mul64(x, y), 2));
}

ALWAYS_INLINE ae_f32x2 MulQ27x2(ae_f32x2 x, ae_f32x2 y)
{
	return roundF64x2ToF32x2(LeftShiftA_64x2(Mul64(x, y), 4));
}

ALWAYS_INLINE F64x2 Mac(F64x2 acc, ae_f32x2 x, ae_f32x2 y)
{
	AE_MULAF32S_LL(acc.l, x, y);
	AE_MULAF32S_HH(acc.h, x, y);
	return acc;
}

ALWAYS_INLINE F64x2 MSub(F64x2 acc, ae_f32x2 x, ae_f32x2 y)
{
	AE_MULSF32S_LL(acc.l, x, y);
	AE_MULSF32S_HH(acc.h, x, y);
	return acc;
}

ALWAYS_INLINE F64x2 Abs64x2(F64x2 x)
{
	x.h = AE_ABS64S(x.h);
	x.l = AE_ABS64S(x.l);
	return x;
}

ALWAYS_INLINE ae_f32x2 Div(ae_f32x2 x, ae_f32x2 y)
{
	ae_f32x2 precision = AE_MOVDA32X2(DIV_PRECISION, DIV_PRECISION);

	ae_f32x2 low = AE_ZERO32();								//low boundary
	ae_f32x2 high = AE_MOVDA32X2(INT32_MAX, INT32_MAX);		//high boundary
	ae_f32x2 mid;											//middle value
	ae_f32x2 midY;											//mid * y

	xtbool2 resultIsNegative = AE_LT32(AE_XOR32(x, y), AE_ZERO32());
	xtbool2 precisionAchieved;

	//flags for mid value:
	xtbool2	isLimitValue;
	xtbool2 ifLessThanX;
	xtbool2 ifBiggerThanX;

	xtbool2 yIs0 = AE_EQ32(y, AE_ZERO32());			//if y == 0
	xtbool2 xIs0 = AE_EQ32(x, AE_ZERO32());			//if x == 0

	//isCalculated = yIs0 | xIs0
	xtbool2 isCalculated = xtbool_join_xtbool2(
								XT_ORB(xtbool2_extract_0(yIs0), xtbool2_extract_0(xIs0)),
								XT_ORB(xtbool2_extract_1(yIs0), xtbool2_extract_1(xIs0))
								);

	AE_MOVT32X2(mid, AE_ZERO32(), xIs0);							//if x == 0, mid = 0
	AE_MOVT32X2(mid, AE_MOVDA32X2(INT32_MAX, INT32_MAX), yIs0);		//if y == 0, mid = MAX

	if ((int8_t)isCalculated == 3)
	{
		return mid;
	}

	x = AE_ABS32S(x);		//abs values
	y = AE_ABS32S(y);

	while (1)
	{
		AE_MOVF32X2(mid, Add(low, RightShiftA_32x2(Sub(high, low), 1)), isCalculated);		//mid = low + ((high - low) / 2),
																							//if is not calculated yet
		midY = Mul(mid, y);		//mid * y

		precisionAchieved = AE_LE32(AE_ABS32S(Sub(midY, x)), precision);		//if current precision is good
		isLimitValue = AE_LE32(Sub(INT32_MAX, AE_ABS32S(mid)), precision);			//if mid value is close to MAX or MIN

		//update isCalculated
		//isCalculated = isCalculated | precisionAchieved | isLimitValue
		isCalculated = xtbool_join_xtbool2(
											XT_ORB(
													xtbool2_extract_0(isCalculated),
													XT_ORB(
															xtbool2_extract_0(precisionAchieved),
															xtbool2_extract_0(isLimitValue)
															)
													),
											XT_ORB(
													xtbool2_extract_1(isCalculated),
													XT_ORB(
															xtbool2_extract_1(precisionAchieved),
															xtbool2_extract_1(isLimitValue)
															)
													)
											);

		if ((int8_t)isCalculated == 3)
		{
			AE_MOVT32X2(mid, AE_NEG32(mid), resultIsNegative);		//unsigned result to signed
			return mid;
		}

		ifLessThanX = AE_LT32(midY, x);				//if midY < x
		ifBiggerThanX = AE_LE32(x, midY);			//if midY >= x

		AE_MOVT32X2(low, mid, ifLessThanX);			//if midY < x, low = mid
		AE_MOVT32X2(high, mid, ifBiggerThanX);		//if midY >= x, high = mid
	}
}

ALWAYS_INLINE ae_f32x2 DivQ29x2(ae_f32x2 x, ae_f32x2 y)
{
	ae_f32x2 precision = AE_MOVDA32X2(DIV_PRECISION, DIV_PRECISION);

	ae_f32x2 low = AE_ZERO32();								//low boundary
	ae_f32x2 high = AE_MOVDA32X2(INT32_MAX, INT32_MAX);		//high boundary
	ae_f32x2 mid;											//middle value
	ae_f32x2 midY;											//mid * y

	xtbool2 resultIsNegative = AE_LT32(AE_XOR32(x, y), AE_ZERO32());
	xtbool2 precisionAchieved;

	//flags for mid value:
	xtbool2	isLimitValue;
	xtbool2 ifLessThanX;
	xtbool2 ifBiggerThanX;

	xtbool2 yIs0 = AE_EQ32(y, AE_ZERO32());			//if y == 0
	xtbool2 xIs0 = AE_EQ32(x, AE_ZERO32());			//if x == 0

	//isCalculated = yIs0 | xIs0
	xtbool2 isCalculated = xtbool_join_xtbool2(
								XT_ORB(xtbool2_extract_0(yIs0), xtbool2_extract_0(xIs0)),
								XT_ORB(xtbool2_extract_1(yIs0), xtbool2_extract_1(xIs0))
								);

	AE_MOVT32X2(mid, AE_ZERO32(), xIs0);							//if x == 0, mid = 0
	AE_MOVT32X2(mid, AE_MOVDA32X2(INT32_MAX, INT32_MAX), yIs0);		//if y == 0, mid = MAX

	if ((int8_t)isCalculated == 3)
	{
		return mid;
	}

	x = AE_ABS32S(x);		//abs values
	y = AE_ABS32S(y);

	while (1)
	{
		AE_MOVF32X2(mid, Add(low, RightShiftA_32x2(Sub(high, low), 1)), isCalculated);		//mid = low + ((high - low) / 2),
																							//if is not calculated yet
		midY = MulQ29x2(mid, y);		//mid * y

		precisionAchieved = AE_LE32(AE_ABS32S(Sub(midY, x)), precision);		//if current precision is good
		isLimitValue = AE_LE32(Sub(INT32_MAX, AE_ABS32S(mid)), precision);			//if mid value is close to MAX or MIN

		//update isCalculated
		//isCalculated = isCalculated | precisionAchieved | isLimitValue
		isCalculated = xtbool_join_xtbool2(
											XT_ORB(
													xtbool2_extract_0(isCalculated),
													XT_ORB(
															xtbool2_extract_0(precisionAchieved),
															xtbool2_extract_0(isLimitValue)
															)
													),
											XT_ORB(
													xtbool2_extract_1(isCalculated),
													XT_ORB(
															xtbool2_extract_1(precisionAchieved),
															xtbool2_extract_1(isLimitValue)
															)
													)
											);

		if ((int8_t)isCalculated == 3)
		{
			AE_MOVT32X2(mid, AE_NEG32(mid), resultIsNegative);		//unsigned result to signed
			return mid;
		}

		ifLessThanX = AE_LT32(midY, x);				//if midY < x
		ifBiggerThanX = AE_LE32(x, midY);			//if midY >= x

		AE_MOVT32X2(low, mid, ifLessThanX);			//if midY < x, low = mid
		AE_MOVT32X2(high, mid, ifBiggerThanX);		//if midY >= x, high = mid
	}
}

ALWAYS_INLINE ae_f32x2 DivQ27x2(ae_f32x2 x, ae_f32x2 y)
{
	ae_f32x2 precision = AE_MOVDA32X2(DIV_PRECISION, DIV_PRECISION);

	ae_f32x2 low = AE_ZERO32();								//low boundary
	ae_f32x2 high = AE_MOVDA32X2(INT32_MAX, INT32_MAX);		//high boundary
	ae_f32x2 mid;											//middle value
	ae_f32x2 midY;											//mid * y

	xtbool2 resultIsNegative = AE_LT32(AE_XOR32(x, y), AE_ZERO32());
	xtbool2 precisionAchieved;

	//flags for mid value:
	xtbool2	isLimitValue;
	xtbool2 ifLessThanX;
	xtbool2 ifBiggerThanX;

	xtbool2 yIs0 = AE_EQ32(y, AE_ZERO32());			//if y == 0
	xtbool2 xIs0 = AE_EQ32(x, AE_ZERO32());			//if x == 0

	//isCalculated = yIs0 | xIs0
	xtbool2 isCalculated = xtbool_join_xtbool2(
								XT_ORB(xtbool2_extract_0(yIs0), xtbool2_extract_0(xIs0)),
								XT_ORB(xtbool2_extract_1(yIs0), xtbool2_extract_1(xIs0))
								);

	AE_MOVT32X2(mid, AE_ZERO32(), xIs0);							//if x == 0, mid = 0
	AE_MOVT32X2(mid, AE_MOVDA32X2(INT32_MAX, INT32_MAX), yIs0);		//if y == 0, mid = MAX

	if ((int8_t)isCalculated == 3)
	{
		return mid;
	}

	x = AE_ABS32S(x);		//abs values
	y = AE_ABS32S(y);

	while (1)
	{
		AE_MOVF32X2(mid, Add(low, RightShiftA_32x2(Sub(high, low), 1)), isCalculated);		//mid = low + ((high - low) / 2),
																							//if is not calculated yet
		midY = MulQ27x2(mid, y);		//mid * y

		precisionAchieved = AE_LE32(AE_ABS32S(Sub(midY, x)), precision);		//if current precision is good
		isLimitValue = AE_LE32(Sub(INT32_MAX, AE_ABS32S(mid)), precision);			//if mid value is close to MAX or MIN

		//update isCalculated
		//isCalculated = isCalculated | precisionAchieved | isLimitValue
		isCalculated = xtbool_join_xtbool2(
											XT_ORB(
													xtbool2_extract_0(isCalculated),
													XT_ORB(
															xtbool2_extract_0(precisionAchieved),
															xtbool2_extract_0(isLimitValue)
															)
													),
											XT_ORB(
													xtbool2_extract_1(isCalculated),
													XT_ORB(
															xtbool2_extract_1(precisionAchieved),
															xtbool2_extract_1(isLimitValue)
															)
													)
											);

		if ((int8_t)isCalculated == 3)
		{
			AE_MOVT32X2(mid, AE_NEG32(mid), resultIsNegative);		//unsigned result to signed
			return mid;
		}

		ifLessThanX = AE_LT32(midY, x);				//if midY < x
		ifBiggerThanX = AE_LE32(x, midY);			//if midY >= x

		AE_MOVT32X2(low, mid, ifLessThanX);			//if midY < x, low = mid
		AE_MOVT32X2(high, mid, ifBiggerThanX);		//if midY >= x, high = mid
	}
}


#endif /* ARITHMETIC */
