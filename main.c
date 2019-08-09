/*
 * main.c
 *
 *  Created on: Jul 15, 2019
 *      Author: Intern_2
 */

#include "ExternalAndInternalTypesConverters.h"
#include "GeneralArithmetic.h"
#include "InternalTypesArithmetic.h"
#include "AmplitudeProc.h"

#include <stdio.h>
#include <stdlib.h>

#define INPUT_FILE_NAME "DiffAmplitudeTest.wav"
#define OUTPUT_FILE_NAME "Output.wav"
#define FILE_HEADER_SIZE 44
#define BYTES_PER_SAMPLE 4
#define SAMPLE_RATE 48000
#define CHANNELS 2

// Thresholds in dB (double!)
#define NOISE_THR_dB 			-20
#define	EXPANDER_HIGH_THR_dB 	-7
#define COMPRESSOR_LOW_THR_dB 	-5
#define LIMITER_THR_dB 			-2.5

// Active flags
#define NOISE_GATE_IS_ACTIVE	1
#define EXPANDER_IS_ACTIVE		1
#define	COMPRESSOR_IS_ACTIVE	1
#define LIMITER_IS_ACTIVE		1

// Ratios
#define EXPANDER_RATIO			0.75
#define COMPRESSOR_RATIO		1.75

#define SAMPLES_ATTACK_TIME_ms	0.0001
#define SAMPLES_RELEASE_TIME_ms	30
#define GAIN_ATTACK_TIME_ms		1
#define GAIN_RELEASE_TIME_ms	1
#define FADE_ATTACK_TIME_ms		1
#define FADE_RELEASE_TIME_ms	1

#define SAMPLES_ATTACK_TIME		0.0000001
#define SAMPLES_RELEASE_TIME	0.03
#define GAIN_ATTACK_TIME		0.001
#define GAIN_RELEASE_TIME		0.001
#define FADE_ATTACK_TIME		0.001
#define FADE_RELEASE_TIME		0.001

#define RING_BUFF_SIZE 		128
#define DATA_BUFF_SIZE 		1024	//must be twice bigger than RING_BUFF_SIZE


//typedef struct {
//	uint16_t currNum;
//	F32x2 samples[RING_BUFF_SIZE];	// Q31
//	F32x2 maxSample;				// Q31
//	_Bool isFade;
//} RingBuff;
//
//typedef struct {
//	double noiseThr;
//	double expanderHighThr;
//	double compressorLowThr;
//	double limiterThr;
//	double expanderRatio;
//	double compressorRatio;
//} Params;
//
//typedef struct {
//	F32x2 noiseThr;					// Q31
//	F32x2 expanderHighThr;			// Q31
//	F32x2 compressorLowThr;			// Q31
//	F32x2 limiterThr;				// Q31
//
//	F32x2 expanderC1;				// Q31
//	F32x2 expanderC2;				// Q31
//	F32x2 compressorC1;				// Q31
//	F32x2 compressorC2;				// Q31
//
//	F32x2 samplesAlphaAttack;
//	F32x2 samplesAlphaRelease;
//	F32x2 gainAlphaAttack;
//	F32x2 gainAlphaRelease;
//	F32x2 fadeAlphaAttack;
//	F32x2 fadeAlphaRelease;
//} Coeffs;


ALWAYS_INLINE FILE * openFile(char *fileName, _Bool mode);		//if 0 - read, if 1 - write
ALWAYS_INLINE void readHeader(uint8_t *headerBuff, FILE *inputFilePtr);
ALWAYS_INLINE void writeHeader(uint8_t *headerBuff, FILE *outputFilePtr);

