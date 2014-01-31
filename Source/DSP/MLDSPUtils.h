//
//  MLDSPUtils.h
//  Kaivo
//
//  Created by Randy Jones on 1/31/14.
//
//

#ifndef __MLDSPUtils__
#define __MLDSPUtils__

#include "MLDSP.h"
#include "MLSignal.h"

// ----------------------------------------------------------------
// DSP utility objects -- some very basic building blocks, not in MLProcs
// so they can be used by MLProcs.
//

// ----------------------------------------------------------------
#pragma mark MLBiquad

class MLBiquad
{
public:
	MLBiquad() { a0 = a1 = a2 = b1 = b2 = 0.f; mInvSr = 1.f;}
	~MLBiquad() {}
	
    void clear();
	void setSampleRate(float sr) { mInvSr = 1.f / sr; }
	void setLopass(float f, float q);
	void setHipass(float f, float q);
    void setPeakNotch(float f, float q, float gain);
	void setNotch(float f, float q);
	void setOnePole(float f);
	void setDifferentiate(void);
    inline MLSample processSample(float x)
    {
        float out;
        out = a0*x + a1*x1 + a2*x2 - b1*y1 - b2*y2;
        x2 = x1;
        x1 = x;
        y2 = y1;
        y1 = out;
        return(out);
    }
    
	float a0, a1, a2, b1, b2;
	float x1, x2, y1, y2;
	float mInvSr;
};

// ----------------------------------------------------------------
#pragma mark MLSineOsc

// this sine generator makes a looping counter by letting a 32 bit word overflow.
class MLSineOsc
{
public:
    static const float kIntDomain, kRootX, kOneSixth, kRange, kDomain, kScale, kDomainScale, kFlipOffset;
    MLSineOsc() : mStep32(0) { clear(); }
    ~MLSineOsc(){}
    
	inline void clear() { mOmega32 = 0; }
    inline void setSampleRate(int sr) { mInvSrDomain = (float)kIntDomain / (float)sr; }
    inline void setFrequency(MLSample f) { mStep32 = (int)(mInvSrDomain * f); }
    inline MLSample processSample()
    {
        float x, fOmega;
        
        // add increment with wrap
        mOmega32 += mStep32;
        
        // scale to sin approx domain
        fOmega = mOmega32 * kDomainScale + kRootX;
        
        // reverse upper half to make triangle wave
        x = fOmega + fSignBit(mOmega32)*(kFlipOffset - fOmega - fOmega);
        
        // sine approx.
        return x*(1.0f - kOneSixth*x*x) * kScale;
    }
private:
	int32_t mOmega32, mStep32;
    float mInvSrDomain;
};

// ----------------------------------------------------------------
#pragma mark MLTriOsc

// this triangle generator makes a looping counter by letting a 32 bit word overflow.
// it's a simple triangle, not antialiased.

class MLTriOsc
{
public:
    static const float kIntDomain, kRootX, kOneSixth, kRange, kDomain, kScale, kDomainScale, kFlipOffset;
    MLTriOsc() : mStep32(0) { clear(); }
    ~MLTriOsc(){}
    
	inline void clear() { mOmega32 = 0; }
    inline void setSampleRate(int sr) { mInvSrDomain = (float)kIntDomain / (float)sr; }
    inline void setFrequency(MLSample f) { mStep32 = (int)(mInvSrDomain * f); }
    inline MLSample processSample()
    {
        float x, fOmega;
        
        // add increment with wrap
        mOmega32 += mStep32;
        
        // scale to sin approx domain
        fOmega = mOmega32 * kDomainScale + kRootX;
        
        // reverse upper half to make triangle wave
        x = fOmega + fSignBit(mOmega32)*(kFlipOffset - fOmega - fOmega);
        
        // sine approx.
        return x*kScale;
    }
private:
	int32_t mOmega32, mStep32;
    float mInvSrDomain;
};

// ----------------------------------------------------------------
#pragma mark MLAllpassDelay
// a delay with one fixed feedback tap and one allpass interpolated
// modulation tap. A dry blend is also summed at the output.
// After Dattorro.

class MLAllpassDelay
{
public:
    MLAllpassDelay() { clear(); }
	~MLAllpassDelay() {}
    
	inline void clear()
    {
        mBuffer.clear();
    }
    inline void setSampleRate(int sr) { mSR = sr; mInvSr = 1.0f / (float)sr; }
    void resize(float duration);
    inline void setMixParams(float b, float ff, float fb) { mBlend = b; mFeedForward = ff; mFeedback = fb; }
    inline void setFixedDelay(float d) { mFixedDelayInSamples = (int)(d*(float)mSR); }
    inline void setModDelay(float d) { mModDelayInSamples = d*(float)mSR; }
	 MLSample processSample(const MLSample x);
    
private:
	
    MLSignal mBuffer;
    int mSR;
    float mInvSr;
    
	uintptr_t mWriteIndex;
	uintptr_t mLengthMask;
	int mCounter;
    
    int mFixedDelayInSamples;
    float mModDelayInSamples;
    
    MLSample mBlend, mFeedForward, mFeedback;
	
	MLSample mkFreq; // onepole coeff;
	MLSample mFreq1;
    
	MLSample mFixedTapOut;
	MLSample mX1;
	MLSample mY1;

};



#endif /* defined(__MLDSPUtils__) */
