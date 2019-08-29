/*
 * AmplitudeProc.c
 *
 *  Created on: Aug 8, 2019
 *      Author: Intern_2
 */

#include "AmplitudeProc.h"

ALWAYS_INLINE Status paramsInit(AmplitudeProcParams *params, int sampleRate)
{
	// initializes parameters into initial state

	if (!params)
		return statusError;

	params->sampleRate 				= sampleRate;

	params->noiseGateIsActive 		= 0;
	params->expanderIsActive 		= 0;
	params->compressorIsActive 		= 0;
	params->limiterIsActive 		= 0;

	double noiseThr 				= 0;
	double expanderHighThr 			= 0;
	double compressorLowThr 		= 0;
	double limiterThr 				= 0;

	double expanderRatio 			= 0;
	double compressorRatio 			= 0;

	double samplesAlphaAttackTime 	= 0;
	double samplesAlphaReleaseTime 	= 0;
	double gainAlphaAttackTime 		= 0;
	double gainAlphaReleaseTime 	= 0;
	double fadeAlphaAttackTime 		= 0;
	double fadeAlphaReleaseTime 	= 0;

	return statusOK;
}

ALWAYS_INLINE Status coeffsInit(AmplitudeProcCoeffs *coeffs)	//TODO
{
	// initializes coefficients into initial state

	if (!coeffs)
		return statusError;

	coeffs->noiseGate.isActive 	 	= 0;
	coeffs->expander.isActive 		= 0;
	coeffs->compressor.isActive 	= 0;
	coeffs->limiter.isActive 		= 0;

	coeffs->noiseGate.threshold 	= F32x2Zero();
	coeffs->expander.threshold 	 	= F32x2Zero();
	coeffs->compressor.threshold 	= F32x2Zero();
	coeffs->limiter.threshold 		= F32x2Zero();

	coeffs->expander.C1 			= F32x2Zero();
	coeffs->expander.C2 			= F32x2Zero();
	coeffs->compressor.C1 			= F32x2Zero();
	coeffs->compressor.C2 			= F32x2Zero();

	coeffs->envelope.alphaAttack 	= F32x2Zero();
	coeffs->envelope.alphaRelease 	= F32x2Zero();
	coeffs->expander.alphaAttack 	= F32x2Zero();
	coeffs->expander.alphaRelease 	= F32x2Zero();
	coeffs->compressor.alphaAttack  = F32x2Zero();
	coeffs->compressor.alphaRelease = F32x2Zero();

	return statusOK;
}

ALWAYS_INLINE Status rignBuffInit(RingBuff *ringBuff)
{
	// initializes ring buffer into initial state

	if (!ringBuff)
		return statusError;

	int16_t i;
	ringBuff->currNum = 0;
	ringBuff->maxSample = 0;

	for (i = 0; i < RING_BUFF_SIZE; i++)
	{
		ringBuff->samples[i] = 0;
	}

	return statusOK;
}

ALWAYS_INLINE Status statesInit(AmplitudeProcStates *states)
{
	// initializes states into initial state

	if (!states)
		return statusError;

	rignBuffInit(&states->ringBuff);

	states->envelope.prevSample = F32x2Zero();
	states->noiseGate.isWorked 	= Boolx2Set(0);
	states->compressor.isWorked = Boolx2Set(0);
	states->expander.isWorked 	= Boolx2Set(0);
	states->compressor.prevGain = F32x2Set(0x08000000);
	states->expander.prevGain 	= F32x2Set(0x08000000);

	return statusOK;
}

ALWAYS_INLINE void coeffsCalc(double *C1, double *C2, const double ratio,
		const double threshold, const int8_t type)
{
	// calculates coefficients for expander, or compressor
	// if type = 0, calculates for expander, else - for compressor

	if (!type)
	{
		*C1 = ((double)1 / ratio) - 1;
	}
	else
	{
		*C1 = (double)1 - ((double)1 / ratio);
	}

	*C2 = pow(dBtoGain(threshold), *C1);
}

ALWAYS_INLINE double alphaCalc(int sampleRate, double time)
{
	// calculates alpha coefficient from sample rate and time in ms

	return (double)1 - exp((double)-1 / (sampleRate * time / 1000));
}


