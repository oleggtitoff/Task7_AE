/*
 * main.c
 *
 *  Created on: Jul 15, 2019
 *      Author: Intern_2
 */

#define _CRT_SECURE_NO_WARNINGS

#include "Arithmetic.h"

#include <stdio.h>
#include <stdlib.h>

#include <xtensa/tie/xt_hifi2.h>

#define ALWAYS_INLINE static inline __attribute__((always_inline))
#define NEVER_INLINE static __attribute__((noinline))

#define INPUT_FILE_NAME "Input.wav"
#define OUTPUT_FILE_NAME "Output.wav"
#define FILE_HEADER_SIZE 44
#define BYTES_PER_SAMPLE 4
#define SAMPLE_RATE 48000
#define CHANNELS 2

#define	EXPANDER_THRESHOLD 0.06
#define COMPRESSOR_THRESHOLD 0.5
#define LIMITER_THRESHOLD 0.9
#define RATIO 2

#define FIXED_EXPANDER_THRESHOLD (doubleToFixed31((EXPANDER_THRESHOLD) / (SIGNAL_PROC_DIVIDER)))
#define FIXED_COMPRESSOR_THRESHOLD (doubleToFixed31((COMPRESSOR_THRESHOLD) / (SIGNAL_PROC_DIVIDER)))
#define FIXED_LIMITER_THRESHOLD (doubleToFixed31((LIMITER_THRESHOLD) / (SIGNAL_PROC_DIVIDER)))
#define FIXED_RATIO (doubleToFixed31((RATIO) / (SIGNAL_PROC_DIVIDER)))

#define VECTOR_EXPANDER_THRESHOLD (int32ToF32x2(FIXED_EXPANDER_THRESHOLD, FIXED_EXPANDER_THRESHOLD))
#define VECTOR_COMPRESSOR_THRESHOLD (int32ToF32x2(FIXED_COMPRESSOR_THRESHOLD, FIXED_COMPRESSOR_THRESHOLD))
#define VECTOR_LIMITER_THRESHOLD (int32ToF32x2(FIXED_LIMITER_THRESHOLD, FIXED_LIMITER_THRESHOLD))
#define VECTOR_RATIO (int32ToF32x2(FIXED_RATIO, FIXED_RATIO))

#define RING_BUFF_SIZE 128
#define DATA_BUFF_SIZE 1000		//must be bigger than RING_BUFF_SIZE

#define DIV_PRECISION 5			//be careful with too small values
#define SIGNAL_PROC_DIVIDER 4	//for Q29

#define PI 3.14159265358979323846


typedef struct {
	uint16_t currNum;
	ae_f32x2 samples[RING_BUFF_SIZE];
} RingBuff;


static inline FILE * openFile(char *fileName, _Bool mode);		//if 0 - read, if 1 - write
static inline void readHeader(uint8_t *headerBuff, FILE *inputFilePtr);
static inline void writeHeader(uint8_t *headerBuff, FILE *outputFilePtr);

void ringInitialization(RingBuff *ringBuff, int32_t *samplesBuff);
NEVER_INLINE ae_f32x2 signalProc(RingBuff *ringBuff);
void run(FILE *inputFilePtr, FILE *outputFilePtr, RingBuff *ringBuff);


int main()
{
	FILE *inputFilePtr = openFile(INPUT_FILE_NAME, 0);
	FILE *outputFilePtr = openFile(OUTPUT_FILE_NAME, 1);
	uint8_t headerBuff[FILE_HEADER_SIZE];
	RingBuff ringBuff;

	readHeader(headerBuff, inputFilePtr);
	writeHeader(headerBuff, outputFilePtr);

	run(inputFilePtr, outputFilePtr, &ringBuff);
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

void ringInitialization(RingBuff *ringBuff, int32_t *dataBuff)
{
	int i;

	for (i = 0; i < RING_BUFF_SIZE; i++)
	{
		ringBuff->samples[i] = dataBuff[i];
	}

	ringBuff->currNum = 0;
}

NEVER_INLINE ae_f32x2 signalProc(RingBuff *ringBuff)
{
	ae_f32x2 maxSample = AE_ABS32S(ringBuff->samples[0]);
	uint16_t i;
	uint16_t index;
	int32_t fixedGain = doubleToFixed31(1.0 / SIGNAL_PROC_DIVIDER);
	ae_f32x2 gain = int32ToF32x2(fixedGain, fixedGain);
	ae_f32x2 res;
	ae_f32x2 tmp;

	xtbool2 isCalculated = xtbool_join_xtbool2(0, 0);
	xtbool2 isNoiseGate;
	xtbool2 isLimiter;

	for (i = 1; i < RING_BUFF_SIZE; i++)
	{
		maxSample = AE_MAX32(maxSample, AE_ABS32S(ringBuff->samples[i]));
	}

	index = ringBuff->currNum & (RING_BUFF_SIZE - 1);

	isNoiseGate = AE_LT32(maxSample, VECTOR_EXPANDER_THRESHOLD);
	AE_MOVT32X2(gain, 0, isNoiseGate);		//if isNoiseGate, gain = 0

	//isCalculated = isCalculated | isNoiseGate
	isCalculated = xtbool_join_xtbool2(
										XT_ORB(
												xtbool2_extract_0(isCalculated),
												xtbool2_extract_0(isNoiseGate)
												),
										XT_ORB(
												xtbool2_extract_1(isCalculated),
												xtbool2_extract_1(isNoiseGate)
												)
										);

	isLimiter = AE_LT32(VECTOR_LIMITER_THRESHOLD, maxSample);
	//if isLimiter, gain = LIMITER_TRESHOLD / maxSample
	AE_MOVT32X2(gain, Div(VECTOR_LIMITER_THRESHOLD, maxSample), isLimiter);

	//isCalculated = isCalculated | isLimiter
	isCalculated = xtbool_join_xtbool2(
										XT_ORB(
												xtbool2_extract_0(isCalculated),
												xtbool2_extract_0(isLimiter)
												),
										XT_ORB(
												xtbool2_extract_1(isCalculated),
												xtbool2_extract_1(isLimiter)
												)
										);

	ringBuff->currNum = (ringBuff->currNum + 1) & (RING_BUFF_SIZE - 1);

	return Mul(ringBuff->samples[index], gain);
}

void run(FILE *inputFilePtr, FILE *outputFilePtr, RingBuff *ringBuff)
{
	int32_t dataBuff[DATA_BUFF_SIZE * CHANNELS];
	size_t samplesRead;
	uint32_t i;
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
			ringInitialization(ringBuff, dataBuff);
			isFirstIteration = 0;
		}

		for (i = 0; i < samplesRead / CHANNELS; i++)
		{
			ringBuff->samples[ringBuff->currNum] = dataBuff[i * CHANNELS];
			dataBuff[i * CHANNELS] = AE_MOVAD32_H(signalProc(ringBuff));
		}

		fwrite(dataBuff, BYTES_PER_SAMPLE, samplesRead, outputFilePtr);
	}
}
