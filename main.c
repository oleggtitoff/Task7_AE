/*
 * main.c
 *
 *  Created on: Jul 15, 2019
 *      Author: Intern_2
 */

#include "ExternalAndInternalTypesConverters.h"
#include "GeneralArithmetic.h"
#include "InternalTypesArithmetic.h"

#include <stdio.h>
#include <stdlib.h>

#define INPUT_FILE_NAME "Input0.5sec.wav"
#define OUTPUT_FILE_NAME "Output.wav"
#define FILE_HEADER_SIZE 44
#define BYTES_PER_SAMPLE 4
#define SAMPLE_RATE 48000
#define CHANNELS 2

// Thresholds in dB (double!)
#define NOISE_THR_dB 			-20
#define	EXPANDER_HIGH_THR_dB 	-6
#define COMPRESSOR_LOW_THR_dB 	-4
#define LIMITER_THR_dB 			-3

// Ratios must be bigger than 1 (double!)
#define EXPANDER_RATIO 		5.0
#define COMPRESSOR_RATIO 	9.0

#define RING_BUFF_SIZE 		128
#define DATA_BUFF_SIZE 		1024	//must be twice bigger than RING_BUFF_SIZE


typedef struct {
	uint16_t currNum;
	F32x2 samples[RING_BUFF_SIZE];	// Q31
	F32x2 maxSample;				// Q31
} RingBuff;

typedef struct {
	double noiseThr;
	double expanderHighThr;
	double compressorLowThr;
	double limiterThr;
	double expanderRatio;
	double compressorRatio;
} Params;

typedef struct {
	F32x2 noiseThr;					// Q27
	F32x2 expanderHighThr;			// Q27
	F32x2 compressorLowThr;			// Q27
	F32x2 limiterThr;				// Q27

	F32x2 expanderC1;				// Q27
	F32x2 expanderC2;				// Q27
	F32x2 compressorC1;				// Q27
	F32x2 compressorC2;				// Q27
} Coeffs;


ALWAYS_INLINE FILE * openFile(char *fileName, _Bool mode);		//if 0 - read, if 1 - write
ALWAYS_INLINE void readHeader(uint8_t *headerBuff, FILE *inputFilePtr);
ALWAYS_INLINE void writeHeader(uint8_t *headerBuff, FILE *outputFilePtr);

ALWAYS_INLINE void calcCoeffs(Params *params, Coeffs *coeffs);
ALWAYS_INLINE void updateMaxRingBuffValue(RingBuff *ringBuff);
ALWAYS_INLINE void initRing(RingBuff *ringBuff, int32_t *samplesBuff);
NEVER_INLINE F32x2 signalProc(RingBuff *ringBuff, const Coeffs *coeffs);
static inline void run(FILE *inputFilePtr, FILE *outputFilePtr, RingBuff *ringBuff, const Coeffs *coeffs);


