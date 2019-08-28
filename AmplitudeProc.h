/*
 * AmplitudeProc.h
 *
 *  Created on: Aug 8, 2019
 *      Author: Intern_2
 */

#ifndef AMPLITUDE_PROC
#define AMPLITUDE_PROC

#include "GeneralArithmetic.h"
#include "InternalTypesArithmetic.h"
#include "ExternalAndInternalTypesConverters.h"

#define RING_BUFF_SIZE 		128
#define DATA_BUFF_SIZE 		512		//must be twice bigger than RING_BUFF_SIZE#define CHANNELS 2


typedef enum {
	noiseGateIsActiveID 	= 201,
	expanderIsActiveID 		= 202,
	compressorIsActiveID 	= 203,
	limiterIsActiveID 		= 204,

	noiseThrID 				= 205,
	expanderHighThrID 		= 206,
	compressorLowThrID 		= 207,
	limiterThrID 			= 208,

	expanderRatioID 		= 209,
	compressorRatioID 		= 210,

	envelopeAttackTimeID 	= 211,
	envelopeReleaseTimeID 	= 212,
	expanderAttackTimeID 	= 213,
	expanderReleaseTimeID 	= 214,
	compressorAttackTimeID 	= 215,
	compressorReleaseTimeID = 216
} amplitudeProcParamID;


typedef struct {
	uint16_t currNum;
	F32x2 samples[RING_BUFF_SIZE];	// Q27
	F32x2 maxSample;				// Q27
} RingBuff;

typedef struct {
	int sampleRate;

	int8_t noiseGateIsActive;
	int8_t expanderIsActive;
	int8_t compressorIsActive;
	int8_t limiterIsActive;

	double noiseThr;				// in dB
	double expanderHighThr;			// in dB
	double compressorLowThr;		// in dB
	double limiterThr;				// in dB

	double expanderRatio;
	double compressorRatio;

	double envelopeAttackTime;		// in ms
	double envelopeReleaseTime;		// in ms
	double expanderAttackTime;		// in ms
	double expanderReleaseTime;		// in ms
	double compressorAttackTime;	// in ms
	double compressorReleaseTime;	// in ms
} AmplitudeProcParams;

typedef struct {
	F32x2 alphaAttack;			// Q31
	F32x2 alphaRelease;			// Q31
} EnvelopeCoeffs;

typedef struct {
	int8_t isActive;
	F32x2 threshold;			// Q27
} LimiterCoeffs;
typedef LimiterCoeffs NoiseGateCoeffs;

typedef struct {
	int8_t isActive;
	F32x2 threshold;			// Q27
	F32x2 C1;					// Q27
	F32x2 C2;					// Q27
	F32x2 alphaAttack;			// Q31
	F32x2 alphaRelease;			// Q31
} CompressorCoeffs;
typedef CompressorCoeffs ExpanderCoeffs;

typedef struct {
	EnvelopeCoeffs envelope;
	NoiseGateCoeffs noiseGate;
	LimiterCoeffs limiter;
	CompressorCoeffs compressor;
	ExpanderCoeffs expander;
} AmplitudeProcCoeffs;

typedef struct {
	F32x2 prevSample;			// Q27
} EnvelopeStates;

typedef struct {
	Boolx2 isWorked;
} NoiseGateStates;

typedef struct {
	Boolx2 isWorked;
	F32x2 prevGain;				// Q27
} CompressorStates;
typedef CompressorStates ExpanderStates;

typedef struct {
	RingBuff ringBuff;
	EnvelopeStates envelope;
	NoiseGateStates noiseGate;
	CompressorStates compressor;
	ExpanderStates expander;
} AmplitudeProcStates;


Status AmplitudeProcInit(AmplitudeProcParams *params, AmplitudeProcCoeffs *coeffs,
						 AmplitudeProcStates *states, int sampleRate);
Status AmplitudeProcSetParam(AmplitudeProcParams *params, AmplitudeProcCoeffs *coeffs,
							 AmplitudeProcStates *states, const uint16_t id, double value);
void ringBuffAddValue(RingBuff *ringBuff, F32x2 value);
F32x2 AmplitudeProc_Process(const AmplitudeProcCoeffs *coeffs, AmplitudeProcStates *states,
						   F32x2 sample);

#endif /* AMPLITUDE_PROC */
