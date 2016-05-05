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
// This modules should include any DSP functors that we typically want to use from 
// C++ code directly. They will typically be used to implement MLProcs, which can
// be a lot more complicated. 
//
// DSPUtils:
// - are stateless functions if they don't need a sampling rate or memory (add, multiply, etc)
// - are functors if they need a sampling rate or memory (filters, oscillators etc)
// - should be inlined
// - loops should be fixed-sized at compile time and thereby unrollable
// - should use static binding of operator()
// - should output a single DSPVector from operator()(const DSPVector& in1 ...)
// - may need a sample rate to be set
// - may have static data such as tables, created using a
//	singleton pattern when the first object is made
// - do not require any other infrastructure
//
// TODO we can make some of these qualities explicit in the code with a templated base class 
// and CRTP https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern
// to overload operator() (DSPVector) for each proc class
// base class will have operator()<T> for each proc.
// 
// TODO this is all a statement of intent to refactor things—most
// of the functions here are not yet in line with the guidelines above
//
// for dynamically dispatched objects, making graphs from JSON, etc, see MLProcs.

// TODO test pass by value everywhere idea: 
// - look for difference in generated code of pass by value vs. pointer. 
// - write one fairly complex object with both value and pointer, compare and time.

// best idea so far?
// DSPutils can operate on the assumption of default size signals: 16 x 1 or whatever.
// and, if making default signals is fast enough (TIME IT) we can return new default sigs by value.
// these can coexist with slower matrix-like MLSignal methods that actually do range checking. 



inline MLSignal reciprocalEst(const MLSignal& x)
{
	int frames = x.getWidth();
	MLSignal y(frames); 
	for(int n=0; n<frames; ++n)
	{
		// MLTEST use SSE
		y[n] = 1.f/x[n];
	}
	return y;
}		


inline void scaleAndAccumulate(MLSignal& a, const MLSignal& b, float k)
{
	int vectors = a.getSize() >> kMLSamplesPerSSEVectorBits;	
	float* pa = a.getBuffer();
	const float* pb = b.getConstBuffer();
	__m128 va, vb, vk;
	
	vk = _mm_set1_ps(k);
	
	for(int v=0; v<vectors; ++v)
	{
		va = _mm_load_ps(pa);
		vb = _mm_load_ps(pb);
		_mm_store_ps(pa, _mm_add_ps(va, _mm_mul_ps(vb, vk)));
		pa += kSSEVecSize;
		pb += kSSEVecSize;
	}
}

inline void scaleByConstant(MLSignal& a, const MLSignal& b, float k)
{
	int vectors = a.getSize() >> kMLSamplesPerSSEVectorBits;	
	float* pa = a.getBuffer();
	const float* pb = b.getConstBuffer();
	__m128 vb, vk;
	
	vk = _mm_set1_ps(k);
	
	for(int v=0; v<vectors; ++v)
	{
		vb = _mm_load_ps(pb);
		_mm_store_ps(pa, _mm_mul_ps(vb, vk));
		pa += kSSEVecSize;
		pb += kSSEVecSize;
	}
}

// keep scalar version? make some macros? :-P
/*
inline MLSignal abs(const MLSignal& x)
{
	int frames = x.getWidth();
	MLSignal y(frames); 
	for(int n=0; n<frames; ++n)
	{
		y[n] = fabs(x[n]);
	}
	return y;
}		
*/

inline void abs(MLSignal& x) 
{
	int frames = x.getWidth();
	int vectors = frames >> kMLSamplesPerSSEVectorBits;
	float* px = x.getBuffer();
	__m128 vx;
	
	static const __m128 sign_mask = _mm_set1_ps(-0.f); // -0.f = 1 << 31
		
	for(int v=0; v<vectors; ++v)
	{
		vx = _mm_load_ps(px);
		vx = _mm_andnot_ps(sign_mask, vx);
		_mm_store_ps(px, vx);
		px += kSSEVecSize;
	}
}

