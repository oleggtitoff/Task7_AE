/*
 * AmplitudeProc.c
 *
 *  Created on: Aug 8, 2019
 *      Author: Intern_2
 */

#include "AmplitudeProc.h"

ALWAYS_INLINE Status paramsInit(Params *params, const int sampleRate)
{
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

ALWAYS_INLINE Status coeffsInit(Coeffs *coeffs)
{
	if (!coeffs)
		return statusError;

	coeffs->noiseGateIsActive 		= 0;
	coeffs->expanderIsActive 		= 0;
	coeffs->compressorIsActive 		= 0;
	coeffs->limiterIsActive 		= 0;

	coeffs->noiseThr 				= F32x2Zero();
	coeffs->expanderHighThr 		= F32x2Zero();
	coeffs->compressorLowThr 		= F32x2Zero();
	coeffs->limiterThr 				= F32x2Zero();

	coeffs->expanderC1 				= F32x2Zero();
	coeffs->expanderC2 				= F32x2Zero();
	coeffs->compressorC1 			= F32x2Zero();
	coeffs->compressorC2 			= F32x2Zero();

	coeffs->samplesAlphaAttackTime 	= F32x2Zero();
	coeffs->samplesAlphaReleaseTime = F32x2Zero();
	coeffs->gainAlphaAttackTime 	= F32x2Zero();
	coeffs->gainAlphaReleaseTime 	= F32x2Zero();
	coeffs->fadeAlphaAttackTime 	= F32x2Zero();
	coeffs->fadeAlphaReleaseTime 	= F32x2Zero();

	return statusOK;
}

ALWAYS_INLINE Status ringBuffInit(RingBuff *ringBuff)
{
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

ALWAYS_INLINE Status prevValuesBuffInit(PrevValuesBuff *prevValuesBuff)
{
	if (!prevValuesBuff)
		return statusError;

	prevValuesBuff->prevSample = F32x2Zero();
	prevValuesBuff->prevGain = F32x2Zero();
	prevValuesBuff->isFade = Boolx2Set(1);

	return statusOK;
}

ALWAYS_INLINE void coeffsCalc(double *C1, double *C2, const double ratio,
		const double threshold, const int type)		// if type = 0, expander
													// else - compressor
{
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
	return (double)1 - exp((double)-1 / (sampleRate * time / 1000));
}

ALWAYS_INLINE void updateMaxRingBuffValue(RingBuff *ringBuff)
{
	uint16_t i;
	ringBuff->maxSample = ringBuff->samples[0];

	for (i = 1; i < RING_BUFF_SIZE; i++)
	{
		ringBuff->maxSample = F32x2Max(ringBuff->maxSample, ringBuff->samples[i]);
	}
}


ALWAYS_INLINE F32x2 smoothingFilter(const F32x2 x, const F32x2 alpha, const F32x2 prevY)
{
	F32x2 alpha2 = F32x2Sub(0x7fffffff, alpha);
	F32x2 mulRes1 = F32x2Mul(x, alpha);
	F32x2 mulRes2 = F32x2Mul(alpha2, prevY);
	return F32x2Add(mulRes1, mulRes2);
}

ALWAYS_INLINE F32x2 envelopeCalc(const Coeffs *coeffs, const PrevValuesBuff *prevValuesBuff,
								 const F32x2 sample)
{
	F32x2 alpha = coeffs->samplesAlphaAttackTime;
	Boolx2 isRelease = F32x2LessThan(F32x2Abs(sample), F32x2Abs(prevValuesBuff->prevSample));
	F32x2MovIfTrue(&alpha, coeffs->samplesAlphaReleaseTime, isRelease);

	return smoothingFilter(F32x2Abs(sample), alpha, prevValuesBuff->prevSample);
}

ALWAYS_INLINE F32x2 instantGainCalc(const F32x2 envelopeSample, const F32x2 C1,
									const F32x2 C2)
{
	F32x2 sampleInQ27 = F32x2RightShiftA(envelopeSample, 4);
	F32x2 negC1 	  = F32x2Sub(F32x2Zero(), C1);
	F32x2 powRes	  = F32x2Pow(sampleInQ27, negC1);
	F32x2 mulRes	  = F32x2Mul(C2, powRes);
	F32x2 instantGain = F32x2LeftShiftAS(mulRes, 4);

	return instantGain;
}

ALWAYS_INLINE F32x2 gainAlphaChoose(const Coeffs *coeffs,
					const PrevValuesBuff *prevValuesBuff, const F32x2 instantGain)
{
	F32x2 alpha = coeffs->gainAlphaAttackTime;
	Boolx2 isRelease = F32x2LessThan(prevValuesBuff->prevGain, instantGain);
	F32x2MovIfTrue(&alpha, coeffs->gainAlphaReleaseTime, isRelease);

	return alpha;
}


ALWAYS_INLINE Boolx2 noiseGate(const Coeffs *coeffs, F32x2 sample, F32x2 *gain)
{
	Boolx2 isNoiseGate = F32x2LessThan(sample, coeffs->noiseThr);
	F32x2MovIfTrue(gain, F32x2Zero(), isNoiseGate);

	return isNoiseGate;
}

ALWAYS_INLINE Boolx2 expander(const Coeffs *coeffs, const PrevValuesBuff *prevValuesBuff,
							  const F32x2 envelopeSample, F32x2 *gain)
{
	F32x2 instantGain = instantGainCalc(envelopeSample, coeffs->expanderC1,
										coeffs->expanderC2);
	F32x2 alpha = gainAlphaChoose(coeffs, prevValuesBuff, instantGain);
	F32x2 expanderGain = smoothingFilter(instantGain, alpha, prevValuesBuff->prevGain);

	Boolx2 isExpander = F32x2LessThan(envelopeSample, coeffs->expanderHighThr);
	isExpander = Boolx2AND(isExpander, F32x2LessThan(expanderGain, *gain));
	F32x2MovIfTrue(gain, expanderGain, isExpander);

	return isExpander;
}

ALWAYS_INLINE Boolx2 compressor(const Coeffs *coeffs, const PrevValuesBuff *prevValuesBuff,
								const F32x2 envelopeSample, F32x2 *gain)
{
	F32x2 instantGain = instantGainCalc(envelopeSample, coeffs->compressorC1,
										coeffs->compressorC2);
	F32x2 alpha = gainAlphaChoose(coeffs, prevValuesBuff, instantGain);
	F32x2 compressorGain = smoothingFilter(instantGain, alpha, prevValuesBuff->prevGain);

	Boolx2 isCompressor = F32x2LessThan(coeffs->compressorLowThr, envelopeSample);
	isCompressor = Boolx2AND(isCompressor, F32x2LessThan(compressorGain, *gain));
	F32x2MovIfTrue(gain, compressorGain, isCompressor);

	return isCompressor;

}

//ALWAYS_INLINE
Boolx2 limiter(const Coeffs *coeffs, RingBuff *ringBuff, F32x2 *gain)
{
	updateMaxRingBuffValue(ringBuff);

	F32x2 sampleValue = F32x2LeftShiftAS(F32x2Mul(*gain, ringBuff->maxSample), 4);
	Boolx2 isLimiter  = F32x2LessThan(coeffs->limiterThr, sampleValue);
	F32x2 limGain 	  = F32x2Mul(*gain, F32x2Div(coeffs->limiterThr, sampleValue));

	F32x2MovIfTrue(gain, limGain, isLimiter);

	return isLimiter;
}

ALWAYS_INLINE F32x2 fade(const Coeffs *coeffs, const PrevValuesBuff *prevValuesBuff,
						 const F32x2 gain)
{
	F32x2 alpha = coeffs->fadeAlphaAttackTime;
	Boolx2 isRelease = F32x2LessThan(gain, prevValuesBuff->prevGain);
	F32x2MovIfTrue(&alpha, coeffs->fadeAlphaReleaseTime, isRelease);

	return smoothingFilter(gain, alpha, prevValuesBuff->prevGain);
}


Status AmplitudeProcInit(Params *params, Coeffs *coeffs, RingBuff *ringBuff,
						 PrevValuesBuff *prevValuesBuff, const int sampleRate)
{
	Status status = 0;

	status |= paramsInit(params, sampleRate);
	status |= coeffsInit(coeffs);
	status |= ringBuffInit(ringBuff);
	status |= prevValuesBuffInit(prevValuesBuff);

	return status;
}

Status AmplitudeProcSetParam(Params *params, Coeffs *coeffs, const uint16_t id,
							 const double val)
{
	Status status = statusOK;
	double C1, C2;

	if(!coeffs)
		return statusError;

	switch(id)
	{
	case sampleRateID:
		params->sampleRate = (int)val;
		break;

	case noiseGateIsActiveID:
		params->noiseGateIsActive = (int)val;
		coeffs->noiseGateIsActive = (int)val;
		break;

	case expanderIsActiveID:
		params->expanderIsActive = (int)val;
		coeffs->expanderIsActive = (int)val;
		break;

	case compressorIsActiveID:
		params->compressorIsActive = (int)val;
		coeffs->compressorIsActive = (int)val;
		break;

	case limiterIsActiveID:
		params->limiterIsActive = (int)val;
		coeffs->limiterIsActive = (int)val;
		break;

	case noiseThrID:
		params->noiseThr = val;
		coeffs->noiseThr = doubleToF32x2Set(dBtoGain(val));
		break;

	case expanderHighThrID:
		coeffsCalc(&C1, &C2, params->expanderRatio, val, 0);
		params->expanderHighThr = val;
		coeffs->expanderHighThr = doubleToF32x2Set(dBtoGain(val));
		coeffs->expanderC2 		= doubleToF32x2Set(C2 / 16);
		break;

	case compressorLowThrID:
		coeffsCalc(&C1, &C2, params->compressorRatio, val, 1);
		params->compressorLowThr = val;
		coeffs->compressorLowThr = doubleToF32x2Set(dBtoGain(val));
		coeffs->compressorC2 	 = doubleToF32x2Set(C2 / 16);
		break;

	case limiterThrID:
		params->limiterThr = val;
		coeffs->limiterThr = doubleToF32x2Set(dBtoGain(val));
		break;

	case expanderRatioID:
		coeffsCalc(&C1, &C2, val, params->expanderHighThr, 0);
		params->expanderRatio = val;
		coeffs->expanderC1 	  = doubleToF32x2Set(C1 / 16);
		coeffs->expanderC2 	  = doubleToF32x2Set(C2 / 16);
		break;

	case compressorRatioID:
		coeffsCalc(&C1, &C2, val, params->compressorLowThr, 1);
		params->compressorRatio = val;
		coeffs->compressorC1 	= doubleToF32x2Set(C1 / 16);
		coeffs->compressorC2 	= doubleToF32x2Set(C2 / 16);
		break;

	case samplesAttackTimeID:
		params->samplesAttackTime = val;
		coeffs->samplesAlphaAttackTime = doubleToF32x2Set(alphaCalc(params->sampleRate, val));
		break;

	case samplesReleaseTimeID:
		params->samplesReleaseTime = val;
		coeffs->samplesAlphaReleaseTime = doubleToF32x2Set(alphaCalc(params->sampleRate, val));
		break;

	case gainAttackTimeID:
		params->gainAttackTime = val;
		coeffs->gainAlphaAttackTime = doubleToF32x2Set(alphaCalc(params->sampleRate, val));
		break;

	case gainReleaseTimeID:
		params->gainReleaseTime = val;
		coeffs->gainAlphaReleaseTime = doubleToF32x2Set(alphaCalc(params->sampleRate, val));
		break;

	case fadeAttackTimeID:
		params->fadeAttackTime = val;
		coeffs->fadeAlphaAttackTime = doubleToF32x2Set(alphaCalc(params->sampleRate, val));
		break;

	case fadeReleaseTimeID:
		params->fadeReleaseTime = val;
		coeffs->fadeAlphaReleaseTime = doubleToF32x2Set(alphaCalc(params->sampleRate, val));
		break;

	default:
		status = statusError;
		break;
	}

	return status;
}

Status ringBuffSet(RingBuff *ringBuff, int32_t *dataBuff)
{
	if (!ringBuff || !dataBuff)
		return statusError;

	uint16_t i;

	for (i = 0; i < RING_BUFF_SIZE; i++)
	{
		ringBuff->samples[i] = F32x2Join(dataBuff[i * CHANNELS], dataBuff[i * CHANNELS + 1]);
		dataBuff[i * CHANNELS] = 0;
		dataBuff[i * CHANNELS + 1] = 0;
	}

	return statusOK;
}

void AmplitudeProc_Process(const Coeffs *coeffs, RingBuff *ringBuff, PrevValuesBuff *prevValuesBuff)
{
	F32x2 gain 			 = F32x2Set(INT32_MAX);								// Q27
	Boolx2 isNoiseGate 	 = Boolx2Set(0);
	Boolx2 isExpander 	 = Boolx2Set(0);
	Boolx2 isCompressor  = Boolx2Set(0);
	Boolx2 isProcessed 	 = Boolx2Set(0);
	Boolx2 isSmoothing 	 = Boolx2Set(0);
	F32x2 sample 		 = ringBuff->samples[ringBuff->currNum];			// Q31
	F32x2 envelopeSample = envelopeCalc(coeffs, prevValuesBuff, sample);	// Q31
	F32x2 res;																// Q31

	if (coeffs->noiseGateIsActive)
	{
		isNoiseGate = noiseGate(coeffs, envelopeSample, &gain);
		isProcessed = isNoiseGate;
		prevValuesBuff->isFade = Boolx2OR(prevValuesBuff->isFade, isNoiseGate);
	}

	if (coeffs->expanderIsActive)
	{
		isExpander = expander(coeffs, prevValuesBuff, envelopeSample, &gain);
		isProcessed = Boolx2OR(isProcessed, isExpander);
	}

	if (coeffs->compressorIsActive)
	{
		isCompressor = compressor(coeffs, prevValuesBuff, envelopeSample, &gain);
		isProcessed = Boolx2OR(isProcessed, isCompressor);
	}

	F32x2MovIfTrue(&gain, F32x2Set(0x08000000), Boolx2NOT(isProcessed));

//	F32x2 gainDelta = F32x2Sub(prevValuesBuff->prevGain, gain);
//	Boolx2 isFadeEnd = F32x2Equal(gainDelta, F32x2Zero());
//	prevValuesBuff->isFade = Boolx2AND(prevValuesBuff->isFade, Boolx2NOT(isFadeEnd));

	isSmoothing = Boolx2AND(prevValuesBuff->isFade, Boolx2NOT(isProcessed));
	isSmoothing = Boolx2OR(isSmoothing, isNoiseGate);
	F32x2MovIfTrue(&gain, fade(coeffs, prevValuesBuff, gain), isSmoothing);

	if (coeffs->limiterIsActive)
	{
		limiter(coeffs, ringBuff, &gain);
	}

	prevValuesBuff->prevSample = envelopeSample;
	prevValuesBuff->prevGain = gain;

	res = F32x2Mul(gain, sample);
	res = F32x2LeftShiftAS(res, 4);
	ringBuff->samples[ringBuff->currNum] = res;
}
