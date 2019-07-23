/*
 * main.c
 *
 *  Created on: Jul 15, 2019
 *      Author: Intern_2
 */

//#define _CRT_SECURE_NO_WARNINGS

#include "Arithmetic.h"

#include <stdio.h>
#include <stdlib.h>

#define INPUT_FILE_NAME "Input0.5sec.wav"
#define OUTPUT_FILE_NAME "Output.wav"
#define FILE_HEADER_SIZE 44
#define BYTES_PER_SAMPLE 4
#define SAMPLE_RATE 48000
#define CHANNELS 2

//Thresholds in dB (double!)
#define NOISE_THR_dB 			-20
#define	EXPANDER_HIGH_THR_dB 	-6
#define COMPRESSOR_LOW_THR_dB 	-4
#define LIMITER_THR_dB 			-3

//Ratios must be bigger than 1 (double!)
#define EXPANDER_RATIO 		5.0
#define COMPRESSOR_RATIO 	9.0

#define RING_BUFF_SIZE 		128
#define DATA_BUFF_SIZE 		1024		//must be twice bigger than RING_BUFF_SIZE

#define DIV_PRECISION 		5			//be careful with too small values

#define PI 					3.14159265358979323846


typedef struct {
	uint16_t currNum;
	ae_f32x2 samples[RING_BUFF_SIZE];
	ae_f32x2 maxSample;
} RingBuff;

typedef struct {
	ae_f32x2 noiseThr;
	ae_f32x2 expanderHighThr;
	ae_f32x2 compressorLowThr;
	ae_f32x2 limiterThr;
	ae_f32x2 expanderRatio;
	ae_f32x2 compressorRatio;

	ae_f32x2 expanderC1;
	ae_f32x2 expanderC2;
	ae_f32x2 compressorC1;
	ae_f32x2 compressorC2;
} Coeffs;


ALWAYS_INLINE FILE * openFile(char *fileName, _Bool mode);		//if 0 - read, if 1 - write
ALWAYS_INLINE void readHeader(uint8_t *headerBuff, FILE *inputFilePtr);
ALWAYS_INLINE void writeHeader(uint8_t *headerBuff, FILE *outputFilePtr);

ALWAYS_INLINE void calcCoeffs(Coeffs *coeffs);
ALWAYS_INLINE void updateMaxRingBuffValue(RingBuff *ringBuff);
ALWAYS_INLINE void initRing(RingBuff *ringBuff, int32_t *samplesBuff);
NEVER_INLINE ae_f32x2 signalProc(RingBuff *ringBuff, const Coeffs *coeffs);
static inline void run(FILE *inputFilePtr, FILE *outputFilePtr, RingBuff *ringBuff, const Coeffs *coeffs);