// MLTEST
// TODO: fixed size DSPVectors, SSE
inline MLSignal lerp(const MLSignal& b, const MLSignal& c, const MLSignal& m)
{
	int frames = b.getWidth();
	
	// SETDIMS
	MLSignal y(frames); 
	for(int n=0; n<frames; ++n)
	{
		float fb = b[n];
		float fc = c[n];
		float fm = m[n];
		y[n] = (fb + (fc - fb)*fm);		
	}
	return y;
}		

inline MLSignal lerp(const MLSignal& b, const MLSignal& c, const float m)
{
	int frames = b.getWidth();
	MLSignal y(frames); 
	for(int n=0; n<frames; ++n)
	{
		float fb = b[n];
		float fc = c[n];
		y[n] = (fb + (fc - fb)*m);		
	}
	return y;
}		

inline MLSignal lerpBipolar(const MLSignal& a, const MLSignal& b, const MLSignal& c, const MLSignal& m)
{
	int frames = a.getWidth();
	
	// SETDIMS
	MLSignal y(frames); 
	for(int n=0; n<frames; ++n)
	{
		float fa = a[n];
		float fb = b[n];
		float fc = c[n];
		float fm = m[n];
		
		MLSample absm = fabsf(fm);	// TODO fast abs etc
		MLSample pos = fm > 0.;
		MLSample neg = fm < 0.;
		MLSample q = pos*fc + neg*fa;
		y[n] = (fb + (q - fb)*absm);
		
	}
	return y;
}		

inline MLSignal clamp(const MLSignal& a, const float b, const float c)
{
	int frames = a.getWidth();
	
	// SETDIMS
	MLSignal y(frames); 
	for(int n=0; n<frames; ++n)
	{
		y[n] = clamp(a[n], b, c);		
	}
	return y;
}		



// MLTEST

namespace ml
{
	namespace biquadCoeffs
	{
		inline MLSignal onePole(float omega)
		{
			float e = 2.718281828;
			float x = powf(e, -omega);
			float a0 = 1.f - x;
			float a1 = 0.f;
			float a2 = 0.f;
			float b1 = -x;
			float b2 = 0.f;
			return MLSignal{a0, a1, a2, b1, b2};
		}

		inline MLSignal loShelf(float omega, float q, float gain)
		{
			// lowShelf: H(s) = A * (s^2 + (sqrt(A)/Q)*s + A)/(A*s^2 + (sqrt(A)/Q)*s + 1)
			float A = gain;
			float aMinus1 = A - 1.0f;
			float aPlus1 = A + 1.0f;
			float cosOmega = cosf(omega);
			float alpha = sinf(omega) / (2.f * q);
			float beta = 2.0f*sqrtf(A)*alpha;
			
			float b0 = aPlus1 + aMinus1*cosOmega + beta;			
			float a0 = (A*(aPlus1 - aMinus1*cosOmega + beta)) / b0;
			float a1 = (A*(aPlus1*-2.0f*cosOmega + 2.0f*aMinus1)) / b0;
			float a2 = (A*(aPlus1 - aMinus1*cosOmega - beta)) / b0;
			float b1 = (aPlus1*-2.0f*cosOmega - 2.0f*aMinus1) / b0;
			float b2 = (aPlus1 + aMinus1*cosOmega - beta) / b0;
			
			return MLSignal{a0, a1, a2, b1, b2};
		}
		inline MLSignal hiShelf(float omega, float q, float gain)
		{
			// highShelf: H(s) = A * (A*s^2 + (sqrt(A)/Q)*s + 1)/(s^2 + (sqrt(A)/Q)*s + A)
			float A = gain;
			float aMinus1 = A - 1.0f;
			float aPlus1 = A + 1.0f;
			float cosOmega = cosf(omega);
			float alpha = sinf(omega) / (2.f * q);
			float beta = 2.0f*sqrtf(A)*alpha;

			float b0 = aPlus1 - aMinus1*cosOmega + beta;			
			float a0 = (A*(aPlus1 + aMinus1*cosOmega + beta)) / b0;
			float a1 = (A*(aPlus1*-2.0f*cosOmega + -2.0f*aMinus1)) / b0;
			float a2 = (A*(aPlus1 + aMinus1*cosOmega - beta)) / b0;
			float b1 = (aPlus1*-2.0f*cosOmega + 2.0f*aMinus1) / b0;
			float b2 = (aPlus1 - aMinus1*cosOmega - beta) / b0;
			
			return MLSignal{a0, a1, a2, b1, b2};
		}
	}
	