ALWAYS_INLINE void resetDynamicRangeStates(CompressorStates *states)
{
	states->isWorked = Boolx2Set(0);
	states->prevGain = F32x2Set(0x08000000);
}

ALWAYS_INLINE void updateMaxRingBuffValue(RingBuff *ringBuff)
{
	// calculates and updates maxSample value in ring buffer

	uint16_t i;
	ringBuff->maxSample = F32x2Abs(ringBuff->samples[0]);

	for (i = 1; i < RING_BUFF_SIZE; i++)
	{
		ringBuff->maxSample = F32x2Max(ringBuff->maxSample, F32x2Abs(ringBuff->samples[i]));
	}
}


ALWAYS_INLINE F32x2 smoothingFilter(const F32x2 in, const F32x2 alpha, const F32x2 prev)
{
	// gets current value, previous value and alpha coefficient
	// returns filtered value

	F32x2 alpha2 = F32x2Sub(0x7fffffff, alpha);
	F32x2 mulRes1 = F32x2Mul(in, alpha);
	F32x2 mulRes2 = F32x2Mul(alpha2, prev);
	return F32x2Add(mulRes1, mulRes2);
}

ALWAYS_INLINE F32x2 envelopeCalc(const EnvelopeCoeffs *coeffs, EnvelopeStates *states,
								 const F32x2 sample)
{
	// calculates signal envelope

	F32x2 sampleABS = F32x2Abs(sample);			// Q27
	F32x2 alpha = coeffs->alphaAttack;
	Boolx2 isRelease = F32x2LessThan(sampleABS, F32x2Abs(states->prevSample));
	F32x2MovIfTrue(&alpha, coeffs->alphaRelease, isRelease);
	states->prevSample = smoothingFilter(sampleABS, alpha, states->prevSample);

	return states->prevSample;
}

ALWAYS_INLINE F32x2 instantGainCalc(const F32x2 envelope, const F32x2 C1, const F32x2 C2)
{
	// calculates instant gain value

	F32x2 negC1 	  	= F32x2Sub(F32x2Zero(), C1);
	F32x2 powRes	  	= F32x2Pow(envelope, negC1);
	F32x2 mulRes	 	= F32x2Mul(C2, powRes);
	F32x2 instantGain 	= F32x2LeftShiftAS(mulRes, 4);		// Q27

	return instantGain;
}

ALWAYS_INLINE F32x2 gainSmoothing(const F32x2 gain, const F32x2 alphaAttack,
								  const F32x2 alphaRelease, F32x2 *prevGain)
{
	F32x2 alpha = alphaAttack;
	F32x2MovIfTrue(&alpha, alphaRelease, F32x2LessThan(*prevGain, gain));

	return smoothingFilter(gain, alpha, *prevGain);
}


//ALWAYS_INLINE
static F32x2 noiseGate(const NoiseGateCoeffs *coeffs, NoiseGateStates *states, F32x2 envelope)
{
	// changes gain to zero, if envelope sample is under the threshold

	F32x2 gain = F32x2Set(0x08000000);		// Q27
	Boolx2 isNoiseGate = F32x2LessThan(envelope, coeffs->threshold);
	isNoiseGate = Boolx2AND(isNoiseGate, Boolx2Set(coeffs->isActive));
	F32x2MovIfTrue(&gain, F32x2Zero(), isNoiseGate);
	states->isWorked = isNoiseGate;

	return gain;
}

ALWAYS_INLINE F32x2 dynamicRangeCore(const CompressorCoeffs *coeffs, CompressorStates *states,
									const F32x2 envelope)
{
	F32x2 gain = instantGainCalc(envelope, coeffs->C1, coeffs->C2);		// Q27
	gain = gainSmoothing(gain, coeffs->alphaAttack, coeffs->alphaRelease, &states->prevGain);
	return gain;
}

//ALWAYS_INLINE
static F32x2 expander(const ExpanderCoeffs *coeffs, ExpanderStates *states,
							  const F32x2 envelope)
{
	F32x2 gain = F32x2Set(0x08000000);		// Q27
	states->isWorked = Boolx2Set(0);

	if (coeffs->isActive)
	{
		F32x2 expGain = dynamicRangeCore(coeffs, states, envelope);
		Boolx2 isExpander = F32x2LessThan(envelope, coeffs->threshold);
		F32x2MovIfTrue(&gain, expGain, isExpander);
		states->isWorked = isExpander;
	}

	states->prevGain = gain;
	return gain;
}

