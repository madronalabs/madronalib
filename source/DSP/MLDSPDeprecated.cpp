//
//  MLDSPUtils.cpp
//  Kaivo
//
//  Created by Randy Jones on 1/31/14.
//
//

#include "MLDSPDeprecated.h"


//	bit 31		bits 30-23		bits 22-0
//	sign		exponent		significand
//	0			011 1111 1		000 0000 0000 0000 0000 0000


// ----------------------------------------------------------------
// old DSPUtils

MLSample* alignToCacheLine(const MLSample* p)
{
	uintptr_t pM = (uintptr_t)p;
	pM += (uintptr_t)(kMLCacheAlignSize - 1);
	pM &= kMLCacheAlignMask;	
	return(MLSample*)pM;
} 

int ilog2(int x)
{
	int b=0;
	if(x >= 1<<16) { x >>= 16; b |= 16; }
	if(x >= 1<<8) { x >>= 8; b |= 8; }
	if(x >= 1<<4) { x >>= 4; b |= 4; }
	if(x >= 1<<2) { x >>= 2; b |= 2; }
	if(x >= 1<<1) b |= 1;
	return b;
}

float scaleForRangeTransform(float a, float b, float c, float d)
{
	return (d - c) / (b - a);
}

float offsetForRangeTransform(float a, float b, float c, float d)
{
	return (a*d - b*c) / (a - b);
}

float inMinusPiToPi(float theta)
{
	float t = theta;
	while(t < -kMLPi) t += kMLTwoPi;
	while(t > kMLPi) t -= kMLTwoPi;
	return t;
}


int MLisNaN(float x) 
{ 
#ifdef _WIN32
	return (x != x);
#else
	return std::isnan(x);
#endif
}

int MLisNaN(double x) 
{ 
#ifdef _WIN32
	return (x != x);
#else
	return std::isnan(x);
#endif
}

int MLisInfinite(float x) 
{ 
#ifdef _WIN32
	return ((x > FLT_MAX) || (x < -FLT_MAX));
#else
	return std::isinf(x);
#endif
}

int MLisInfinite(double x) 
{ 
#ifdef _WIN32
	return ((x > DBL_MAX) || (x < -DBL_MAX));
#else
	return std::isinf(x);
#endif
}

// TODO make functor we can easily make more of

static uint32_t gMLRandomSeed = 0;

inline void MLRandStep()
{
	gMLRandomSeed = gMLRandomSeed * 0x0019660D + 0x3C6EF35F;
}

// return single-precision floating point number on [-1, 1]
float MLRand()
{
	MLRandStep();
	uint32_t temp = (gMLRandomSeed >> 9) & 0x007FFFFF;
	temp &= 0x007FFFFF;// DSPConstants.r2;
	temp |= 0x3F800000; // DSPConstants.r1;
	
	float* pf = reinterpret_cast<float*>(&temp);
	*pf *= 2.f;
	*pf -= 3.f;
	
	return *pf;
}

// return 32 pseudorandom bits
uint32_t MLRand32()
{
	MLRandStep();
	return gMLRandomSeed;
}

void MLRandReset(void)
{
	gMLRandomSeed = 0;
}

const MLRange UnityRange = MLRange(0.f, 1.f);

// ----------------------------------------------------------------
#pragma mark MLBiquad

void MLBiquad::clear()
{
    x2 = 0.;
    x1 = 0.;
    y2 = 0.;
    y1 = 0.;
}

void MLBiquad::setLopass(float f, float q)
{
	//LPF:        H(s) = 1 / (s^2 + s/Q + 1)
	float omega = kMLTwoPi * f * mInvSr;
	float cosOmega = cosf(omega);
	float alpha = sinf(omega) / (2.f * q);
	float b0 = 1.f / (1.f + alpha);
	
	a0 = (1.f - cosOmega) * 0.5f * b0;
	a1 = (1.f - cosOmega) * b0;
	a2 = (1.f - cosOmega) * 0.5f * b0;
	b1 = -2.f * cosOmega * b0;
	b2 = (1.f - alpha) * b0;
	b1 = -b1; b2 = -b2;
}