	namespace svfCoeffs
	{
		// get bandpass coeffs where k = 1/q (damping factor)
		inline MLSignal bandpass(float fOverSr, float k)
		{
			float omega = kMLPi*fOverSr;
			float s1 = sin(omega);
			float s2 = sin(2.0f*omega);
			float nrm = 1.0f/(2.f + k*s2);
			float g0 = s2*nrm;
			float g1 = (-2.f*s1*s1 - k*s2)*nrm;
			float g2 = (2.0f*s1*s1)*nrm;
			
			// SETDIMS
			return MLSignal{g0, g1, g2};
		}
		
		// tried fast sin approx, which did not hold up. try better approximations.
	}
	
	
	// TODO duh,    all SVF modes have the same coeffs, rename above
	
	inline MLSignal getSVFCoeffs(float fOverSr, float k)
	{
		float omega = kMLPi*fOverSr;
		float s1 = sin(omega);
		float s2 = sin(2.0f*omega);
		float nrm = 1.0f/(2.f + k*s2);
		float g0 = s2*nrm;
		float g1 = (-2.f*s1*s1 - k*s2)*nrm;
		float g2 = (2.0f*s1*s1)*nrm;		
		return MLSignal{g0, g1, g2, k};
	}
}	
	
// experimental: bandpass only.

class MLSVF
{
public:
	MLSVF() : g0(0), g1(0), g2(0), k(1) { clear(); }
	void clear()
	{
		ic1eq = ic2eq = 0.f;
	}
	void setCoefficients(const MLSignal& coeffs)
	{
		g0 = coeffs[0];
		g1 = coeffs[1];
		g2 = coeffs[2];
		k = coeffs[3];
	}
	
	inline MLSignal operator()(const MLSignal& in)
	{
		int frames = in.getWidth();
		MLSignal y(frames); 
		for(int n=0; n<frames; ++n)
		{
			float v0 = in[n];
			float t0 = v0 - ic2eq;
			float t1 = g0*t0 + g1*ic1eq;
			float t2 = g2*t0 + g0*ic1eq;
			float v1 = t1 + ic1eq;
			//			float v2 = t2 + ic2eq;
			ic1eq += 2.0f*t1;
			ic2eq += 2.0f*t2;
			y[n] = v1;
		}		
		return y;
	}
	
	inline MLSignal processLopass(const MLSignal& in)
	{
		int frames = in.getWidth();
		
		// SETDIMS a problem! 
		MLSignal y(frames); 
		for(int n=0; n<frames; ++n)
		{
			float v0 = in[n];
			float t0 = v0 - ic2eq;
			float t1 = g0*t0 + g1*ic1eq;
			float t2 = g2*t0 + g0*ic1eq;
			//float v1 = t1 + ic1eq;
			float v2 = t2 + ic2eq;
			ic1eq += 2.0f*t1;
			ic2eq += 2.0f*t2;
			y[n] = v2;
		}		
		return y;
	}
	
	inline MLSignal processHipass(const MLSignal& in)
	{
		int frames = in.getWidth();
		MLSignal y(frames); 
		for(int n=0; n<frames; ++n)
		{
			float v0 = in[n];
			float t0 = v0 - ic2eq;
			float t1 = g0*t0 + g1*ic1eq;
			float t2 = g2*t0 + g0*ic1eq;
			float v1 = t1 + ic1eq;
			float v2 = t2 + ic2eq;
			ic1eq += 2.0f*t1;
			ic2eq += 2.0f*t2;
			y[n] = v0 - k*v1 - v2;
		}		
		return y;
	}
	
	// MLTEST
	inline void processSignal(MLSignal& y)
	{
		int frames = y.getWidth();
		for(int n=0; n<frames; ++n)
		{
			float v0 = y[n];
			float t0 = v0 - ic2eq;
			float t1 = g0*t0 + g1*ic1eq;
			float t2 = g2*t0 + g0*ic1eq;
			float v1 = t1 + ic1eq;
			//			float v2 = t2 + ic2eq;
			ic1eq += 2.0f*t1;
			ic2eq += 2.0f*t2;
			y[n] = v1;
		}		
	}
	
