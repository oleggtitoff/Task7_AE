/*
 * PredicatesOperations.h
 *
 *  Created on: Jul 23, 2019
 *      Author: Intern_2
 */

#ifndef PREDICATES_OPERATIONS
#define PREDICATES_OPERATIONS

#include "InternalTypesDefinitions.h"

//
// LOGICAL OPERATIONS
//
ALWAYS_INLINE Bool BoolNOT(Bool x)
{
	return XT_XORB(x, 1);
}

ALWAYS_INLINE Bool BoolAND(Bool x, Bool y)
{
	return XT_ANDB(x, y);
}

ALWAYS_INLINE Bool BoolOR(Bool x, Bool y)
{
	return XT_ORB(x, y);
}

ALWAYS_INLINE Bool BoolXOR(Bool x, Bool y)
{
	return XT_XORB(x, y);
}

ALWAYS_INLINE Boolx2 Boolx2NOT(Boolx2 x)
{
	return xtbool_join_xtbool2(
								BoolNOT(xtbool2_extract_1(x)),
								BoolNOT(xtbool2_extract_0(x)));
}

ALWAYS_INLINE Boolx2 Boolx2AND(Boolx2 x, Boolx2 y)
{
	return xtbool_join_xtbool2(
								BoolAND(xtbool2_extract_1(x), xtbool2_extract_1(y)),
								BoolAND(xtbool2_extract_0(x), xtbool2_extract_0(y)));
}

ALWAYS_INLINE Boolx2 Boolx2OR(Boolx2 x, Boolx2 y)
{
	return xtbool_join_xtbool2(
								BoolOR(xtbool2_extract_1(x), xtbool2_extract_1(y)),
								BoolOR(xtbool2_extract_0(x), xtbool2_extract_0(y)));
}

ALWAYS_INLINE Boolx2 Boolx2XOR(Boolx2 x, Boolx2 y)
{
	return xtbool_join_xtbool2(
								BoolXOR(xtbool2_extract_1(x), xtbool2_extract_1(y)),
								BoolXOR(xtbool2_extract_0(x), xtbool2_extract_0(y)));
}

//
// COMPARSION OPERATIONS
//
ALWAYS_INLINE Boolx2 F32x2Equal(F32x2 x, F32x2 y)
{
	return AE_EQ32(x, y);
}

ALWAYS_INLINE Boolx2 F32x2LessThan(F32x2 x, F32x2 y)
{
	return AE_LT32(x, y);
}

ALWAYS_INLINE Boolx2 F32x2LessEqual(F32x2 x, F32x2 y)
{
	return AE_LE32(x, y);
}

//
// CONDITIONAL MOV OPERATIONS
//
ALWAYS_INLINE void F32x2MovIfTrue(F32x2 inout, F32x2 x, Boolx2 a)
{
	AE_MOVT32X2(inout, x, a);
}

#endif /* PREDICATES_OPERATIONS */