void MLBiquad::setHipass(float f, float q)
{
	//HPF:        H(s) = s^2 / (s^2 + s/Q + 1)
	float omega = kMLTwoPi * f * mInvSr;
	float cosOmega = cosf(omega);
	float alpha = sinf(omega) / (2.f * q);
	float b0 = 1.0f / (1.f + alpha);
	
	a0 = (1.f + cosOmega) * 0.5f *b0;
	a1 = -(1.f + cosOmega) *b0;
	a2 = (1.f + cosOmega) * 0.5f *b0;
	b1 = -2.f * cosOmega *b0;
	b2 = (1.f - alpha) *b0;
	b1 = -b1; b2 = -b2;
}

void MLBiquad::setPeakNotch(float f, float q, float gain)
{
	//notch: H(s) = (s^2 + 1) / (s^2 + s/Q + 1)
	float omega = kMLTwoPi * f * mInvSr;
	float cosOmega = cosf(omega);
	float alpha = sinf(omega) / (2.f * q);
    float A = sqrtf(gain);
    float alphaOverA = alpha/A;
    A *= alpha;
	float b0 = 1.f / (1.f + alphaOverA);
	
	a0 = (1.f + A) * b0;
	a1 = -2.f * cosOmega * b0;
	a2 = (1.f - A) * b0;
	b1 = a1*b0;
	b2 = (1.f - alphaOverA) * b0;
	b1 = -b1; b2 = -b2;
}

void MLBiquad::setBandpass(float f, float q)
{
	//BPF: H(s) = s / (s^2 + s/Q + 1)  (constant skirt gain, peak gain = Q)

	float omega = kMLTwoPi * f * mInvSr;
	float cosOmega = cosf(omega);
	float alpha = sinf(omega) / (2.f * q);
	float b0 = 1.f + alpha;
	
	a0 = alpha / b0;
	a1 = 0.;
	a2 = -alpha / b0;
	b1 = -2.f * cosOmega / b0;
	b2 = (1.f - alpha) / b0;
	b1 = -b1; b2 = -b2;
}

void MLBiquad::setNotch(float f, float q)
{
	//notch: H(s) = (s^2 + 1) / (s^2 + s/Q + 1)
	float omega = kMLTwoPi * f * mInvSr;
	float cosOmega = cosf(omega);
	float alpha = sinf(omega) / (2.f * q);
	float b0 = 1.f + alpha;
	
	a0 = 1.f / b0;
	a1 = -2.f * cosOmega / b0;
	a2 = a0;
	b1 = -2.f * cosOmega / b0;
	b2 = (1.f - alpha) / b0;
	b1 = -b1; b2 = -b2;
}

void MLBiquad::setOnePole(float f)
{
	float e = 2.718281828;
	float x = powf(e, -kMLTwoPi * f * mInvSr);
	a0 = 1.f - x;
	a1 = 0;
	a2 = 0;
	b1 = -x;
	b2 = 0;
	b1 = -b1; b2 = -b2;
}

void MLBiquad::setLoShelf(float f, float q, float gain)
{
    // lowShelf: H(s) = A * (s^2 + (sqrt(A)/Q)*s + A)/(A*s^2 + (sqrt(A)/Q)*s + 1)
    float A = gain;
    float aMinus1 = A - 1.0f;
    float aPlus1 = A + 1.0f;
	float omega = kMLTwoPi * f * mInvSr;
	float cosOmega = cosf(omega);
	float alpha = sinf(omega) / (2.f * q);
    float beta = 2.0f*sqrtf(A)*alpha;
	
	float b0 = aPlus1 + aMinus1*cosOmega + beta;
	a0 = (A*(aPlus1 - aMinus1*cosOmega + beta)) / b0;
	a1 = (A*(aPlus1*-2.0f*cosOmega + 2.0f*aMinus1)) / b0;
	a2 = (A*(aPlus1 - aMinus1*cosOmega - beta)) / b0;
    b1 = (aPlus1*-2.0f*cosOmega - 2.0f*aMinus1) / b0;
	b2 = (aPlus1 + aMinus1*cosOmega - beta) / b0;
	b1 = -b1; b2 = -b2;
}