	inline float processSample(float x)
	{
		float v0 = x;
		float t0 = v0 - ic2eq;
		float t1 = g0*t0 + g1*ic1eq;
		float t2 = g2*t0 + g0*ic1eq;
		float v1 = t1 + ic1eq;
		//			float v2 = t2 + ic2eq;
		ic1eq += 2.0f*t1;
		ic2eq += 2.0f*t2;
		return v1;
	}
	void setSampleRate(float) {} // temp
	
	float g0, g1, g2;
	float k; // needed for high, notch, peak
	float ic1eq, ic2eq;
};

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
	void setCoefficients(const MLSignal& coeffs)
	{
		a0 = coeffs[0];
		a1 = coeffs[1];
		a2 = coeffs[2];
		b1 = coeffs[3];
		b2 = coeffs[4];
	}
	
	// set the internal state of the filter as if the output has been at
	// the value f indefinitely. May cause a discontinuity in output.
	void setState(float f);
	
	// TODO deprecate in favor of operator()
	inline MLSample processSample(float x)
	{
		const float out = a0*x + a1*x1 + a2*x2 - b1*y1 - b2*y2;
		x2 = x1;
		x1 = x;
		y2 = y1;
		y1 = out;
		return(out);
	}
	
	// MLTEST to DSPVector
	inline MLSignal operator()(const MLSignal& in)
	{
		int frames = in.getWidth();
		MLSignal y(frames); 
		for(int n=0; n<frames; ++n)
		{
			const float x = in[n];
			const float out = a0*x + a1*x1 + a2*x2 - b1*y1 - b2*y2;
			x2 = x1;
			x1 = x;
			y2 = y1;
			y1 = out;
			y[n] = out;
		}		
		return y;
	}
	
	// MLTEST to DSPVector
	// process a signal in place. should be called that?
	inline void processSignal(MLSignal& in)
	{
		int frames = in.getWidth();
		for(int n=0; n<frames; ++n)
		{
			const float x = in[n];
			const float out = a0*x + a1*x1 + a2*x2 - b1*y1 - b2*y2;
			x2 = x1;
			x1 = x;
			y2 = y1;
			y1 = out;
			in[n] = out;
		}		
	}
	
	// MLTEST to DSPVector
	// use an interpolated coefficient matrix that must contain input width x 5 rows
	inline MLSignal operator()(const MLSignal& in, const MLSignal& coeffs)
	{
		int frames = in.getWidth();
		MLSignal y(frames); 
		
		// 5 rows of coefficients
		const float* pB = coeffs.getConstBuffer();
		const float* pa0 = pB + coeffs.row(0);
		const float* pa1 = pB + coeffs.row(1);
		const float* pa2 = pB + coeffs.row(2);
		const float* pb1 = pB + coeffs.row(3);
		const float* pb2 = pB + coeffs.row(4);
		
		for(int n=0; n<frames; ++n)
		{
			const float x = in[n];
			// hmm maybe should be horizontal for SSE
			const float out = pa0[n]*x + pa1[n]*x1 + pa2[n]*x2 - pb1[n]*y1 - pb2[n]*y2;
			x2 = x1;
			x1 = x;
			y2 = y1;
			y1 = out;
			y[n] = out;
		}		
		return y;
	}
	
	inline float getOutput() { return y1; }
	
	float a0, a1, a2, b1, b2;
	float x1, x2, y1, y2;
	float mInvSr;
};


// ----------------------------------------------------------------
#pragma mark MLBandpass

class MLBandpass
{
public:
	static const int kOversample = 1;
	
	MLBandpass() { mInvSr = 1.f; clear(); }
	~MLBandpass() {}
	
