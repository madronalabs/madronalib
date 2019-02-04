//
//  MLDSPDeprecated.h
//  Madronalib
//
//  Created by Randy Jones on 1/31/14.
//
//

#pragma once

#include "MLDSP.h"
#include "MLSignal.h"
#include "MLScalarMath.h"
using namespace ml;

#include <cassert>
#include <iostream>
#include <string>

#ifdef _WIN32
#include <memory>
#else
//#include <tr1/memory>
#endif

#include <math.h>
#ifdef _WIN32
#define	MAXFLOAT	((float)3.40282346638528860e+38)
#endif

#ifndef MAXFLOAT
#include <float.h>
#define MAXFLOAT FLT_MAX
#endif

#include <emmintrin.h>

#ifndef DEBUG
#define force_inline  inline __attribute__((always_inline))
#else
#define force_inline  inline
#endif

// logs for Windows.   
#ifdef _WIN32
inline double log2( double n )   
{       
	return log( n ) / log( 2. );   
} 
inline float log2f(float n)
{
	return logf(n) / 0.693147180559945309417232121458176568f;
}
#endif

#ifdef _MSC_VER
#define snprintf _snprintf
#endif // _MSC_VER

// ----------------------------------------------------------------
#pragma mark Types
// ----------------------------------------------------------------

typedef float MLSample;
typedef double MLDouble;

// ----------------------------------------------------------------
#pragma mark Engine Constants
// ----------------------------------------------------------------

const uintptr_t kMLSamplesPerSSEVectorBits = 2;
const uintptr_t kSSEVecSize = 1 << kMLSamplesPerSSEVectorBits;

//const uintptr_t kMLProcessChunkBits = 6;     // signals are always processed in chunks of this size.
//const uintptr_t kMLProcessChunkSize = 1 << kMLProcessChunkBits;
//const uintptr_t kMLProcessChunkVectors = kMLProcessChunkSize << kMLSamplesPerSSEVectorBits;

//const int kMLEngineMaxVoices = 8;

const uintptr_t kMLCacheAlignBits = 6; // cache line is probably 64 bytes
const uintptr_t kMLCacheAlignSize = 1 << kMLCacheAlignBits;
const uintptr_t kMLCacheAlignMask = ~(kMLCacheAlignSize - 1);

const float kMLTwoPi = 6.2831853071795864769252867f;
const float kMLPi = 3.1415926535897932384626433f;
const float kMLOneOverTwoPi = 1.0f / kMLTwoPi;
const float kMLTwelfthRootOfTwo = 1.05946309436f;

const float kMLMinGain = 0.00001f; // 10e-5 = -120dB

const float kMLTimeless = -1.f;
const float kMLToBeCalculated = 0.f;

const MLSample kMLMaxSample = MAXFLOAT;
const MLSample kMLMinSample = -MAXFLOAT;


// ----------------------------------------------------------------
#pragma mark utility functions
// ----------------------------------------------------------------

// return bool as float 0. or 1.
inline float boolToFloat(uint32_t b)
{
	uint32_t temp = 0x3F800000 & (!b - 1);
	return *((float*)&temp);
}

MLSample* alignToCacheLine(const MLSample* p);

int ilog2(int n);

inline MLSample lerp(const MLSample a, const MLSample b, const MLSample m)
{
	return(a + m*(b-a));
}

inline MLSample herp(const MLSample* t, float phase)
{
	// 4-point, 3rd-order Hermite interpolation
	const float c = (t[2] - t[0]) * 0.5f;
	const float v = t[1] - t[2];
	const float w = c + v;
	const float a = w + v + (t[3] - t[1]) * 0.5f;
	const float b = w + a;
	return (((a * phase) - b) * phase + c) * phase + t[1];
}

inline MLSample werp(const MLSample* t, float phase)
{
	// 4-point, 2nd-order Watte trilinear interpolation
	const float threeOverTwo = 1.5f;
	const float oneHalf = 0.5f;
	float ym1py2 = t[0] + t[3];
	float c0 = t[1];
	float c1 = threeOverTwo * t[2] - oneHalf * (t[1] + ym1py2);
	float c2 = oneHalf * (ym1py2 - t[1] - t[2]);
	return (c2 * phase + c1) * phase + c0;
}