//ALWAYS_INLINE
static F32x2 compressor(const CompressorCoeffs *coeffs, CompressorStates *states,
							   const F32x2 envelope)
{
	F32x2 gain = F32x2Set(0x08000000);		// Q27
	states->isWorked = Boolx2Set(0);

	if (coeffs->isActive)
	{
		F32x2 comprGain = dynamicRangeCore(coeffs, states, envelope);
		Boolx2 isCompressor = F32x2LessThan(coeffs->threshold, envelope);
		F32x2MovIfTrue(&gain, comprGain, isCompressor);
		states->isWorked = isCompressor;
	}

	states->prevGain = gain;
	return gain;
}

//ALWAYS_INLINE
static F32x2 limiter(const LimiterCoeffs *coeffs, RingBuff *ringBuff)
{
	// changes gain to new limited gain, if maxSample in ring buffer with applied
	// current gain is over the threshold

	F32x2 gain = 0x08000000;		// Q27

	if (coeffs->isActive)
	{
		updateMaxRingBuffValue(ringBuff);
		Boolx2 isLimiter  = F32x2LessThan(coeffs->threshold, ringBuff->maxSample);
		F32x2 limGain = F32x2RightShiftA(F32x2Div(coeffs->threshold, ringBuff->maxSample), 4);
		F32x2MovIfTrue(&gain, limGain, isLimiter);
	}

	return gain;
}


Status AmplitudeProcInit(AmplitudeProcParams *params, AmplitudeProcCoeffs *coeffs,
						 AmplitudeProcStates *states, int sampleRate)
{
	// initializes parameters, coefficients and buffers into well-defined initial state

	Status status = 0;

	status |= paramsInit(params, sampleRate);
	status |= coeffsInit(coeffs);
	status |= statesInit(states);

	return status;
}

Status AmplitudeProcSetParam(AmplitudeProcParams *params, AmplitudeProcCoeffs *coeffs,
							 AmplitudeProcStates *states, const uint16_t id, double value)
{
	// sets one parameter by ID and appropriate coefficient,
	// calculates coefficient if needed

	Status status = statusOK;
	double C1, C2;

	if (!coeffs)
		return statusError;

	switch(id)
	{
	case noiseGateIsActiveID:
		params->noiseGateIsActive = (int)value;
		coeffs->noiseGate.isActive = (int)value;
		break;

	case expanderIsActiveID:
		if ((int)value == 0)
		{
			resetDynamicRangeStates(&states->expander);
		}

		params->expanderIsActive = (int)value;
		coeffs->expander.isActive = (int)value;
		break;

	case compressorIsActiveID:
		if ((int)value == 0)
		{
			resetDynamicRangeStates(&states->compressor);
		}

		params->compressorIsActive = (int)value;
		coeffs->compressor.isActive = (int)value;
		break;

	case limiterIsActiveID:
		params->limiterIsActive = (int)value;
		coeffs->limiter.isActive = (int)value;
		break;

	case noiseThrID:
		if (value > -70)
		{
			value = -70;
		}

		params->noiseThr = value;
		coeffs->noiseGate.threshold = doubleToF32x2Set(dBtoGain(value) / 16);
		break;

	case expanderHighThrID:
		coeffsCalc(&C1, &C2, params->expanderRatio, value, 0);
		params->expanderHighThr    = value;
		coeffs->expander.threshold = doubleToF32x2Set(dBtoGain(value) / 16);
		coeffs->expander.C2 	   = doubleToF32x2Set(C2 / 16);
		break;

	case compressorLowThrID:
		coeffsCalc(&C1, &C2, params->compressorRatio, value * 16, 1);
		params->compressorLowThr 	 = value;
		coeffs->compressor.threshold = doubleToF32x2Set(dBtoGain(value) / 16);
		coeffs->compressor.C2 	 	 = doubleToF32x2Set(C2 / 16);
		break;

	case limiterThrID:
		params->limiterThr = value;
		coeffs->limiter.threshold = doubleToF32x2Set(dBtoGain(value) / 16);
		break;

	case expanderRatioID:
		coeffsCalc(&C1, &C2, value, params->expanderHighThr, 0);
		params->expanderRatio = value;
		coeffs->expander.C1   = doubleToF32x2Set(C1 / 16);
		coeffs->expander.C2   = doubleToF32x2Set(C2 / 16);
		break;

	case compressorRatioID:
		coeffsCalc(&C1, &C2, value, params->compressorLowThr, 1);
		params->compressorRatio = value;
		coeffs->compressor.C1 	= doubleToF32x2Set(C1 / 16);
		coeffs->compressor.C2 	= doubleToF32x2Set(C2 / 16);
		break;

	case envelopeAttackTimeID:
		params->envelopeAttackTime = value;
		coeffs->envelope.alphaAttack = doubleToF32x2Set(alphaCalc(params->sampleRate, value));
		break;

	case envelopeReleaseTimeID:
		params->envelopeReleaseTime = value;
		coeffs->envelope.alphaRelease = doubleToF32x2Set(alphaCalc(params->sampleRate, value));
		break;

	case expanderAttackTimeID:
		params->expanderAttackTime = value;
		coeffs->expander.alphaAttack = doubleToF32x2Set(alphaCalc(params->sampleRate, value));
		break;

	case expanderReleaseTimeID:
		params->expanderReleaseTime = value;
		coeffs->expander.alphaRelease = doubleToF32x2Set(alphaCalc(params->sampleRate, value));
		break;

	case compressorAttackTimeID:
		params->compressorAttackTime = value;
		coeffs->compressor.alphaAttack = doubleToF32x2Set(alphaCalc(params->sampleRate, value));
		break;

	case compressorReleaseTimeID:
		params->compressorReleaseTime = value;
		coeffs->compressor.alphaRelease = doubleToF32x2Set(alphaCalc(params->sampleRate, value));
		break;

	default:
		status = statusError;
		break;
	}

	return status;
}