	void clear() 
	{
		mInState = mLoState = mBandState = 0.f;
		mOmega = 0.f;
	}
	void setSampleRate(float sr) { mSr = sr; mInvSr = 1.f/sr; }
	void setBandpass(float f, float q)
	{
		const float oversample = 1.f/kOversample; 
		const float invOver = mInvSr*oversample;
		
		const float maxQ = 0.95f;
		const float maxFreq = mSr*0.5f;
		
		float clampedFreq = clamp(f, 20.f, maxFreq);
		mOneMinusQ = 1.f - (maxQ*q);
		mOneMinusQ = clamp(mOneMinusQ, 0.f, 0.9f);
		mOmega = 2.0f * fsin1(kMLPi * clampedFreq * invOver);		
	}
	
	inline float processSample(float x)
	{	
		const float k3 = mInvSr*2.f; // TODO tune to match SEM lo freq res damping
		mInState = x;		
		//for(int i=0; i<kOversample; ++i) // TODO verify this is unrolled
		{
			mLoState += mOmega * mBandState;
			mLoState -= mLoState*mLoState*mLoState*k3;						
			mBandState += mOmega*(mInState - mLoState - mBandState*mOneMinusQ); // SEMclip(mBandState, oneMinusQ);
		}
		return mBandState;
	}
	
	// MLTEST to DSPVector
	inline MLSignal operator()(const MLSignal& x)
	{
		int frames = x.getWidth();
		MLSignal y(frames); 
		for(int n=0; n<frames; ++n)
		{
			y[n] = processSample(x[n]);
		}		
		return y;
	}
	
	float mInState, mLoState, mBandState, mOmega;
	float mSr, mInvSr, mOneMinusQ;
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
	
	// TODO deprecate
    inline MLSample processSample(float x)
    {
        float dxdt = x - y1;
        float s = (dxdt < 0.f ? -1.f : 1.f);
        float k = ((1.f - s)*kb + (1.f + s)*ka)*0.5f;
        float out = y1 + k*dxdt;
        y1 = out;
        return(out);
    }
	
	inline MLSample operator()(float x)
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
#pragma mark MLDifference

class MLDifference
{
public:
	MLDifference() : mX1(0) {}
	~MLDifference(){}
	
	inline MLSample operator()(float x)
	{
		float d = x - mX1;
		mX1 = x;
		return d;
	}
private:
	float mX1;
};

// ----------------------------------------------------------------
#pragma mark MLSineOsc

// this sine generator makes a looping counter by letting a 32 bit word overflow. The frequency 
// resolution of this counter is good enough for most uses: around 0.000005 Hz.
class MLSineOsc
{
public:
    static const float kIntDomain, kRootX, kOneSixth, kRange, kDomain, kScale, kDomainScale, kPhaseInvScale, kFlipOffset;
    MLSineOsc() : mOmega32(0), mStep32(0) { }
    ~MLSineOsc(){}
    
    inline void setSampleRate(int sr) { mInvSrDomain = (float)kIntDomain / (float)sr; }
    inline void setFrequency(float f) { mStep32 = (int)(mInvSrDomain * f); }
	inline void setPhase(float f) { mOmega32 = f*kPhaseInvScale; }
	