float scaleForRangeTransform(float a, float b, float c, float d); // TODO replace with MLRange object
float offsetForRangeTransform(float a, float b, float c, float d);

MLSample MLRand(void);
uint32_t MLRand32(void);
void MLRandReset(void);

// ----------------------------------------------------------------
#pragma mark portable numeric checks
// ----------------------------------------------------------------

#include <cmath>

#ifdef __INTEL_COMPILER
#include <mathimf.h>
#endif

int MLisNaN(float x);
int MLisNaN(double x);
int MLisInfinite(float x);
int MLisInfinite(double x);

// ----------------------------------------------------------------

float inMinusPiToPi(float theta);


// ----------------------------------------------------------------
#pragma mark fast trig approximations
// ----------------------------------------------------------------

// fastest and worst.  rough approximation sometimes useful in [-pi/2, pi/2].
inline float fsin1(const float x)
{
	return x - (x*x*x*0.15f);
}

inline float fcos1(const float x)
{
	float xx = x*x;
	return 1.f - xx*0.5f*(1.f - xx*0.08333333f);
}

// ----------------------------------------------------------------
#pragma mark fast SSE exp2 and log2 approx
// Courtesy José Fonseca, http://jrfonseca.blogspot.com/2008/09/fast-sse2-pow-tables-or-polynomials.html

#define EXP_POLY_DEGREE 3

#define POLY0(x, c0) _mm_set1_ps(c0)
#define POLY1(x, c0, c1) _mm_add_ps(_mm_mul_ps(POLY0(x, c1), x), _mm_set1_ps(c0))
#define POLY2(x, c0, c1, c2) _mm_add_ps(_mm_mul_ps(POLY1(x, c1, c2), x), _mm_set1_ps(c0))
#define POLY3(x, c0, c1, c2, c3) _mm_add_ps(_mm_mul_ps(POLY2(x, c1, c2, c3), x), _mm_set1_ps(c0))
#define POLY4(x, c0, c1, c2, c3, c4) _mm_add_ps(_mm_mul_ps(POLY3(x, c1, c2, c3, c4), x), _mm_set1_ps(c0))
#define POLY5(x, c0, c1, c2, c3, c4, c5) _mm_add_ps(_mm_mul_ps(POLY4(x, c1, c2, c3, c4, c5), x), _mm_set1_ps(c0))

inline __m128 exp2Approx4(__m128 x)
{
	__m128i ipart;
	__m128 fpart, expipart, expfpart;
	
	x = _mm_min_ps(x, _mm_set1_ps( 129.00000f));
	x = _mm_max_ps(x, _mm_set1_ps(-126.99999f));
	
	// ipart = int(x - 0.5) 
	ipart = _mm_cvtps_epi32(_mm_sub_ps(x, _mm_set1_ps(0.5f)));
	
	// fpart = x - ipart 
	fpart = _mm_sub_ps(x, _mm_cvtepi32_ps(ipart));
	
	// expipart = (float) (1 << ipart) 
	expipart = _mm_castsi128_ps(_mm_slli_epi32(_mm_add_epi32(ipart, _mm_set1_epi32(127)), 23));
	
	// minimax polynomial fit of 2**x, in range [-0.5, 0.5[ 
#if EXP_POLY_DEGREE == 5
	expfpart = POLY5(fpart, 9.9999994e-1f, 6.9315308e-1f, 2.4015361e-1f, 5.5826318e-2f, 8.9893397e-3f, 1.8775767e-3f);
#elif EXP_POLY_DEGREE == 4
	expfpart = POLY4(fpart, 1.0000026f, 6.9300383e-1f, 2.4144275e-1f, 5.2011464e-2f, 1.3534167e-2f);
#elif EXP_POLY_DEGREE == 3
	expfpart = POLY3(fpart, 9.9992520e-1f, 6.9583356e-1f, 2.2606716e-1f, 7.8024521e-2f);
#elif EXP_POLY_DEGREE == 2
	expfpart = POLY2(fpart, 1.0017247f, 6.5763628e-1f, 3.3718944e-1f);
#else
#error
#endif
	
	return _mm_mul_ps(expipart, expfpart);
}

#define LOG_POLY_DEGREE 5

