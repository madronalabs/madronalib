//
//  MLDSPUtils.cpp
//  Kaivo
//
//  Created by Randy Jones on 1/31/14.
//
//

#include "MLDSPUtils.h"

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
	a2 = a0;
	b1 = -2.f * cosOmega * b0;
	b2 = (1.f - alpha) * b0;
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
	b1 = a1;
	b2 = (1.f - alphaOverA) * b0;
}

void MLBiquad::setHipass(float f, float q)
{
	//HPF:        H(s) = s^2 / (s^2 + s/Q + 1)
	float omega = kMLTwoPi * f * mInvSr;
	float cosOmega = cosf(omega);
	float alpha = sinf(omega) / (2.f * q);
	float b0 = 1.f + alpha;
	
	a0 = (1.f + cosOmega) * 0.5f / b0;
	a1 = -(1.f + cosOmega) / b0;
	a2 = a0;
	b1 = -2.f * cosOmega / b0;
	b2 = (1.f - alpha) / b0;
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
}

void MLBiquad::setOnePole(float f)
{
    //	float omega = kMLTwoPi * f * mInvSr;
    //	float cosOmega = cosf(omega);
	float e = 2.718281828;
	float x = powf(e, -kMLTwoPi * f * mInvSr);
	a0 = 1.f - x;
	a1 = 0;
	a2 = 0;
	b1 = -x;
	b2 = 0;
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
}

void MLBiquad::setDifferentiate(void)
{
	a0 = 1.f;
	a1 = -1.f;
	a2 = 0;
	b1 = 0;
	b2 = 0;
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
const float MLSineOsc::kFlipOffset = kRootX*2.f;

// ----------------------------------------------------------------
#pragma mark MLTriOsc

const float MLTriOsc::kIntDomain = powf(2.f, 32.f);
const float MLTriOsc::kDomainScale = 4.f/kIntDomain;

// ----------------------------------------------------------------
#pragma mark MLLinearDelay

void MLLinearDelay::resize(float duration)
{
    int newSize = duration*(float)mSR;
    mBuffer.setDims(newSize);
    mLengthMask = (1 << mBuffer.getWidthBits()) - 1;
}

// ----------------------------------------------------------------
#pragma mark MLAllpassDelay

void MLAllpassDelay::resize(float duration)
{
    int newSize = duration*(float)mSR;
    mBuffer.setDims(newSize);
    mLengthMask = (1 << mBuffer.getWidthBits()) - 1;
}

void MLAllpassDelay::setModDelay(float d)
{
//    int prevD = mModDelayInSamples;

    mModDelayInSamples = d*(float)mSR;
    
//    int newD = mModDelayInSamples;    
//    if(prevD < newD) debug() << "<";
//    if(prevD > newD) debug() << ">";
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
    
    return sum*mBlend + modTapOut*mFeedForward;
}