int main()
{
	//TODO: Test AE_ZERO() speed

	//F32x2Div(F32x2Join(236534, 26543))

//	FILE *inputFilePtr = openFile(INPUT_FILE_NAME, 0);
//	FILE *outputFilePtr = openFile(OUTPUT_FILE_NAME, 1);
//	uint8_t headerBuff[FILE_HEADER_SIZE];
//	RingBuff ringBuff;
//	Coeffs coeffs;
//	Params params;
//
//	readHeader(headerBuff, inputFilePtr);
//	writeHeader(headerBuff, outputFilePtr);
//
//	calcCoeffs(&params, &coeffs);
//	run(inputFilePtr, outputFilePtr, &ringBuff, &coeffs);
//	fclose(inputFilePtr);
//	fclose(outputFilePtr);

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

ALWAYS_INLINE void calcCoeffs(Params *params, Coeffs *coeffs)
{
	params->noiseThr 			= dBtoGain(NOISE_THR_dB);
	params->expanderHighThr 	= dBtoGain(EXPANDER_HIGH_THR_dB);
	params->compressorLowThr 	= dBtoGain(COMPRESSOR_LOW_THR_dB);
	params->limiterThr 			= dBtoGain(LIMITER_THR_dB);
	params->expanderRatio 		= dBtoGain(EXPANDER_RATIO);
	params->compressorRatio 	= dBtoGain(COMPRESSOR_RATIO);

	coeffs->noiseThr 			= doubleToF32x2(params->noiseThr / 16.0);
	coeffs->expanderHighThr 	= doubleToF32x2(params->expanderHighThr / 16.0);
	coeffs->compressorLowThr 	= doubleToF32x2(params->compressorLowThr / 16.0);
	coeffs->limiterThr 			= doubleToF32x2(params->limiterThr / 16.0);

	coeffs->expanderC1 = doubleToF32x2((params->expanderHighThr -
										(params->expanderHighThr / params->expanderRatio)) /
										16.0);
	coeffs->expanderC2 = doubleToF32x2((1 / params->expanderRatio) / 16.0);

	coeffs->compressorC1 = doubleToF32x2((params->compressorLowThr -
											(params->compressorLowThr / params->compressorRatio)) /
										16.0);
	coeffs->compressorC2 = doubleToF32x2((1 / params->compressorRatio) / 16.0);
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

	updateMaxRingBuffValue(ringBuff);
}

NEVER_INLINE F32x2 signalProc(RingBuff *ringBuff, const Coeffs *coeffs)
{
	F32x2 gain = F32x2Set(0x07ffffff);
	F32x2 limGain;
	F32x2 maxSample27 = F32x2RightShiftA(ringBuff->maxSample, 4);

	Boolx2 isExpander;
	Boolx2 isCompressor;
	Boolx2 isLimiter;
	Boolx2 isNoiseGate;
	Boolx2 isCalculated = Boolx2Set(0);


	//=== if NoiseGate ===
	isNoiseGate = F32x2LessThan(maxSample27, coeffs->noiseThr);
	F32x2MovIfTrue(gain, 0, isNoiseGate);		//if isNoiseGate, gain = 0
	isCalculated = isNoiseGate;


	//=== if Limiter ===
	isLimiter = F32x2LessThan(coeffs->limiterThr, maxSample27);
	isLimiter = Boolx2AND(isLimiter, Boolx2NOT(isCalculated));
	F32x2MovIfTrue(limGain, F32x2Div(coeffs->limiterThr, maxSample27, 27), isLimiter);


	//=== if Expander ===
	isExpander = F32x2LessThan(maxSample27, coeffs->expanderHighThr);
	isExpander = Boolx2AND(isExpander, Boolx2NOT(isCalculated));
	isCalculated = Boolx2OR(isCalculated, isExpander);

	F32x2MovIfTrue(
				gain,
				F32x2Add(F32x2Div(coeffs->expanderC1, maxSample27, 27), coeffs->expanderC2),
				isExpander
				);


	//=== if Compressor ===
	isCompressor = F32x2LessEqual(coeffs->compressorLowThr, maxSample27);
	isCompressor = Boolx2AND(isCompressor, Boolx2NOT(isCalculated));
	isCalculated = Boolx2OR(isCalculated, isCompressor);

	F32x2MovIfTrue(
				gain,
				F32x2Add(F32x2Div(coeffs->compressorC1, maxSample27, 27), coeffs->compressorC2),
				isCompressor
				);


	//=== if Limiter, Limiter = Compressor/Limiter min ===
	F32x2MovIfTrue(gain, F32x2Min(gain, limGain), isLimiter);

	return F32x2LeftShiftAS(F32x2Mul(F32x2RightShiftA(ringBuff->samples[ringBuff->currNum], 4), gain), 4);
}

void run(FILE *inputFilePtr, FILE *outputFilePtr, RingBuff *ringBuff, const Coeffs *coeffs)
{
	int32_t dataBuff[DATA_BUFF_SIZE * CHANNELS];
	size_t samplesRead;
	uint32_t i;
	F32x2 res;
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
			initRing(ringBuff, dataBuff);
			i = RING_BUFF_SIZE;
			isFirstIteration = 0;
		}
		else
		{
			i = 0;
		}

		for (i; i < samplesRead / CHANNELS; i++)
		{
			res = signalProc(ringBuff, coeffs);
			ringBuff->samples[ringBuff->currNum] =
						F32x2Join(dataBuff[i * CHANNELS], dataBuff[i * CHANNELS + 1]);
			dataBuff[i * CHANNELS] = I32x2Extract_h(res);
			//dataBuff[i * CHANNELS + 1] = I32x2Extract_l(res);

			updateMaxRingBuffValue(ringBuff);
			ringBuff->currNum = (ringBuff->currNum + 1) & (RING_BUFF_SIZE - 1);
		}

		fwrite(dataBuff, BYTES_PER_SAMPLE, samplesRead, outputFilePtr);
	}
}
