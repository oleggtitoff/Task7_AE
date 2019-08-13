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

#define INPUT_FILE_NAME "EnvelopeTestSound.wav"
#define OUTPUT_FILE_NAME "Output.wav"
#define FILE_HEADER_SIZE 44
#define BYTES_PER_SAMPLE 4
#define SAMPLE_RATE 48000
#define CHANNELS 2

// Thresholds in dB
#define NOISE_THR_dB 			-70
#define	EXPANDER_HIGH_THR_dB 	-7
#define COMPRESSOR_LOW_THR_dB 	-5
#define LIMITER_THR_dB 			-4.0

// Active flags
#define NOISE_GATE_IS_ACTIVE	1
#define EXPANDER_IS_ACTIVE		1
#define	COMPRESSOR_IS_ACTIVE	1
#define LIMITER_IS_ACTIVE		1

// Ratios
#define EXPANDER_RATIO			0.65
#define COMPRESSOR_RATIO		2.0

// Attack/Release times in ms
#define ENVELOPE_ATTACK_TIME_ms		0.0001
#define ENVELOPE_RELEASE_TIME_ms	50
#define EXPANDER_ATTACK_TIME_ms		1
#define EXPANDER_RELEASE_TIME_ms	1
#define COMPRESSOR_ATTACK_TIME_ms	1
#define COMPRESSOR_RELEASE_TIME_ms	1

#define RING_BUFF_SIZE 		128
#define DATA_BUFF_SIZE 		1024	//must be twice bigger than RING_BUFF_SIZE


ALWAYS_INLINE FILE * openFile(char *fileName, _Bool mode);		//if 0 - read, if 1 - write
ALWAYS_INLINE void readHeader(uint8_t *headerBuff, FILE *inputFilePtr);
ALWAYS_INLINE void writeHeader(uint8_t *headerBuff, FILE *outputFilePtr);

ALWAYS_INLINE void calcCoeffs(Params *params, Coeffs *coeffs);
ALWAYS_INLINE void updateMaxRingBuffValue(RingBuff *ringBuff);
ALWAYS_INLINE void initRing(RingBuff *ringBuff, int32_t *samplesBuff);
NEVER_INLINE F32x2 signalProc1(RingBuff *ringBuff, const Coeffs *coeffs);
static inline void run(FILE *inputFilePtr, FILE *outputFilePtr, RingBuff *ringBuff,
						const Coeffs *coeffs, States *states);
void setParams(Params *params, Coeffs *coeffs, States *states);


int main()
{
// F32x2Pow test here:
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
	States states;

	readHeader(headerBuff, inputFilePtr);
	writeHeader(headerBuff, outputFilePtr);

	AmplitudeProcInit(&params, &coeffs, &ringBuff, &states, SAMPLE_RATE);
	setParams(&params, &coeffs, &states);

	run(inputFilePtr, outputFilePtr, &ringBuff, &coeffs, &states);

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

void setParams(Params *params, Coeffs *coeffs, States *states)
{
	AmplitudeProcSetParam(params, coeffs, states, noiseGateIsActiveID, NOISE_GATE_IS_ACTIVE);
	AmplitudeProcSetParam(params, coeffs, states, expanderIsActiveID, EXPANDER_IS_ACTIVE);
	AmplitudeProcSetParam(params, coeffs, states, compressorIsActiveID, COMPRESSOR_IS_ACTIVE);
	AmplitudeProcSetParam(params, coeffs, states, limiterIsActiveID, LIMITER_IS_ACTIVE);

	AmplitudeProcSetParam(params, coeffs, states, noiseThrID, NOISE_THR_dB);
	AmplitudeProcSetParam(params, coeffs, states, expanderHighThrID, EXPANDER_HIGH_THR_dB);
	AmplitudeProcSetParam(params, coeffs, states, compressorLowThrID, COMPRESSOR_LOW_THR_dB);
	AmplitudeProcSetParam(params, coeffs, states, limiterThrID, LIMITER_THR_dB);

	AmplitudeProcSetParam(params, coeffs, states, expanderRatioID, EXPANDER_RATIO);
	AmplitudeProcSetParam(params, coeffs, states, compressorRatioID, COMPRESSOR_RATIO);

	AmplitudeProcSetParam(params, coeffs, states, envelopeAttackTimeID, ENVELOPE_ATTACK_TIME_ms);
	AmplitudeProcSetParam(params, coeffs, states, envelopeReleaseTimeID, ENVELOPE_RELEASE_TIME_ms);
	AmplitudeProcSetParam(params, coeffs, states, expanderAttackTimeID, EXPANDER_ATTACK_TIME_ms);
	AmplitudeProcSetParam(params, coeffs, states, expanderReleaseTimeID, EXPANDER_RELEASE_TIME_ms);
	AmplitudeProcSetParam(params, coeffs, states, compressorAttackTimeID, COMPRESSOR_ATTACK_TIME_ms);
	AmplitudeProcSetParam(params, coeffs, states, compressorReleaseTimeID, COMPRESSOR_RELEASE_TIME_ms);
}

void run(FILE *inputFilePtr, FILE *outputFilePtr, RingBuff *ringBuff,
		 const Coeffs *coeffs, States *states)
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
			AmplitudeProc_Process(coeffs, ringBuff, states);
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