inline __m128 log2Approx4(__m128 x)
{
	__m128i exp = _mm_set1_epi32(0x7F800000);
	__m128i mant = _mm_set1_epi32(0x007FFFFF);
	
	__m128 one = _mm_set1_ps( 1.0f);
	
	__m128i i = _mm_castps_si128(x);
	
	__m128 e = _mm_cvtepi32_ps(_mm_sub_epi32(_mm_srli_epi32(_mm_and_si128(i, exp), 23), _mm_set1_epi32(127)));
	
	__m128 m = _mm_or_ps(_mm_castsi128_ps(_mm_and_si128(i, mant)), one);
	
	__m128 p;
	
	// Minimax polynomial fit of log2(x)/(x - 1), for x in range [1, 2[ 
#if LOG_POLY_DEGREE == 6
	p = POLY5( m, 3.1157899f, -3.3241990f, 2.5988452f, -1.2315303f,  3.1821337e-1f, -3.4436006e-2f);
#elif LOG_POLY_DEGREE == 5
	p = POLY4(m, 2.8882704548164776201f, -2.52074962577807006663f, 1.48116647521213171641f, -0.465725644288844778798f, 0.0596515482674574969533f);
#elif LOG_POLY_DEGREE == 4
	p = POLY3(m, 2.61761038894603480148f, -1.75647175389045657003f, 0.688243882994381274313f, -0.107254423828329604454f);
#elif LOG_POLY_DEGREE == 3
	p = POLY2(m, 2.28330284476918490682f, -1.04913055217340124191f, 0.204446009836232697516f);
#else
#error
#endif
	
	// This effectively increases the polynomial degree by one, but ensures that log2(1) == 0
	p = _mm_mul_ps(p, _mm_sub_ps(m, one));
	
	return _mm_add_ps(p, e);
}

// ----------------------------------------------------------------
// interpolation
// ----------------------------------------------------------------

// ----------------------------------------------------------------
#pragma mark  MLRange
// ----------------------------------------------------------------

// TODO remove everywhere and use new ml::Interval and ml::IntervalProjection

class MLRange
{
public:
	MLRange() : mA(0.f), mB(1.f), mScale(1.f), mOffset(0.f), mClip(false), mMinOutput(0), mMaxOutput(0) {}
	MLRange(float a, float b) : mA(a), mB(b), mScale(1.f), mOffset(0.f), mClip(false), mMinOutput(0), mMaxOutput(0) {}
	MLRange(float a, float b, float c, float d, bool clip = false) : 
	mA(a), mB(b), mScale(1.f), mOffset(0.f), mClip(clip), mMinOutput(0), mMaxOutput(0) 
	{
		convertTo(MLRange(c, d));
	}
	~MLRange(){}
	float getA() const {return mA;}
	float getB() const {return mB;}
	void setA(float f){mA = f;} 
	void setB(float f){mB = f;}  
	void set(float a, float b){mA = a; mB = b;}
	void setClip(bool c)
	{
		mClip = c;
	} 
	bool getClip() const { return mClip; } 
	
	// no
	void convertFrom(const MLRange& r)
	{
		float a, b, c, d;
		a = r.mA;
		b = r.mB;
		c = mA;
		d = mB;
		mScale = (d - c) / (b - a);
		mOffset = (a*d - b*c) / (a - b);
		mMinOutput = ml::min(c, d);
		mMaxOutput = ml::max(c, d);
	}
	
	// no
	void convertTo(const MLRange& r)
	{
		float a, b, c, d;
		a = mA;
		b = mB;
		c = r.mA;
		d = r.mB;
		mScale = (d - c) / (b - a);
		mOffset = (a*d - b*c) / (a - b);
		mMinOutput = ml::min(c, d);
		mMaxOutput = ml::max(c, d);
	}
	
	float operator()(float f) const 
	{
		float r = f*mScale + mOffset;
		if(mClip) r = ml::clamp(r, mMinOutput, mMaxOutput);
		return r;
	}
	
	inline float convert(float f) const
	{
		return f*mScale + mOffset;
	}
	
	inline float convertAndClip(float f) const
	{
		return ml::clamp((f*mScale + mOffset), mMinOutput, mMaxOutput);
	}
	
