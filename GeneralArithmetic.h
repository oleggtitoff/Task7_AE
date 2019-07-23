/*
 * GeneralArithmetic.h
 *
 *  Created on: Jul 23, 2019
 *      Author: Intern_2
 */

#ifndef GENERAL_ARITHMETIC
#define GENERAL_ARITHMETIC

#include "InternalTypesDefinitions.h"

//
// GENERAL ARITHMETIC
//
ALWAYS_INLINE double dBtoGain(double dB)
{
	return pow(10, dB / 20.0);
}

#endif /* GENERAL_ARITHMETIC */
