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

ALWAYS_INLINE F32x2 F32x2OldLog2(F32x2 x)
{
	// Input/Output in Q27
	F32x2 index = 0;
	F32x2 res = F32x2Zero();
	Boolx2 isZero = F32x2Equal(x, F32x2Zero());
	Boolx2 isCalculated = isZero;
	F32x2MovIfTrue(&res, F32x2Set(INT32_MIN), isZero);
	x = F32x2Abs(x);
	int8_t clsH = AE_NSAZ32_L(AE_SEL32_LH(x, x));
	int8_t clsL = AE_NSAZ32_L(x);
	F32x2 cls = F32x2Join(clsH, clsL);
	Boolx2 isBigger = Boolx2AND(F32x2LessThan(cls, F32x2Set(4)), Boolx2NOT(isCalculated));
	Boolx2 isSmaller = Boolx2AND(F32x2LessThan(F32x2Set(4), cls), Boolx2NOT(isCalculated));
	F32x2MovIfTrue(&x, F32x2RightShiftA_Apart(x, 4 - clsH, 4 - clsL), isBigger);
	F32x2MovIfTrue(&res, F32x2LeftShiftAS(F32x2Sub(F32x2Set(4), cls), 27), isBigger);
	F32x2MovIfTrue(&x, F32x2LeftShiftAS_Apart(x, clsH - 4, clsL - 4), isSmaller);
	F32x2MovIfTrue(&res, F32x2LeftShiftAS(F32x2Sub(F32x2Zero(), F32x2Sub(cls, F32x2Set(4))), 27), isSmaller);
	// Here 0x4000000 is min (first) value in log2InputsTable and 0x81020 is the step between values
	index = F32x2BuiltInDiv(F32x2Sub(x, F32x2Set(0x4000000)), F32x2Set(0x81020));
	return F32x2Add(res, F32x2Join(
									log2OutputsTable[(int)F32x2ToI32Extract_h(index)],
									log2OutputsTable[(int)F32x2ToI32Extract_l(index)]));
}

ALWAYS_INLINE F32x2 F32x2OldPowOf2(F32x2 x)
{
	// Input/Output in Q27
	F32x2 index = 0;
	F32x2 res = 0x8000000;
	F32x2 mask = F32x2Set(0x78000000);
	Boolx2 isNegative = F32x2LessThan(x, F32x2Zero());
	F32x2 count = F32x2AND(F32x2Abs(x), mask);
	F32x2 countShifted = F32x2RightShiftA(count, 27);
	F32x2MovIfTrue(&x, F32x2Sub(x, count), Boolx2NOT(isNegative));
	F32x2MovIfTrue(&res,
			F32x2LeftShiftAS_Apart(res, (int)F32x2ToI32Extract_h(countShifted), (int)F32x2ToI32Extract_l(countShifted)),
			Boolx2NOT(isNegative));
	F32x2MovIfTrue(&x, F32x2Add(x, count), isNegative);
	F32x2MovIfTrue(&res,
			F32x2RightShiftA_Apart(res, (int)F32x2ToI32Extract_h(countShifted), (int)F32x2ToI32Extract_l(countShifted)),
			isNegative);
	index = F32x2BuiltInDiv(x, F32x2Set(0x102040));
	F32x2MovIfTrue(&index, F32x2Add(F32x2Abs(index), F32x2Set(127)), F32x2LessThan(index, F32x2Zero()));
	return F32x2LeftShiftAS(F32x2Mul(res, F32x2Join(
										powOf2OutputsTable[(int)F32x2ToI32Extract_h(index)],
										powOf2OutputsTable[(int)F32x2ToI32Extract_l(index)])), 4);
}

ALWAYS_INLINE F32x2 F32x2OldPow(F32x2 x, F32x2 y)
{
	// Input/Output in Q27
	return F32x2OldPowOf2(F32x2LeftShiftAS(F32x2Mul(y, F32x2OldLog2(x)), 4));
}


