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
#define SIGNAL_PROC_DIVIDER 4	//for Q29

#define FIXED_EXPANDER_THRESHOLD (doubleToFixed31(EXPANDER_THRESHOLD))
#define FIXED_COMPRESSOR_THRESHOLD (doubleToFixed31(COMPRESSOR_THRESHOLD))
#define FIXED_LIMITER_THRESHOLD (doubleToFixed31(LIMITER_THRESHOLD))
#define FIXED_RATIO (doubleToFixed31((double)(RATIO) / (SIGNAL_PROC_DIVIDER)))

#define VECTOR_EXPANDER_THRESHOLD (int32ToF32x2(FIXED_EXPANDER_THRESHOLD, FIXED_EXPANDER_THRESHOLD))
#define VECTOR_COMPRESSOR_THRESHOLD (int32ToF32x2(FIXED_COMPRESSOR_THRESHOLD, FIXED_COMPRESSOR_THRESHOLD))
#define VECTOR_LIMITER_THRESHOLD (int32ToF32x2(FIXED_LIMITER_THRESHOLD, FIXED_LIMITER_THRESHOLD))
#define VECTOR_RATIO (int32ToF32x2(FIXED_RATIO, FIXED_RATIO))

#define RING_BUFF_SIZE 128
#define DATA_BUFF_SIZE 1024		//must be twice bigger than RING_BUFF_SIZE

#define DIV_PRECISION 5			//be careful with too small values

#define PI 3.14159265358979323846


typedef struct {
	uint16_t currNum;
	ae_f32x2 samples[RING_BUFF_SIZE];
	ae_f32x2 maxSample;
} RingBuff;


static inline FILE * openFile(char *fileName, _Bool mode);		//if 0 - read, if 1 - write
static inline void readHeader(uint8_t *headerBuff, FILE *inputFilePtr);
static inline void writeHeader(uint8_t *headerBuff, FILE *outputFilePtr);

ALWAYS_INLINE void updateMaxRingBuffValue(RingBuff *ringBuff);
ALWAYS_INLINE void ringInitialization(RingBuff *ringBuff, int32_t *samplesBuff);
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

ALWAYS_INLINE void updateMaxRingBuffValue(RingBuff *ringBuff)
{
	uint16_t i;

	for (i = 0; i < RING_BUFF_SIZE; i++)
	{
		ringBuff->maxSample = AE_MAX32(ringBuff->maxSample, ringBuff->samples[i]);
	}
}

ALWAYS_INLINE void ringInitialization(RingBuff *ringBuff, int32_t *dataBuff)
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

NEVER_INLINE ae_f32x2 signalProc(RingBuff *ringBuff)
{
	uint16_t i;
	ae_f32x2 gain;// = AE_MOVDA32X2(0x20000000, 0x20000000);
	ae_f32x2 res = ringBuff->maxSample;

	xtbool2 isExpander;
	xtbool2 isCompressor;
	xtbool2 isLimiter;
	xtbool2 isNoiseGate;
	xtbool2 isCalculated;
	xtbool2 resIsTooBig;

	//=== if NoiseGate ===
	isNoiseGate = AE_LT32(ringBuff->maxSample, VECTOR_EXPANDER_THRESHOLD);
	AE_MOVT32X2(gain, 0, isNoiseGate);		//if isNoiseGate, gain = 0
	isCalculated = isNoiseGate;

	//=== if Limiter ===
	isLimiter = AE_LT32(VECTOR_LIMITER_THRESHOLD, res);

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

	//if isLimiter, gain = LIMITER_TRESHOLD / maxSample
	AE_MOVT32X2(gain, Div(VECTOR_LIMITER_THRESHOLD, res), isLimiter);


	//=== if Expander ===
	isExpander = AE_LT32(ringBuff->maxSample, VECTOR_COMPRESSOR_THRESHOLD);

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

	AE_MOVT32X2(res, Mul(ringBuff->maxSample, VECTOR_RATIO), isExpander);

	resIsTooBig = AE_LT32(RightShiftA_32x2(VECTOR_LIMITER_THRESHOLD, 2), res);
	AE_MOVT32X2(res, RightShiftA_32x2(VECTOR_LIMITER_THRESHOLD, 2), resIsTooBig);

	//=== if Compressor ===
	isCompressor = AE_LE32(VECTOR_COMPRESSOR_THRESHOLD, ringBuff->maxSample);

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

	AE_MOVT32X2(res, Div(ringBuff->maxSample, VECTOR_RATIO), isCompressor);

	AE_MOVT32X2(gain, Div(res, ringBuff->maxSample), isExpander);
	AE_MOVT32X2(gain, Div(res, ringBuff->maxSample), isCompressor);

	ae_f32x2 result = Mul(ringBuff->samples[ringBuff->currNum], gain);
	AE_MOVT32X2(result, LeftShiftA_32x2(result, 2), isExpander);
	AE_MOVT32X2(result, RightShiftA_32x2(result, 2), isCompressor);

	return result;
}

void run(FILE *inputFilePtr, FILE *outputFilePtr, RingBuff *ringBuff)
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
			ringInitialization(ringBuff, dataBuff);
			i = RING_BUFF_SIZE;
			isFirstIteration = 0;
		}
		else
		{
			i = 0;
		}

		for (i; i < samplesRead / CHANNELS; i++)
		{
			res = signalProc(ringBuff);
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