void MLBiquad::setHiShelf(float f, float q, float gain)
{
    // highShelf: H(s) = A * (A*s^2 + (sqrt(A)/Q)*s + 1)/(s^2 + (sqrt(A)/Q)*s + A)
    float A = gain;
    float aMinus1 = A - 1.0f;
    float aPlus1 = A + 1.0f;
	float omega = kMLTwoPi * f * mInvSr;
	float cosOmega = cosf(omega);
	float alpha = sinf(omega) / (2.f * q);
    float beta = 2.0f*sqrtf(A)*alpha;
	
	float b0 = aPlus1 - aMinus1*cosOmega + beta;
	a0 = (A*(aPlus1 + aMinus1*cosOmega + beta)) / b0;
	a1 = (A*(aPlus1*-2.0f*cosOmega + -2.0f*aMinus1)) / b0;
	a2 = (A*(aPlus1 + aMinus1*cosOmega - beta)) / b0;
    b1 = (aPlus1*-2.0f*cosOmega + 2.0f*aMinus1) / b0;
	b2 = (aPlus1 - aMinus1*cosOmega - beta) / b0;
	b1 = -b1; b2 = -b2;
}

// make first order allpass section based on delay parameter d.
void MLBiquad::setAllpassDelay(float D)
{
    float alpha = (1.f - D) / (1.f + D);
    a0 = alpha;
    a1 = 1.f;
    a2 = 0;
    b1 = alpha;
    b2 = 0;
	b1 = -b1; b2 = -b2;
}

// set first order allpass section alpha directly.
void MLBiquad::setAllpassAlpha(float alpha)
{
    a0 = alpha;
    a1 = 1.f;
    a2 = 0;
    b1 = alpha;
    b2 = 0;
	b1 = -b1; b2 = -b2;
}

// make second order allpass section based on frequency f and pole radius r.
void MLBiquad::setAllpass2(float f, float r)
{
	float omega = kMLTwoPi * f * mInvSr;
	float cosOmega = cosf(omega);
	a0 = r*r;
	a1 = -2.f*r*cosOmega;
	a2 = 1.f;
	b1 = -2.f*r*cosOmega;
	b2 = r*r;
	b1 = -b1; b2 = -b2;
}

void MLBiquad::setDifferentiate(void)
{
	a0 = 1.f;
	a1 = -1.f;
	a2 = 0;
	b1 = 0;
	b2 = 0;
	b1 = -b1; b2 = -b2;
}

void MLBiquad::setCoefficients(float pa0, float pa1, float pa2, float pb1, float pb2)
{
	a0 = pa0;
	a1 = pa1;
	a2 = pa2;
	b1 = pb1;
	b2 = pb2;
}

// TODO verify that this is correct for different filter types.
void MLBiquad::setState(float f)
{
	x2 = x1 = f;
	y2 = y1 = f;
}

// ----------------------------------------------------------------
#pragma mark MLSineOsc

const float MLSineOsc::kIntDomain = powf(2.f, 32.f);
const float MLSineOsc::kRootX = sqrtf(2.f);
const float MLSineOsc::kOneSixth = 1.f/6.f;
const float MLSineOsc::kRange = kRootX - kRootX*kRootX*kRootX*kOneSixth;
const float MLSineOsc::kDomain = kRootX*4.f;
const float MLSineOsc::kScale = 1.f/kRange;
const float MLSineOsc::kDomainScale = kDomain/kIntDomain;
const float MLSineOsc::kPhaseInvScale = kIntDomain;
const float MLSineOsc::kFlipOffset = kRootX*2.f;