    inline MLSample processSample()
    {
        float x, fOmega;
        
        // add increment with wrap
        mOmega32 += mStep32;
        
        // scale to sin approx domain
        fOmega = mOmega32 * kDomainScale + kRootX;
        
        // reverse upper half to make sin wave approx
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
#pragma mark MLPhaseOsc

class MLPhaseOsc
{
public:
	static const float kIntDomain, kDomainScale;
	MLPhaseOsc() : mStep32(0) { clear(); }
	~MLPhaseOsc(){}
	
	inline void clear() { mOmega32 = 0; }
	inline void setSampleRate(int sr) { mInvSrDomain = (float)kIntDomain / (float)sr; }
	inline void setFrequency(float f) { mStep32 = (int)(mInvSrDomain * f); }
	inline MLSample processSample()
	{
		// add increment with wrap
		mOmega32 += mStep32;
		
		// scale to [0, 1]
		return mOmega32*kDomainScale + 0.5f;
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
    
	void resize(float duration);
	
	inline void clear(){ mBuffer.clear(); mWriteIndex = 0; }	
	inline void setSampleRate(int sr) { mSR = sr; mInvSr = 1.0f / (float)sr; }
    inline void setDelay(float d) { mDelayInSamples = (int)(d*(float)mSR); }

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
#pragma mark MLModDelay
// a delay with one linear interpolated modulation tap. 

class MLModDelay
{
public:
	inline void clear()
	{
		mBuffer.clear();
		mWriteIndex = 0;
	}
	inline void setSampleRate(int sr) { mSR = sr; }
	inline void setMaxDelayTime(float durationInSeconds)
	{
		mBuffer.setDims(static_cast<int>(ceilf(durationInSeconds*(float)mSR)));
		mLengthMask = (1 << mBuffer.getWidthBits()) - 1;
		clear();
	}
	inline void setDelayTime(float delayInSeconds) 
	{ 
		mModDelayInSamples = delayInSeconds*(float)mSR; 
		float fDelayInt = floorf(mModDelayInSamples);
		mDelayInt = static_cast<int>(fDelayInt);
		
		// get linear interpolation coefficient D
		mD = mModDelayInSamples - fDelayInt;
	}
	
	inline MLSample processSample(const MLSample x)
	{
		mBuffer[mWriteIndex & mLengthMask] = x;
		mWriteIndex++;
				
		const uintptr_t readIndex = mWriteIndex - mDelayInt;
		const uintptr_t readIndex2 = readIndex - 1;
		
		float a = mBuffer[readIndex & mLengthMask];
		float b = mBuffer[readIndex2 & mLengthMask];
		return lerp(a, b, mD);
	}
	
	// MLTEST towards DSPVectors
	inline MLSignal operator()(const MLSignal& x)
	{
		int frames = x.getWidth();
		
		// SETDIMS
		MLSignal y(frames); 
		for(int n=0; n<frames; ++n)
		{
			y[n] = processSample(x[n]);
		}
		return y;
	}
	
	// MLTEST towards DSPVectors
	inline MLSignal operator()(const MLSignal& x, const MLSignal& delay)
	{
		int frames = x.getWidth();
		MLSignal y(frames); 
		for(int n=0; n<frames; ++n)
		{
			// MLTEST slow!
			setDelayTime(delay[n]);
			
			y[n] = processSample(x[n]);
		}
		return y;
	}
	
private:
	MLSignal mBuffer;
	
	int mSR;
	uintptr_t mWriteIndex;
	uintptr_t mLengthMask;
	uintptr_t mDelayInt;
	float mModDelayInSamples;
	float mD;
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
	inline void setModDelay(float d) { mModDelayInSamples = d*(float)mSR; }
	
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
	inline void setModDelay(float d) { mModDelayInSamples = d*(float)mSR; }
    
    MLSample processSample(const MLSample x);
	
	// MLTEST towards DSPVectors
	inline MLSignal operator()(const MLSignal& x)
	{
		int frames = x.getWidth();
		MLSignal y(frames); 
		for(int n=0; n<frames; ++n)
		{
			y[n] = processSample(x[n]);
		}
		return y;
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
	MLSignal operator()(const MLSignal& x);
    
private:
    int mSize;
    int mSR;
    //std::vector<MLAllpassDelay> mDelays;
    std::vector<MLLinearDelay> mDelays;
    std::vector<MLBiquad> mFilters; // TODO onepole bank object
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

// ----------------------------------------------------------------
#pragma mark RandomSource

namespace ml
{		
	class RandomSource
	{
	public:
		RandomSource() : mSeed(0) {}
		~RandomSource() {}
		
		inline float getSample()
		{
			mSeed = mSeed * 0x0019660D + 0x3C6EF35F;
			uint32_t temp = (mSeed >> 9) & 0x007FFFFF;
			temp &= 0x007FFFFF;
			temp |= 0x3F800000;
			float* pf = reinterpret_cast<float*>(&temp);
			return (*pf)*2.f - 3.f;			
		}
		
		// TODO DSPVector operator()()
		inline DSPVector operator()()
		{
			DSPVector y;
			for(int i=0; i<kDSPVectorSizeFloat; ++i)
			{
				y[i] = getSample();
			}
			return y;
		}
		
		void reset() { mSeed = 0; }
		
	private:
		uint32_t mSeed = 0;
		
	};
}



#endif /* defined(__MLDSPUtils__) */