ALWAYS_INLINE void calcCoeffs(Params *params, Coeffs *coeffs);
ALWAYS_INLINE void updateMaxRingBuffValue(RingBuff *ringBuff);
ALWAYS_INLINE void initRing(RingBuff *ringBuff, int32_t *samplesBuff);
NEVER_INLINE F32x2 signalProc1(RingBuff *ringBuff, const Coeffs *coeffs);
static inline void run(FILE *inputFilePtr, FILE *outputFilePtr, RingBuff *ringBuff,
						const Coeffs *coeffs, PrevValuesBuff *prevValuesBuff);
void setParams(Params *params, Coeffs *coeffs);


int main()
{
//	double x1 = -2.978 / 16;
//	double x2 = 5.79 / 16;
//	double y1 = -2.0 / 16;
//	double y2 = -0.3 / 16;
//	F32x2 x = doubleToF32x2Join(x1, x2);
//	F32x2 y = doubleToF32x2Join(y1, y2);
//	F32x2 res = F32x2Pow(x, y);
//	double z1 = F32x2ToDoubleExtract_h(res) * 16;
//	double z2 = F32x2ToDoubleExtract_l(res) * 16;

	FILE *inputFilePtr = openFile(INPUT_FILE_NAME, 0);
	FILE *outputFilePtr = openFile(OUTPUT_FILE_NAME, 1);
	uint8_t headerBuff[FILE_HEADER_SIZE];
	Params params;
	Coeffs coeffs;
	RingBuff ringBuff;
	PrevValuesBuff prevValuesBuff;


	readHeader(headerBuff, inputFilePtr);
	writeHeader(headerBuff, outputFilePtr);

	AmplitudeProcInit(&params, &coeffs, &ringBuff, &prevValuesBuff, SAMPLE_RATE);
	setParams(&params, &coeffs);

	run(inputFilePtr, outputFilePtr, &ringBuff, &coeffs, &prevValuesBuff);

	fclose(inputFilePtr);
	fclose(outputFilePtr);

	return 0;
}


FILE * openFile(char *fileName, _Bool mode)		//if 0 - read, if 1 - write
{
	FILE *filePtr;

	if (mode == 0)
	{
		if ((filePtr = fopen(fileName, "rb")) == NULL)
		{
			printf("Error opening input file\n");
			system("pause");
			exit(0);
		}
	}
	else
	{
		if ((filePtr = fopen(fileName, "wb")) == NULL)
		{
			printf("Error opening output file\n");
			system("pause");
			exit(0);
		}
	}

	return filePtr;
}

static inline void readHeader(uint8_t *headerBuff, FILE *inputFilePtr)
{
	if (fread(headerBuff, FILE_HEADER_SIZE, 1, inputFilePtr) != 1)
	{
		printf("Error reading input file (header)\n");
		system("pause");
		exit(0);
	}
}

static inline void writeHeader(uint8_t *headerBuff, FILE *outputFilePtr)
{
	if (fwrite(headerBuff, FILE_HEADER_SIZE, 1, outputFilePtr) != 1)
	{
		printf("Error writing output file (header)\n");
		system("pause");
		exit(0);
	}
}

void setParams(Params *params, Coeffs *coeffs)
{
	AmplitudeProcSetParam(params, coeffs, noiseGateIsActiveID, NOISE_GATE_IS_ACTIVE);
	AmplitudeProcSetParam(params, coeffs, expanderIsActiveID, EXPANDER_IS_ACTIVE);
	AmplitudeProcSetParam(params, coeffs, compressorIsActiveID, COMPRESSOR_IS_ACTIVE);
	AmplitudeProcSetParam(params, coeffs, limiterIsActiveID, LIMITER_IS_ACTIVE);

	AmplitudeProcSetParam(params, coeffs, noiseThrID, NOISE_THR_dB);
	AmplitudeProcSetParam(params, coeffs, expanderHighThrID, EXPANDER_HIGH_THR_dB);
	AmplitudeProcSetParam(params, coeffs, compressorLowThrID, COMPRESSOR_LOW_THR_dB);
	AmplitudeProcSetParam(params, coeffs, limiterThrID, LIMITER_THR_dB);

	AmplitudeProcSetParam(params, coeffs, expanderRatioID, EXPANDER_RATIO);
	AmplitudeProcSetParam(params, coeffs, compressorRatioID, COMPRESSOR_RATIO);

	AmplitudeProcSetParam(params, coeffs, samplesAttackTimeID, SAMPLES_ATTACK_TIME_ms);
	AmplitudeProcSetParam(params, coeffs, samplesReleaseTimeID, SAMPLES_RELEASE_TIME_ms);
	AmplitudeProcSetParam(params, coeffs, gainAttackTimeID, GAIN_ATTACK_TIME_ms);
	AmplitudeProcSetParam(params, coeffs, gainReleaseTimeID, GAIN_RELEASE_TIME_ms);
	AmplitudeProcSetParam(params, coeffs, fadeAttackTimeID, FADE_ATTACK_TIME_ms);
	AmplitudeProcSetParam(params, coeffs, fadeReleaseTimeID, FADE_RELEASE_TIME_ms);
}