// ----------------------------------------------------------------
#pragma mark MLTriOsc

const float MLTriOsc::kIntDomain = powf(2.f, 32.f);
const float MLTriOsc::kDomainScale = 4.f/kIntDomain;

// ----------------------------------------------------------------
#pragma mark MLPhaseOsc

const float MLPhaseOsc::kIntDomain = powf(2.f, 32.f);
const float MLPhaseOsc::kDomainScale = 1.f/kIntDomain;

// ----------------------------------------------------------------
#pragma mark MLSampleDelay

/*
void MLSampleDelay::resize(float duration)
{
    int newSize = duration*(float)mSR;
    mBuffer.setDims(newSize);
    mLengthMask = (1 << mBuffer.getWidthBits()) - 1;
    clear();
}
*/

// ----------------------------------------------------------------
#pragma mark MLLinearDelay

void MLLinearDelay::resize(float duration)
{
    int newSize = duration*(float)mSR;
    mBuffer.setDims(newSize);
    mLengthMask = (1 << mBuffer.getWidthBits()) - 1;
    clear();
}

// ----------------------------------------------------------------
#pragma mark MLAllpassDelay

void MLAllpassDelay::resize(float duration)
{
    int newSize = duration*(float)mSR;
    mBuffer.setDims(newSize);
    mLengthMask = (1 << mBuffer.getWidthBits()) - 1;
    mWriteIndex = 0;
}

// TODO modulating this allpass is a little bit clicky.
// add history crossfading to address this. 
MLSample MLAllpassDelay::processSample(const MLSample x)
{
    float fDelayInt, D;
    float alpha, allpassIn;
    float sum;
    int delayInt;
    
    mWriteIndex &= mLengthMask;
    sum = x - mFeedback*mFixedTapOut;

    mBuffer[mWriteIndex] = sum;
    mWriteIndex++;
    
    // get modulation tap
    fDelayInt = floorf(mModDelayInSamples);
    delayInt = (int)fDelayInt;
    
    // get allpass interpolation coefficient D
    D = mModDelayInSamples - fDelayInt;

    // constrain D to [0.5 - 1.5];
    if (D < 0.5f)
    {
        D += 1.f;
        delayInt -= 1;
    }
    
    alpha = (1.f - D) / (1.f + D); // exact
    // TODO try this or Taylor approx. in van Duyne thesis
    //float xm1 = (D - 1.f);
    //alpha = -0.53f*xm1 + 0.25f*xm1*xm1; // approx on [0.5, 1.5]
    
    uintptr_t readIndex = mWriteIndex - (uintptr_t)delayInt;
    readIndex &= mLengthMask;
    allpassIn = mBuffer[readIndex];
    float modTapOut = alpha*allpassIn + mX1 - alpha*mY1;
    mX1 = allpassIn;
    mY1 = modTapOut;
    
    // get fixed tap
    readIndex = mWriteIndex - (uintptr_t)mFixedDelayInSamples;
    readIndex &= mLengthMask;
    mFixedTapOut = mBuffer[readIndex];
    
	// TODO mBlend is not dry blend, see where this is used and correct! 
    return sum*mBlend + modTapOut*mFeedForward;
}


// ----------------------------------------------------------------
#pragma mark MLFDN

static const float kMaxDelayLength = 1.0f;

void MLFDN::resize(int n)
{
    mDelays.resize(n);
    for(int i=0; i<n; ++i)
    {
        mDelays[i].setSampleRate(mSR);
        mDelays[i].resize(kMaxDelayLength);
    }

    mFilters.resize(n);
    mDelayOutputs.setDims(n);
    
    // make Householder feedback matrix (default)
    mMatrix.setDims(n, n);
    mMatrix.setIdentity();
    mMatrix.subtract(2.0f/(float)n);
    
    mSize = n;
}

void MLFDN::setIdentityMatrix()
{
    mMatrix.setIdentity();
}