void ringBuffAddValue(RingBuff *ringBuff, F32x2 value)
{
	ringBuff->samples[ringBuff->currNum] = value;
	ringBuff->currNum = (ringBuff->currNum + 1) & (RING_BUFF_SIZE - 1);
}

F32x2 AmplitudeProc_Process(const AmplitudeProcCoeffs *coeffs, AmplitudeProcStates *states,
						   const F32x2 sample)
{
	F32x2 gain 		   = F32x2Set(INT32_MAX);											// Q27
	Boolx2 isProcessed = Boolx2Set(0);
	F32x2 envelope	   = envelopeCalc(&coeffs->envelope, &states->envelope, sample);	// Q27
	F32x2 res;																			// Q27

	// NOISE GATE
	F32x2 tmpGain = noiseGate(&coeffs->noiseGate, &states->noiseGate, envelope);
	F32x2MovIfTrue(&gain, tmpGain, states->noiseGate.isWorked);

	// EXPANDER
	tmpGain = expander(&coeffs->expander, &states->expander, envelope);
	F32x2MovIfTrue(&gain, F32x2Min(gain, tmpGain), states->expander.isWorked);

	// COMPRESSOR
	tmpGain = compressor(&coeffs->compressor, &states->compressor, envelope);
	F32x2MovIfTrue(&gain, F32x2Min(gain, tmpGain), states->compressor.isWorked);

	// if nothing was applied, gain sets to "1"
	isProcessed = Boolx2OR(states->noiseGate.isWorked, states->expander.isWorked);
	isProcessed = Boolx2OR(isProcessed, states->compressor.isWorked);
	F32x2MovIfTrue(&gain, F32x2Set(0x08000000), Boolx2NOT(isProcessed));

	// gain applies to sample
	res = F32x2Mul(gain, sample);
	res = F32x2LeftShiftAS(res, 4);
	ringBuffAddValue(&states->ringBuff, res);

	// LIMITER
	tmpGain = limiter(&coeffs->limiter, &states->ringBuff);
	res = F32x2Mul(states->ringBuff.samples[states->ringBuff.currNum], tmpGain);
	res = F32x2LeftShiftAS(res, 4);

	return res;
}