ALWAYS_INLINE void calcCoeffs(Params *params, Coeffs *coeffs)
{
	params->noiseThr 			= dBtoGain(NOISE_THR_dB);
	params->expanderHighThr 	= dBtoGain(EXPANDER_HIGH_THR_dB);
	params->compressorLowThr 	= dBtoGain(COMPRESSOR_LOW_THR_dB);
	params->limiterThr 			= dBtoGain(LIMITER_THR_dB);
	params->expanderRatio 		= dBtoGain(EXPANDER_RATIO);
	params->compressorRatio 	= dBtoGain(COMPRESSOR_RATIO);

	coeffs->noiseThr 			= doubleToF32x2Set(params->noiseThr);
	coeffs->expanderHighThr 	= doubleToF32x2Set(params->expanderHighThr);
	coeffs->compressorLowThr 	= doubleToF32x2Set(params->compressorLowThr);
	coeffs->limiterThr 			= doubleToF32x2Set(params->limiterThr);

	coeffs->expanderC1 = doubleToF32x2Set((params->expanderHighThr -
										(params->expanderHighThr / params->expanderRatio)));
	coeffs->expanderC2 = doubleToF32x2Set((1 / params->expanderRatio));

	coeffs->compressorC1 = doubleToF32x2Set((params->compressorLowThr -
											(params->compressorLowThr / params->compressorRatio)));
	coeffs->compressorC2 = doubleToF32x2Set((1 / params->compressorRatio));
}

ALWAYS_INLINE void updateMaxRingBuffValue(RingBuff *ringBuff)
{
	uint16_t i;

	for (i = 0; i < RING_BUFF_SIZE; i++)
	{
		ringBuff->maxSample = F32x2Max(ringBuff->maxSample, ringBuff->samples[i]);
	}
}

ALWAYS_INLINE void initRing(RingBuff *ringBuff, int32_t *dataBuff)
{
	uint16_t i;
	ringBuff->currNum = 0;
	ringBuff->maxSample = F32x2Zero();

	for (i = 0; i < RING_BUFF_SIZE; i++)
	{
		ringBuff->samples[i] = F32x2Join(dataBuff[i * CHANNELS], dataBuff[i * CHANNELS + 1]);
		dataBuff[i * CHANNELS] = 0;
		//dataBuff[i * CHANNELS + 1] = 0;
	}
}