void MLFDN::clear()
{
    for(int i=0; i<mDelays.size(); ++i)
    {
        mDelays[i].clear();
    }
    
    for(int i=0; i<mFilters.size(); ++i)
    {
        mFilters[i].clear();
    }
    mDelayOutputs.clear();
}

void MLFDN::setSampleRate(int sr)
{
    mSR = sr;
    mInvSr = 1.0f / (float)sr;
    // delays samples rates need to be set here so resize calculates correctly
    for(int i=0; i<mSize; ++i)
    {
        mDelays[i].setSampleRate(sr);
        mDelays[i].resize(kMaxDelayLength);
        mDelays[i].clear();        
        mFilters[i].setSampleRate(sr);
    }
}

void MLFDN::setDelayLengths(float maxLength)
{
    float t = ml::clamp(maxLength, 0.f, kMaxDelayLength);
    mDelayTime = t;
	float offset = mDelayTime*0.02f;
   //debug() << " MLFDN delays: \n ";
    for(int i=0; i<mSize; ++i)
    {
        // clear delay and set to all feedforward, no feedback
        mDelays[i].setSampleRate(mSR);
        mDelays[i].setMixParams(0., 1., 0.);
        mDelays[i].clear();
        
        //debug() << "    " << i << " : " << t << "\n";
        mDelays[i].setModDelay(t);
        t *= mFreqMul;
		t += offset;
    }
}

void MLFDN::setLopass(float f)
{
    for(int i=0; i<mSize; ++i)
    {
        mFilters[i].setOnePole(f);
    }
}

MLSample MLFDN::processSample(const MLSample x)
{
	float outputSum = 0.f;    
	for(int j=0; j<mSize; ++j)
	{
		// input + feedback
		float inputSum = x;
		for(int i=0; i<mSize; ++i)
		{
			inputSum += mDelayOutputs[i]*mMatrix(i, j);
		}
		
		// delays
		mDelayOutputs[j] = mDelays[j].processSample(inputSum);        
		mDelayOutputs[j] *= mFeedbackAmp;
		
		// filters
		mDelayOutputs[j] = mFilters[j].processSample(mDelayOutputs[j]);        
		outputSum += mDelayOutputs[j];
	}
	return outputSum;
}

MLSignal MLFDN::operator()(const MLSignal& x)
{
	int frames=x.getWidth();
	MLSignal y(frames);
	for(int n=0; n<frames; ++n)
	{
		y[n] = processSample(x[n]);
	}
	return y;
}

// ----------------------------------------------------------------
#pragma mark MLHalfBandFilter

const float MLHalfBandFilter::ka0 = 0.07986642623635751;
const float MLHalfBandFilter::ka1 = 0.5453536510711322;
const float MLHalfBandFilter::kb0 = 0.28382934487410993;
const float MLHalfBandFilter::kb1 = 0.8344118914807379;

MLHalfBandFilter::AllpassSection::AllpassSection() :
    a(0)
{
    clear();
}

MLHalfBandFilter::AllpassSection::~AllpassSection()
{
}

void MLHalfBandFilter::AllpassSection::clear()
{
    x0 = x1 = y0 = y1 = 0.f;
}

MLHalfBandFilter::MLHalfBandFilter()
{
    apa0.a = ka0;
    apa1.a = ka1;
    apb0.a = kb0;
    apb1.a = kb1;
    x0 = x1 = a0 = b0 = b1 = 0.f;
    k = 0;
    clear();
}

MLHalfBandFilter::~MLHalfBandFilter()
{
}

void MLHalfBandFilter::clear()
{
    apa0.clear();
    apa1.clear();
    apb0.clear();
    apb1.clear();
}

// ----------------------------------------------------------------
#pragma mark MLDownsample2x

void MLDownsample2x::clear()
{
    f.clear();
}

// ----------------------------------------------------------------
#pragma mark MLUpsample2x

void MLUpsample2x::clear()
{
    f.clear();
}