double callLog2Diff(double x)
{
	F32x2 fixres = F32x2Log2(doubleToF32x2Set(x / 16));
	double dres = log2(x);
	double err = fabs(dres - (F32x2ToDoubleExtract_h(fixres) * 16));

	printf("log:\n");
	printf("fixed: %.30f\n", F32x2ToDoubleExtract_h(fixres) * 16);
	printf("double: %.30f\n", dres);
	printf("err: %.30f\n", err);

	return err;
}

double callPowOf2Diff(double x)
{
	F32x2 fixres = F32x2PowOf2(doubleToF32x2Set(x / 16));
	double dres = pow(2.0, x);
	double err = fabs(dres - (F32x2ToDoubleExtract_h(fixres) * 16));

	printf("log:\n");
	printf("fixed: %.30f\n", F32x2ToDoubleExtract_h(fixres) * 16);
	printf("double: %.30f\n", dres);
	printf("err: %.30f\n", err);

	return err;
}

double callPowDiff(double x, double y)
{
	F32x2 fixres = F32x2Pow(doubleToF32x2Set(x / 16), doubleToF32x2Set(y / 16));
	double dres = pow(x, y);
	double err = fabs(dres - (F32x2ToDoubleExtract_h(fixres) * 16));

	printf("log:\n");
	printf("fixed: %.30f\n", F32x2ToDoubleExtract_h(fixres) * 16);
	printf("double: %.30f\n", dres);
	printf("err: %.30f\n", err);

	return err;
}


F32x2 callOldLog2(F32x2 x)
{
	return F32x2OldLog2(x);
}

F32x2 callOldPowOf2(F32x2 x)
{
	return F32x2OldPowOf2(x);
}

F32x2 callOldPow(F32x2 x, F32x2 y)
{
	return F32x2OldPow(x, y);
}


F32x2 callLog2(F32x2 x)
{
	return F32x2Log2(x);
}

F32x2 callPowOf2(F32x2 x)
{
	return F32x2PowOf2(x);
}

F32x2 callPow(F32x2 x, F32x2 y)
{
	return F32x2Pow(x, y);
}

int main()
{
//// F32x2Pow test here:
//	double x1 = 2.36851 / 16;
//	double x2 = 0.6846 / 16;
//	double y1 = 1.6732 / 16;
//	double y2 = 0.31003 / 16;
//	F32x2 x = doubleToF32x2Join(x1, x2);
//	F32x2 y = doubleToF32x2Join(y1, y2);
//	F32x2 res = callPowOf2(x, y);
//	double z1 = F32x2ToDoubleExtract_h(res) * 16;
//	double z2 = F32x2ToDoubleExtract_l(res) * 16;

	double x = 13.36851;
	double y = 0.0006846;
	double z = 1.6732;
	double k = -5.31003;

	printf("log: %.30f\n", callPowDiff(x, y));
//	printf("powOf2: %.30f\n", callPowOf2Diff(x));
//	printf("pow: %.30f\n", callPowDiff(x, y));

//	FILE *inputFilePtr = openFile(INPUT_FILE_NAME, 0);
//	FILE *outputFilePtr = openFile(OUTPUT_FILE_NAME, 1);
//	uint8_t headerBuff[FILE_HEADER_SIZE];
//	Params params;
//	Coeffs coeffs;
//	RingBuff ringBuff;
//	States states;
//
//	readHeader(headerBuff, inputFilePtr);
//	writeHeader(headerBuff, outputFilePtr);
//
//	AmplitudeProcInit(&params, &coeffs, &ringBuff, &states);
//	setParams(&params, &coeffs, &states);
//
//	run(inputFilePtr, outputFilePtr, &ringBuff, &coeffs, &states);
//
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

void setParams(Params *params, Coeffs *coeffs, States *states)
{
	AmplitudeProcSetParam(params, coeffs, states, sampleRateID, SAMPLE_RATE);

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
