/*
 * InternalTypesConverters.h
 *
 *  Created on: Jul 23, 2019
 *      Author: Intern_2
 */

#ifndef INTERNAL_TYPES_CONVERTERS
#define INTERNAL_TYPES_CONVERTERS

#include "InternalTypesDefinitions.h"

//
// INTERNAL TYPES CONVERTERS
//
ALWAYS_INLINE xtbool Boolx2Extract_h(Boolx2 x)
{
	return xtbool2_extract_1(x);
}

ALWAYS_INLINE Bool Boolx2Extract_l(Boolx2 x)
{
	return xtbool2_extract_0(x);
}

ALWAYS_INLINE Boolx2 Boolx2Set(Bool x)
{
	return xtbool_join_xtbool2(x, x);
}

ALWAYS_INLINE Boolx2 Boolx2Join(Bool h, Bool l)
{
	return xtbool_join_xtbool2(h, l);
}

ALWAYS_INLINE I32 I32x2Extract_h(F32x2 x)
{
	return AE_MOVAD32_H(F32x2ToI32x2);
}

ALWAYS_INLINE I32 I32x2Extract_l(F32x2 x)
{
	return AE_MOVAD32_L(F32x2ToI32x2);
}

ALWAYS_INLINE I32x2 I32x2Set(I32 x)
{
	return AE_MOVDA32X2(x, x);
}

ALWAYS_INLINE I32x2 I32x2Join(I32 h, I32 l)
{
	return AE_MOVDA32X2(h, l);
}

ALWAYS_INLINE I32x2 F32x2ToI32x2(F32x2 x)
{
	AE_MOVINT32X2_FROMF32X2(x);
}

ALWAYS_INLINE F32x2 F32x2Set(I32 x)
{
	return I32x2ToF32x2(I32x2Set(x));
}

ALWAYS_INLINE F32x2 F32x2Join(I32 h, I32 l)
{
	return I32x2ToF32x2(I32x2Join(h, l));
}

ALWAYS_INLINE F32x2 I32x2ToF32x2(I32x2 x)
{
	return AE_MOVF32X2_FROMINT32X2(x);
}

ALWAYS_INLINE F32x2 F64x2ToF32x2Round(F64x2 x)
{
	return AE_ROUND32X2F64SSYM(x.h, x.l);
}

ALWAYS_INLINE F64x2 F64x2Set(F64 x)
{
	F64x2 res;
	res.h = x;
	res.l = x;
	return res;
}

ALWAYS_INLINE F64x2 F64x2Join(F64 h, F64 l)
{
	F64x2 res;
	res.h = h;
	res.l = l;
	return res;
}

#endif /* INTERNAL_TYPES_CONVERTERS */
