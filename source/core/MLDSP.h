
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef _ML_DSP_H
#define _ML_DSP_H

#include <cassert>
#include <iostream>
#include <string>

#ifdef _WIN32
	#include <memory>
#else
	//#include <tr1/memory>
#endif

#include "math.h"
#ifdef _WIN32
	#define	MAXFLOAT	((float)3.40282346638528860e+38)
#endif

#ifndef MAXFLOAT
	#include <float.h>
	#define MAXFLOAT FLT_MAX
#endif

#ifdef __SSE__
#include <xmmintrin.h>
#endif

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
typedef float MLParamValue;

// ----------------------------------------------------------------
#pragma mark Engine Constants
// ----------------------------------------------------------------

const uintptr_t kMLProcessChunkBits = 6;     // signals are always processed in chunks of this size.
const uintptr_t kMLProcessChunkSize = 1 << kMLProcessChunkBits;

const uintptr_t kMLSamplesPerSSEVectorBits = 2;
const uintptr_t kSSEVecSize = 1 << kMLSamplesPerSSEVectorBits;

const int kMLEngineMaxVoices = 8;

const uintptr_t kMLAlignBits = 6; // cache line is 64 bytes
const uintptr_t kMLAlignSize = 1 << kMLAlignBits;
const uintptr_t kMLAlignMask = ~(kMLAlignSize - 1);

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

// return sign bit of float as float, 1. for positive, 0. for negative.
inline float fSignBit(float f)
{
	uint32_t a = *((uint32_t*)&f);
	a = (((a & 0x80000000) >> 31) - 1) & 0x3F800000;
	return *((float*)&a);
}

MLSample* alignToCacheLine(const MLSample* p);
int bitsToContain(int n);
int ilog2(int n);

// TODO make these interpolators lambdas , useful with signals

inline MLSample lerp(const MLSample a, const MLSample b, const MLSample m)
{
	return(a + m*(b-a));
}

inline MLSample lerpBipolar(const MLSample a, const MLSample b, const MLSample c, const MLSample m)
{
	MLSample absm = fabsf(m);	// TODO fast abs etc
	MLSample pos = m > 0.;
	MLSample neg = m < 0.;
	MLSample q = pos*c + neg*a;
	return (b + (q - b)*absm);
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
#pragma mark min, max, clamp
// ----------------------------------------------------------------

template <class c>
inline c (min)(const c& a, const c& b)
{
	return (a < b) ? a : b;
}

template <class c>
inline c (max)(const c& a, const c& b)
{
	return (a > b) ? a : b;
}

template <class c>
inline c (clamp)(const c& x, const c& min, const c& max)
{
	return (x < min) ? min : (x > max ? max : x);
}

// within range, including start, excluding end value.
template <class c>
inline bool (within)(const c& x, const c& min, const c& max)
{
	return ((x >= min) && (x < max));
}

template <class c>
inline int (sign)(const c& x)
{
	if (x == 0) return 0;
	return (x > 0) ? 1 : -1;
}

float inMinusPiToPi(float theta);


// amp <-> dB conversions, where ratio of the given amplitude is to 1.

inline float ampTodB(float a)
{
	return 20.f * log10f(a);
}	

inline float dBToAmp(float dB)
{
	return powf(10.f, dB/20.f);
}	

#pragma mark smoothstep

inline float smoothstep(float a, float b, float x)
{
	x = clamp((x - a)/(b - a), 0.f, 1.f); 
	return x*x*(3.f - 2.f*x);
}

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
	
	void convertFrom(const MLRange& r)
	{
		float a, b, c, d;
		a = r.mA;
		b = r.mB;
		c = mA;
		d = mB;
		mScale = (d - c) / (b - a);
		mOffset = (a*d - b*c) / (a - b);
		mMinOutput = min(c, d);
		mMaxOutput = max(c, d);
	}
	void convertTo(const MLRange& r)
	{
		float a, b, c, d;
		a = mA;
		b = mB;
		c = r.mA;
		d = r.mB;
		mScale = (d - c) / (b - a);
		mOffset = (a*d - b*c) / (a - b);
		mMinOutput = min(c, d);
		mMaxOutput = max(c, d);
	}
	
	float operator()(float f) const 
	{
		float r = f*mScale + mOffset;
		if(mClip) r = clamp(r, mMinOutput, mMaxOutput);
		return r;
	}
	
	inline float convert(float f) const
	{
		return f*mScale + mOffset;
	}
	
	inline float convertAndClip(float f) const
	{
		return clamp((f*mScale + mOffset), mMinOutput, mMaxOutput);
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

#endif // _ML_DSP_H

