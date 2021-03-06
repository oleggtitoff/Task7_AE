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
ALWAYS_INLINE Bool Boolx2Extract_h(const Boolx2 x)
{
	return xtbool2_extract_1(x);
}

ALWAYS_INLINE Bool Boolx2Extract_l(const Boolx2 x)
{
	return xtbool2_extract_0(x);
}

ALWAYS_INLINE Boolx2 Boolx2Set(const Bool x)
{
	return xtbool_join_xtbool2(x, x);
}

ALWAYS_INLINE Boolx2 Boolx2Join(const Bool h, const Bool l)
{
	return xtbool_join_xtbool2(h, l);
}

ALWAYS_INLINE I32x2 I32x2Set(const I32 x)
{
	return AE_MOVDA32X2(x, x);
}

ALWAYS_INLINE I32x2 I32x2Join(const I32 h, const I32 l)
{
	return AE_MOVDA32X2(h, l);
}

ALWAYS_INLINE I32x2 F32x2ToI32x2(const F32x2 x)
{
	return AE_MOVINT32X2_FROMF32X2(x);
}

ALWAYS_INLINE I32 F32x2ToI32Extract_h(const F32x2 x)
{
	return AE_MOVAD32_H(F32x2ToI32x2(x));
}

ALWAYS_INLINE I32 F32x2ToI32Extract_l(const F32x2 x)
{
	return AE_MOVAD32_L(F32x2ToI32x2(x));
}

ALWAYS_INLINE F32x2 I32x2ToF32x2(const I32x2 x)
{
	return AE_MOVF32X2_FROMINT32X2(x);
}

ALWAYS_INLINE F32x2 F32x2Set(const I32 x)
{
	return I32x2ToF32x2(I32x2Set(x));
}

ALWAYS_INLINE F32x2 F32x2Join(const I32 h, const I32 l)
{
	return I32x2ToF32x2(I32x2Join(h, l));
}

ALWAYS_INLINE F32x2 F64x2ToF32x2Round(const F64x2 x)
{
	return AE_ROUND32X2F64SSYM(x.h, x.l);
}

ALWAYS_INLINE F32x2 F64x2ToF32x2_h(const F64x2 x)
{
	return AE_SEL32_HH(AE_MOVINT32X2_FROMINT64(x.h), AE_MOVINT32X2_FROMINT64(x.l));
}

ALWAYS_INLINE F32x2 F64x2ToF32x2_l(const F64x2 x)
{
	return AE_SEL32_LL(AE_MOVINT32X2_FROMINT64(x.h), AE_MOVINT32X2_FROMINT64(x.l));
}

ALWAYS_INLINE F64x2 F64x2Set(const F64 x)
{
	F64x2 res;
	res.h = x;
	res.l = x;
	return res;
}

ALWAYS_INLINE F64x2 F64x2Join(const F64 h, const F64 l)
{
	F64x2 res;
	res.h = h;
	res.l = l;
	return res;
}

ALWAYS_INLINE F64x2 F32x2ToF64x2_l(const F32x2 x)
{
	F64x2 res;
	res.h = AE_MOVF64_FROMF32X2(AE_SEL32_LL(AE_ZERO32(), x));
	res.l = AE_MOVF64_FROMF32X2(AE_SEL32_HH(AE_ZERO32(), x));
	return res;
}

#endif /* INTERNAL_TYPES_CONVERTERS */
