/*
 * ExternalAndInternalTypesConverters.h
 *
 *  Created on: Jul 23, 2019
 *      Author: Intern_2
 */

#ifndef EXTERNAL_AND_INTERNAL_TYPES_CONVERTERS
#define EXTERNAL_AND_INTERNAL_TYPES_CONVERTERS

#include "InternalTypesDefinitions.h"

//
// EXTERNAL/INTERNAL TYPES CONVERTERS
//
ALWAYS_INLINE I32 doubleToI32(double x)
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

ALWAYS_INLINE F32x2 doubleToF32x2(double x)
{
	return I32x2ToF32x2(I32x2Set(doubleToI32(x)));
}

#endif /* EXTERNAL_AND_INTERNAL_TYPES_CONVERTERS */
