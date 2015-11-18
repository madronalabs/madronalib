//
//  MLDSPUtils.h
//  Madronalib
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
// so they can be used more easily in MLProcs and elsewhere.
//

// ----------------------------------------------------------------
#pragma mark MLBiquad

class MLBiquad
{
public:
	MLBiquad() { a0 = a1 = a2 = b1 = b2 = 0.f; mInvSr = 1.f; clear(); }
	~MLBiquad() {}
	
    void clear();
	void setSampleRate(float sr) { mInvSr = 1.f / sr; }
	void setLopass(float f, float q);
	void setHipass(float f, float q);
	void setBandpass(float f, float q);
    void setPeakNotch(float f, float q, float gain);
	void setNotch(float f, float q);
	void setOnePole(float f);
    void setAllpassAlpha(float a);
    void setAllpassDelay(float d);
    void setAllpass2(float f, float r);
	void setDifferentiate(void);
    void setLoShelf(float f, float q, float gain);
    void setHiShelf(float f, float q, float gain);
    void setCoefficients(float, float, float, float, float);
	
	// set the internal state of the filter as if the output has been at
	// the value f indefinitely. May cause a discontinuity in output.
	void setState(float f);

	inline MLSample processSample(float x)
    {
        const float out = a0*x + a1*x1 + a2*x2 - b1*y1 - b2*y2;
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
#pragma mark MLAsymmetricOnepole

class MLAsymmetricOnepole
{
public:
    MLAsymmetricOnepole() : ka(0), kb(0), y1(0){};
    ~MLAsymmetricOnepole(){};
    void clear()
    {
        y1 = 0.f;
    }
	void setSampleRate(float sr) { mInvSr = 1.f / sr; }
	void setCutoffs(float fa, float fb)
	{
		ka = clamp(kMLTwoPi*fa*mInvSr, 0.f, 0.25f);
		kb = clamp(kMLTwoPi*fb*mInvSr, 0.f, 0.25f);
	}
	void setAttackAndReleaseTimes(float tAttack, float tRelease)
	{
		ka = clamp(kMLTwoPi*(1.0f/tAttack)*mInvSr, 0.f, 0.25f);
		kb = clamp(kMLTwoPi*(1.0f/tRelease)*mInvSr, 0.f, 0.25f);
	}
    inline MLSample processSample(float x)
    {
        float dxdt = x - y1;
        float s = (dxdt < 0.f ? -1.f : 1.f);
        float k = ((1.f - s)*kb + (1.f + s)*ka)*0.5f;
        float out = y1 + k*dxdt;
        y1 = out;
        return(out);
    }
    
	float ka, kb;
	float y1;
	float mInvSr;
};

// ----------------------------------------------------------------
#pragma mark MLSineOsc

// this sine generator makes a looping counter by letting a 32 bit word overflow. The frequency 
// resolution of this counter is good enough for most uses: around 0.000005 Hz.
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
    static const float kIntDomain, kDomainScale;
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
        
        // scale to [-2, 2]
        fOmega = mOmega32 * kDomainScale;
        
        // reverse upper half to make triangle wave
        float s = fSignBit(mOmega32);
        x = 2.f*s*fOmega - fOmega;
       
        // and center
        x -= 1.0f;

        return x;
    }
private:
	int32_t mOmega32, mStep32;
    float mInvSrDomain;
};

// ----------------------------------------------------------------
#pragma mark MLSampleDelay
// a simple delay in integer samples with no mixing.

class MLSampleDelay
{
public:
    MLSampleDelay() { clear(); }
	~MLSampleDelay() {}
    
	inline void clear()
    {
        mBuffer.clear();
        mWriteIndex = 0;
    }
    inline void setSampleRate(int sr) { mSR = sr; mInvSr = 1.0f / (float)sr; }
    void resize(float duration);
    void setDelay(float d);
    
    inline MLSample processSample(const MLSample x)
    {
        mWriteIndex &= mLengthMask;
        mBuffer[mWriteIndex] = x;
        mWriteIndex++;        
        uintptr_t readIndex = mWriteIndex - (uintptr_t)mDelayInSamples;
        readIndex &= mLengthMask;        
        float a = mBuffer[readIndex];        
        return a;
    }
    
private:
    MLSignal mBuffer;
    
    int mSR;
    float mInvSr;
	uintptr_t mWriteIndex;
	uintptr_t mLengthMask;
    int mDelayInSamples;
};
// ----------------------------------------------------------------
#pragma mark MLLinearDelay
// a delay with one fixed feedback tap and one linear interpolated
// modulation tap. A dry blend is also summed at the output.

class MLLinearDelay
{
public:
    MLLinearDelay() { clear(); }
	~MLLinearDelay() {}
    
	inline void clear()
    {
        mBuffer.clear();
        mWriteIndex = 0;
    }
    inline void setSampleRate(int sr) { mSR = sr; mInvSr = 1.0f / (float)sr; }
    void resize(float duration);
    inline void setMixParams(float b, float ff, float fb) { mBlend = b; mFeedForward = ff; mFeedback = fb; }
    inline void setFixedDelay(float d) { mFixedDelayInSamples = (int)(d*(float)mSR); }
    void setModDelay(float d);
    