//NEVER_INLINE F32x2 signalProc1(RingBuff *ringBuff, const Coeffs *coeffs)
//{
//	updateMaxRingBuffValue(ringBuff);
//
//	int8_t expH = 0;
//	int8_t expL = 0;
//
//	F32x2 gain = F32x2Set(0x7fffffff);
//	F32x2 limGain;
//	F32x2 maxSample = ringBuff->maxSample;
//
//	Boolx2 isExpander;
//	Boolx2 isCompressor;
//	Boolx2 isLimiter;
//	Boolx2 isNoiseGate;
//	Boolx2 isCalculated = Boolx2Set(0);
//
//
////	//=== if NoiseGate ===
////	isNoiseGate = F32x2LessThan(maxSample, coeffs->noiseThr);
////	F32x2MovIfTrue(&gain, 0, isNoiseGate);		//if isNoiseGate, gain = 0
////	isCalculated = isNoiseGate;
//
//
//	//=== if Limiter ===
//	isLimiter = F32x2LessThan(coeffs->limiterThr, maxSample);
//	isLimiter = Boolx2AND(isLimiter, Boolx2NOT(isCalculated));
//
//	if ((int8_t)isLimiter != 0)
//	{
//		F32x2MovIfTrue(&limGain, F32x2DivExp(coeffs->limiterThr, maxSample, &expH, &expL), isLimiter);
//	}
//
////	//=== if Expander ===
////	isExpander = F32x2LessThan(maxSample, coeffs->expanderHighThr);
////	isExpander = Boolx2AND(isExpander, Boolx2NOT(isCalculated));
////	isCalculated = Boolx2OR(isCalculated, isExpander);
////
////	if((int8_t)isExpander != 0)
////	{
////		F32x2MovIfTrue(
////						&gain,
////						F32x2Add(F32x2DivExp(coeffs->expanderC1, maxSample, &expH, &expL), coeffs->expanderC2),
////						isExpander
////						);
////	}
////
////
////	//=== if Compressor ===
////	isCompressor = F32x2LessEqual(coeffs->compressorLowThr, maxSample);
////	isCompressor = Boolx2AND(isCompressor, Boolx2NOT(isCalculated));
////	isCalculated = Boolx2OR(isCalculated, isCompressor);
////
////	if ((int8_t)isCompressor != 0)
////	{
////		F32x2MovIfTrue(
////						&gain,
////						F32x2Add(F32x2DivExp(coeffs->compressorC1, maxSample, &expH, &expL), coeffs->compressorC2),
////						isCompressor
////						);
////	}
//
//
//	//=== if Limiter, Limiter = Compressor/Limiter min ===
//	F32x2MovIfTrue(&gain, F32x2Min(gain, limGain), isLimiter);
//
//	return F32x2Mul(ringBuff->samples[ringBuff->currNum], gain);
//}

void run(FILE *inputFilePtr, FILE *outputFilePtr, RingBuff *ringBuff,
		 const Coeffs *coeffs, PrevValuesBuff *prevValuesBuff)
{
	int32_t dataBuff[DATA_BUFF_SIZE * CHANNELS];
	size_t samplesRead;
	uint32_t i;
	F32x2 res;					// Q31
	_Bool isFirstIteration = 1;

	while (1)
	{
		samplesRead = fread(dataBuff, BYTES_PER_SAMPLE, DATA_BUFF_SIZE * CHANNELS, inputFilePtr);

		if (!samplesRead)
		{
			break;
		}

		if (isFirstIteration)
		{
			ringBuffSet(ringBuff, dataBuff);
			i = RING_BUFF_SIZE;
			isFirstIteration = 0;
		}
		else
		{
			i = 0;
		}

		for (i; i < samplesRead / CHANNELS; i++)
		{
			AmplitudeProc_Process(coeffs, ringBuff, prevValuesBuff);
			res = ringBuff->samples[ringBuff->currNum];

			ringBuff->samples[ringBuff->currNum] =
						F32x2Join(dataBuff[i * CHANNELS], dataBuff[i * CHANNELS + 1]);

			dataBuff[i * CHANNELS] = F32x2ToI32Extract_h(res);
			dataBuff[i * CHANNELS + 1] = F32x2ToI32Extract_l(res);

			ringBuff->currNum = (ringBuff->currNum + 1) & (RING_BUFF_SIZE - 1);
		}

		fwrite(dataBuff, BYTES_PER_SAMPLE, samplesRead, outputFilePtr);
	}
}