int main()
{
	FILE *inputFilePtr = openFile(INPUT_FILE_NAME, 0);
	FILE *outputFilePtr = openFile(OUTPUT_FILE_NAME, 1);
	uint8_t headerBuff[FILE_HEADER_SIZE];
	RingBuff ringBuff;
	Coeffs coeffs;

	readHeader(headerBuff, inputFilePtr);
	writeHeader(headerBuff, outputFilePtr);

	calcCoeffs(&coeffs);
	run(inputFilePtr, outputFilePtr, &ringBuff, &coeffs);
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

ALWAYS_INLINE void calcCoeffs(Coeffs *coeffs)
{
	int32_t fixNoiseThr = doubleToFixed27(dBtoGain(NOISE_THR_dB));
	int32_t fixExpanderHighThr = doubleToFixed27(dBtoGain(EXPANDER_HIGH_THR_dB));
	int32_t fixCompressorLowThr = doubleToFixed27(dBtoGain(COMPRESSOR_LOW_THR_dB));
	int32_t fixLimiterThr = doubleToFixed27(dBtoGain(LIMITER_THR_dB));
	int32_t fixExpanderRatio = doubleToFixed27(dBtoGain(EXPANDER_RATIO));
	int32_t fixCompressorRatio = doubleToFixed27(dBtoGain(COMPRESSOR_RATIO));

	int32_t fixedOne = doubleToFixed27(1);
	ae_f32x2 vectorOne = int32ToF32x2(fixedOne, fixedOne);

	coeffs->noiseThr = int32ToF32x2(fixNoiseThr, fixNoiseThr);
	coeffs->expanderHighThr = int32ToF32x2(fixExpanderHighThr, fixExpanderHighThr);
	coeffs->compressorLowThr = int32ToF32x2(fixCompressorLowThr, fixCompressorLowThr);
	coeffs->limiterThr = int32ToF32x2(fixLimiterThr, fixLimiterThr);
	coeffs->expanderRatio = int32ToF32x2(fixExpanderRatio, fixExpanderRatio);
	coeffs->compressorRatio = int32ToF32x2(fixCompressorRatio, fixCompressorRatio);

	coeffs->expanderC1 = Sub(coeffs->expanderHighThr, DivQ27x2(coeffs->expanderHighThr, coeffs->expanderRatio));
	coeffs->expanderC2 = DivQ27x2(vectorOne, coeffs->expanderRatio);
	coeffs->compressorC1 = Sub(coeffs->compressorLowThr, DivQ27x2(coeffs->compressorLowThr, coeffs->compressorRatio));
	coeffs->compressorC2 = DivQ27x2(vectorOne, coeffs->compressorRatio);
}

ALWAYS_INLINE void updateMaxRingBuffValue(RingBuff *ringBuff)
{
	uint16_t i;

	for (i = 0; i < RING_BUFF_SIZE; i++)
	{
		ringBuff->maxSample = AE_MAX32(ringBuff->maxSample, ringBuff->samples[i]);
	}
}

ALWAYS_INLINE void initRing(RingBuff *ringBuff, int32_t *dataBuff)
{
	uint16_t i;
	ringBuff->currNum = 0;
	ringBuff->maxSample = AE_ZERO32();

	for (i = 0; i < RING_BUFF_SIZE; i++)
	{
		ringBuff->samples[i] = AE_MOVDA32X2(dataBuff[i * CHANNELS], dataBuff[i * CHANNELS + 1]);
		dataBuff[i * CHANNELS] = 0;
		//dataBuff[i * CHANNELS + 1] = 0;
	}

	updateMaxRingBuffValue(ringBuff);
}

NEVER_INLINE ae_f32x2 signalProc(RingBuff *ringBuff, const Coeffs *coeffs)
{
	ae_f32x2 gain = AE_MOVF32X2_FROMINT32X2(AE_MOVDA32X2(0x07ffffff, 0x07ffffff));
	ae_f32x2 limGain;
	ae_f32x2 maxSample27 = Q31ToQ27x2(ringBuff->maxSample);

	xtbool2 isExpander;
	xtbool2 isCompressor;
	xtbool2 isLimiter;
	xtbool2 isNoiseGate;
	xtbool2 isCalculated = xtbool_join_xtbool2(0, 0);


	//=== if NoiseGate ===
	isNoiseGate = AE_LT32(maxSample27, coeffs->noiseThr);
	AE_MOVT32X2(gain, 0, isNoiseGate);		//if isNoiseGate, gain = 0
	isCalculated = isNoiseGate;


	//=== if Limiter ===
	isLimiter = AE_LT32(coeffs->limiterThr, maxSample27);
	isLimiter = xtbool_join_xtbool2(
									XT_ANDB(
											xtbool2_extract_0(isLimiter),
											XT_XORB(xtbool2_extract_0(isCalculated), 1)
											),
									XT_ANDB(
											xtbool2_extract_1(isLimiter),
											XT_XORB(xtbool2_extract_1(isCalculated), 1)
											)
									);

	AE_MOVT32X2(limGain, DivQ27x2(coeffs->limiterThr, maxSample27), isLimiter);


	//=== if Expander ===
	isExpander = AE_LT32(maxSample27, coeffs->expanderHighThr);
	isExpander = xtbool_join_xtbool2(
									XT_ANDB(
											xtbool2_extract_0(isExpander),
											XT_XORB(xtbool2_extract_0(isCalculated), 1)
											),
									XT_ANDB(
											xtbool2_extract_1(isExpander),
											XT_XORB(xtbool2_extract_1(isCalculated), 1)
											)
									);
	isCalculated = xtbool_join_xtbool2(
										XT_ORB(
												xtbool2_extract_0(isCalculated),
												xtbool2_extract_0(isExpander)
												),
										XT_ORB(
												xtbool2_extract_1(isCalculated),
												xtbool2_extract_1(isExpander)
												)
										);

	AE_MOVT32X2(
				gain,
				Add(DivQ27x2(coeffs->expanderC1, maxSample27), coeffs->expanderC2),
				isExpander
				);


	//=== if Compressor ===
	isCompressor = AE_LE32(coeffs->compressorLowThr, maxSample27);

	isCompressor = xtbool_join_xtbool2(
									XT_ANDB(
											xtbool2_extract_0(isCompressor),
											XT_XORB(xtbool2_extract_0(isCalculated), 1)
											),
									XT_ANDB(
											xtbool2_extract_1(isCompressor),
											XT_XORB(xtbool2_extract_1(isCalculated), 1)
											)
									);
	isCalculated = xtbool_join_xtbool2(
										XT_ORB(
												xtbool2_extract_0(isCalculated),
												xtbool2_extract_0(isCompressor)
												),
										XT_ORB(
												xtbool2_extract_1(isCalculated),
												xtbool2_extract_1(isCompressor)
												)
										);

	AE_MOVT32X2(
				gain,
				Add(DivQ27x2(coeffs->compressorC1, maxSample27), coeffs->compressorC2),
				isCompressor
				);


	//=== if Limiter, Limiter = Compressor/Limiter min ===
	AE_MOVT32X2(gain, AE_MIN32(gain, limGain), isLimiter);

	return Q27ToQ31x2(MulQ27x2(Q31ToQ27x2(ringBuff->samples[ringBuff->currNum]), gain));
}

void run(FILE *inputFilePtr, FILE *outputFilePtr, RingBuff *ringBuff, const Coeffs *coeffs)
{
	int32_t dataBuff[DATA_BUFF_SIZE * CHANNELS];
	size_t samplesRead;
	uint32_t i;
	ae_f32x2 res;
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
								AE_MOVDA32X2(dataBuff[i * CHANNELS], dataBuff[i * CHANNELS + 1]);
			dataBuff[i * CHANNELS] = AE_MOVAD32_H(res);
			//dataBuff[i * CHANNELS + 1] = AE_MOVAD32_L(res);

			updateMaxRingBuffValue(ringBuff);
			ringBuff->currNum = (ringBuff->currNum + 1) & (RING_BUFF_SIZE - 1);
		}

		fwrite(dataBuff, BYTES_PER_SAMPLE, samplesRead, outputFilePtr);
	}
}