	inline bool contains(float f) const
	{
		return ((f > mMinOutput) && (f < mMaxOutput));
	}
	
private:
	float mA;
	float mB;
	float mScale;
	float mOffset;
	bool mClip;
	float mMinOutput;
	float mMaxOutput;
};

extern const MLRange UnityRange;


// ----------------------------------------------------------------
// old DSPUtils


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
		
		float absm = fabsf(fm);	// TODO fast abs etc
		float pos = fm > 0.;
		float neg = fm < 0.;
		float q = pos*fc + neg*fa;
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
		y[n] = ml::clamp(a[n], b, c);		
	}
	return y;
}		


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
	inline float processSample(float x)
	{
		const float out = a0*x + a1*x1 + a2*x2 + b1*y1 + b2*y2;
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
			const float out = a0*x + a1*x1 + a2*x2 + b1*y1 + b2*y2;
			x2 = x1;
			x1 = x;
			y2 = y1;
			y1 = out;
			y[n] = out;
		}		
		return y;
	}
	
	// MLTEST to DSPVector
	inline void processSignalInPlace(MLSignal& in)
	{
		int frames = in.getWidth();
		for(int n=0; n<frames; ++n)
		{
			const float x = in[n];
			const float out = a0*x + a1*x1 + a2*x2 + b1*y1 + b2*y2;
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
			const float out = pa0[n]*x + pa1[n]*x1 + pa2[n]*x2 + pb1[n]*y1 + pb2[n]*y2;
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
		
		float clampedFreq = ml::clamp(f, 20.f, maxFreq);
		mOneMinusQ = 1.f - (maxQ*q);
		mOneMinusQ = ml::clamp(mOneMinusQ, 0.f, 0.9f);
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
		ka = ml::clamp(kMLTwoPi*fa*mInvSr, 0.f, 0.25f);
		kb = ml::clamp(kMLTwoPi*fb*mInvSr, 0.f, 0.25f);
	}
	void setAttackAndReleaseTimes(float tAttack, float tRelease)
	{
		ka = ml::clamp(kMLTwoPi*(1.0f/tAttack)*mInvSr, 0.f, 0.25f);
		kb = ml::clamp(kMLTwoPi*(1.0f/tRelease)*mInvSr, 0.f, 0.25f);
	}
	
	// TODO deprecate
    inline float processSample(float x)
    {
        float dxdt = x - y1;
        float s = (dxdt < 0.f ? -1.f : 1.f);
        float k = ((1.f - s)*kb + (1.f + s)*ka)*0.5f;
        float out = y1 + k*dxdt;
        y1 = out;
        return(out);
    }
	
	inline float operator()(float x)
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
	
	inline float operator()(float x)
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
	
    inline float processSample()
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
	inline void setFrequency(float f) { mStep32 = (int)(mInvSrDomain * f); }
	inline float processSample()
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
	inline float processSample()
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
#pragma mark floatDelay
// a simple delay in integer samples with no mixing.

class floatDelay
{
public:
    floatDelay() { clear(); }
	~floatDelay() {}
    
	void resize(float duration);
	
	inline void clear(){ mBuffer.clear(); mWriteIndex = 0; }	
	inline void setSampleRate(int sr) { mSR = sr; mInvSr = 1.0f / (float)sr; }
    inline void setDelay(float d) { mDelayInSamples = (int)(d*(float)mSR); }

    inline float processSample(const float x)
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
	
	inline float processSample(const float x)
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
	
	inline float processSample(const float x)
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
	
	float mBlend, mFeedForward, mFeedback;
	float mFixedTapOut;
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
    
    float processSample(const float x);
	
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
    
    int mFixedDelayInSamples;
    float mModDelayInSamples;
    
    float mBlend, mFeedForward, mFeedback;	
	float mFixedTapOut;
	float mX1;
	float mY1;
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
	float processSample(const float x);
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
        
        inline float processSample(const float x)
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
    inline float processSampleDown(const float x)
    {
        float y;
        
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
    
    inline float processSampleUp(const float x)
    {
        float y;
        
        if(k)
        {
            y = apa1.processSample(apa0.processSample(x));
        }
        else
        {
            y = apb1.processSample(apb0.processSample(x));
        }

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


