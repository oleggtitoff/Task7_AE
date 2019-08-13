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
#define DATA_BUFF_SIZE 		1024	//must be twice bigger than RING_BUFF_SIZE#define CHANNELS 2

typedef enum {
	sampleRateID = 1,

	noiseGateIsActiveID = 2,
	expanderIsActiveID = 3,
	compressorIsActiveID = 4,
	limiterIsActiveID = 5,

	noiseThrID = 6,
	expanderHighThrID = 7,
	compressorLowThrID = 8,
	limiterThrID = 9,

	expanderRatioID = 10,
	compressorRatioID = 11,

	envelopeAttackTimeID = 12,
	envelopeReleaseTimeID = 13,
	expanderAttackTimeID = 14,
	expanderReleaseTimeID = 15,
	compressorAttackTimeID = 16,
	compressorReleaseTimeID = 17
} paramID;

typedef struct {
	uint16_t currNum;
	F32x2 samples[RING_BUFF_SIZE];	// Q31
	F32x2 maxSample;				// Q31
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
} Params;

typedef struct {
	F32x2 alphaAttack;			// Q31
	F32x2 alphaRelease;			// Q31
} EnvelopeCoeffs;

typedef struct {
	int8_t isActive;
	F32x2 threshold;			// Q31
} LimiterCoeffs;
typedef LimiterCoeffs NoiseGateCoeffs;

typedef struct {
	int8_t isActive;
	F32x2 threshold;			// Q31
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
} Coeffs;

typedef struct {
	F32x2 prevSample;			// Q31
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
	EnvelopeStates envelope;
	NoiseGateStates noiseGate;
	CompressorStates compressor;
	ExpanderStates expander;
} States;

Status AmplitudeProcInit(Params *params, Coeffs *coeffs, RingBuff *ringBuff,
						 States *states, const int sampleRate);
Status AmplitudeProcSetParam(Params *params, Coeffs *coeffs, States *states,
							 const uint16_t id, double value);
Status ringBuffSet(RingBuff *ringBuff, int32_t *dataBuff);
void AmplitudeProc_Process(const Coeffs *coeffs, RingBuff *ringBuff, States *states);

#endif /* AMPLITUDE_PROC */