    inline MLSample processSample(const MLSample x)
    {
        float fDelayInt, D;
        float sum;
        int delayInt;
        
        sum = x - mFeedback*mFixedTapOut;
        
        mWriteIndex &= mLengthMask;
        mBuffer[mWriteIndex] = sum;
        mWriteIndex++;
        
        // get modulation tap
        fDelayInt = floorf(mModDelayInSamples);
        delayInt = (int)fDelayInt;
        
        // get linear interpolation coefficient D
        D = mModDelayInSamples - fDelayInt;
        
        uintptr_t readIndex = mWriteIndex - (uintptr_t)delayInt;
        uintptr_t readIndex2 = readIndex - 1;
        readIndex &= mLengthMask;
        readIndex2 &= mLengthMask;
        
        float a = mBuffer[readIndex];
        float b = mBuffer[readIndex2];
        float modTapOut = lerp(a, b, D);
        
        // get fixed tap
        readIndex = mWriteIndex - (uintptr_t)mFixedDelayInSamples;
        readIndex &= mLengthMask;
        mFixedTapOut = mBuffer[readIndex];
        
        return sum*mBlend + modTapOut*mFeedForward;
    }
    
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
	MLSample mFixedTapOut;
};


// ----------------------------------------------------------------
#pragma mark MLAllpassDelay
// a delay with one fixed feedback tap and one allpass interpolated
// modulation tap. A dry blend is also summed at the output.

class MLAllpassDelay
{
public:
    MLAllpassDelay() { clear(); }
	~MLAllpassDelay() {}
    
	inline void clear()
    {
        mBuffer.clear();
        mX1 = 0.f;
        mY1 = 0.f;
        mFixedTapOut = 0.;
    }
    inline void setSampleRate(int sr) { mSR = sr; mInvSr = 1.0f / (float)sr; }
    void resize(float duration);
    inline void setMixParams(float b, float ff, float fb) { mBlend = b; mFeedForward = ff; mFeedback = fb; }
    inline void setFixedDelay(float d) { mFixedDelayInSamples = (int)(d*(float)mSR); }
    //inline void setModDelay(float d) { mModDelayInSamples = d*(float)mSR; }
    void setModDelay(float d);
    
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
	MLSample mFixedTapOut;
	MLSample mX1;
	MLSample mY1;
};

// ----------------------------------------------------------------
#pragma mark MLFDN

// A general Feedback Delay Network with N delay lines connected in an NxN matrix.

class MLFDN
{
public:
    MLFDN() :
        mSize(0),
		mSR(44100),
        mFeedbackAmp(0),
        mFreqMul(0.925)
        {}
	~MLFDN()
        {}
    
    // set the number of delay lines.
    void resize(int n);
    void setIdentityMatrix();
    void clear();
    void setSampleRate(int sr);
    void setFreqMul(float m) { mFreqMul = m; }
    void setDelayLengths(float maxLength);
    void setFeedbackAmp(float f) { mFeedbackAmp = f; }
    void setLopass(float f);
    MLSample processSample(const MLSample x);
    
private:
    int mSize;
    int mSR;
    //std::vector<MLAllpassDelay> mDelays;
    std::vector<MLLinearDelay> mDelays;
    std::vector<MLBiquad> mAllpasses;
    std::vector<MLBiquad> mFilters;
    MLSignal mMatrix;
    MLSignal mDelayOutputs;
    float mDelayTime;
    float mFeedbackAmp;
    float mFreqMul;
    float mInvSr;
};

// ----------------------------------------------------------------
#pragma mark MLHalfBandFilter

class MLHalfBandFilter
{
public:
    static const float ka0, ka1, kb0, kb1;

    class AllpassSection
    {
    public:
        AllpassSection();
        ~AllpassSection();
        void clear();
        
        inline MLSample processSample(const MLSample x)
        {
            x1=x0;
            y1=y0;
            x0=x;
            y0 = x1 + (x0 - y1)*a;            
            return y0;
        }
        
        float x0, x1, y0, y1;
        float a;        
    };
    
    MLHalfBandFilter();
    ~MLHalfBandFilter();
    void clear();
    inline MLSample processSampleDown(const MLSample x)
    {
        MLSample y;
        
        if(k)
        {
            a0 = apa1.processSample(apa0.processSample(x));
        }
        else
        {
            b0 = apb1.processSample(apb0.processSample(x));
        }
        
        y = (a0 + b1)*0.5f;
        b1 = b0;
        k = !k;
        return y;
    }
    
    inline MLSample processSampleUp(const MLSample x)
    {
        MLSample y;
        
        if(k)
        {
            a0 = apa1.processSample(apa0.processSample(x));
            y = a0;
        }
        else
        {
            b0 = apb1.processSample(apb0.processSample(x));
            y = b1;
        }

        b1 = b0;
        k = !k;
        return y;
    }
    
private:
    AllpassSection apa0, apa1, apb0, apb1;
    MLBiquad apab;
    float x0, x1;
    float a0, b0, b1;
    bool k;

};

// ----------------------------------------------------------------
#pragma mark MLDownsample2x

class MLDownsample2x
{
public:
    MLDownsample2x(){}
    ~MLDownsample2x(){}
    void clear();
    
    // process n input samples from src and generate n / 2 output samples in dest.
    inline void processVector(const float* src, float* dest, int n)
    {
        int j = 0;
        int nn = n >> 1;
        for(int i = 0; i < nn; ++i)
        {
            f.processSampleDown(src[j++]);
            dest[i] = f.processSampleDown(src[j++]);
        }
    }
    
private:
    MLHalfBandFilter f;
};

// ----------------------------------------------------------------
#pragma mark MLUpsample2x

class MLUpsample2x
{
public:
    MLUpsample2x(){}
    ~MLUpsample2x(){}
    void clear();
    
    // process n input samples from src and generate 2n output samples in dest.
    inline void processVector(const float* src, float* dest, int n)
    {
        int j = 0;
        for(int i = 0; i < n; ++i)
        {
            dest[j++] = f.processSampleUp(src[i]);
            dest[j++] = f.processSampleUp(src[i]);
        }
    }
    
private:
    MLHalfBandFilter f;
};



#endif /* defined(__MLDSPUtils__) */
