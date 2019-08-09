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

	samplesAttackTimeID = 12,
	samplesReleaseTimeID = 13,
	gainAttackTimeID = 14,
	gainReleaseTimeID = 15,
	fadeAttackTimeID = 16,
	fadeReleaseTimeID = 17
} paramID;

typedef struct {
	uint16_t currNum;
	F32x2 samples[RING_BUFF_SIZE];	// Q31
	F32x2 maxSample;				// Q31
} RingBuff;

typedef struct {
	F32x2 prevSample;				// Q31
	F32x2 prevGain;					// Q27
	Boolx2 isFade;
} PrevValuesBuff;

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

	double samplesAttackTime;	// in ms
	double samplesReleaseTime;	// in ms
	double gainAttackTime;		// in ms
	double gainReleaseTime;	// in ms
	double fadeAttackTime;		// in ms
	double fadeReleaseTime;	// in ms
} Params;

typedef struct {
	int8_t noiseGateIsActive;
	int8_t expanderIsActive;
	int8_t compressorIsActive;
	int8_t limiterIsActive;

	F32x2 noiseThr;					// Q31
	F32x2 expanderHighThr;			// Q31
	F32x2 compressorLowThr;			// Q31
	F32x2 limiterThr;				// Q31

	F32x2 expanderC1;				// Q27
	F32x2 expanderC2;				// Q27
	F32x2 compressorC1;				// Q27
	F32x2 compressorC2;				// Q27

	F32x2 samplesAlphaAttackTime;	// Q31
	F32x2 samplesAlphaReleaseTime;	// Q31
	F32x2 gainAlphaAttackTime;		// Q31
	F32x2 gainAlphaReleaseTime;		// Q31
	F32x2 fadeAlphaAttackTime;		// Q31
	F32x2 fadeAlphaReleaseTime;		// Q31
} Coeffs;

Status AmplitudeProcInit(Params *params, Coeffs *coeffs, RingBuff *ringBuff,
		PrevValuesBuff *prevValuesBuff, const int sampleRate);
Status AmplitudeProcSetParam(Params *params, Coeffs *coeffs, const uint16_t id,
		const double val);
Status ringBuffSet(RingBuff *ringBuff, int32_t *dataBuff);
void AmplitudeProc_Process(const Coeffs *coeffs, RingBuff *ringBuff,
		PrevValuesBuff *prevValuesBuff);

#endif /* AMPLITUDE_PROC */
