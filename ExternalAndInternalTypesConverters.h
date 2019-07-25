/*
 * ExternalAndInternalTypesConverters.h
 *
 *  Created on: Jul 23, 2019
 *      Author: Intern_2
 */

#ifndef EXTERNAL_AND_INTERNAL_TYPES_CONVERTERS
#define EXTERNAL_AND_INTERNAL_TYPES_CONVERTERS

#include "InternalTypesDefinitions.h"
#include "InternalTypesConverters.h"

#include <stdint.h>

//
// EXTERNAL/INTERNAL TYPES CONVERTERS
//
ALWAYS_INLINE I32 doubleToI32(const double x)
{
	// converts double in range -1 1 to I32x1 Q31 representation
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

ALWAYS_INLINE F32x2 doubleToF32x2Set(const double x)
{
	return F32x2Set(doubleToI32(x));
}

ALWAYS_INLINE F32x2 doubleToF32x2Join(const double x, const double y)
{
	return F32x2Join(doubleToI32(x), doubleToI32(y));
}

ALWAYS_INLINE double I32ToDouble(I32 x)
{
	return (double)((int32_t)x / (double)(1LL << 31));
}

ALWAYS_INLINE double F32x2ToDoubleExtract_h(F32x2 x)
{
	return I32ToDouble(F32x2ToI32Extract_h(x));
}

ALWAYS_INLINE double F32x2ToDoubleExtract_l(F32x2 x)
{
	return I32ToDouble(F32x2ToI32Extract_l(x));
}

#endif /* EXTERNAL_AND_INTERNAL_TYPES_CONVERTERS */
