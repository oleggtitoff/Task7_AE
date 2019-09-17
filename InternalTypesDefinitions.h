/*
 * InternalTypesDefinitions.h
 *
 *  Created on: Jul 23, 2019
 *      Author: Intern_2
 */

#ifndef INTERNAL_TYPES_DEFINITIONS
#define INTERNAL_TYPES_DEFINITIONS


#include <xtensa/tie/xt_hifi2.h>


//
// GLOBAL MACRO DEFINITIONS
//
#define ALWAYS_INLINE static inline __attribute__((always_inline))
#define NEVER_INLINE static __attribute__((noinline))


//
// INTERNAL TYPES DEFINITIONS
//
//Status	: status flag
typedef enum {
	statusOK = 0,
	statusError
} Status;

// Boolx1	: scalar boolean
typedef xtbool Bool;

// Boolx2 	: vector boolean
typedef xtbool2 Boolx2;

// I32x1	: scalar 32-bit integer
typedef ae_int32 I32;

// F64x1	: scalar 64-bit fractional
typedef ae_f64 F64;

// I32x2	: vector 32-bit integer
typedef ae_int32x2 I32x2;

// F32x2	: vector 32-bit fractional
typedef ae_f32x2 F32x2;

// F64x2	: vector 64-bit fractional
typedef struct {
	ae_f64 h;
	ae_f64 l;
}F64x2;

#endif /* INTERNAL_TYPES_DEFINITIONS */
