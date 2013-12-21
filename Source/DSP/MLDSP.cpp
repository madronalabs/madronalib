
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLDSP.h"
#include "MLDebug.h"

//	bit 31		bits 30-23		bits 22-0
//	sign		exponent		significand
//	0			011 1111 1		000 0000 0000 0000 0000 0000


// ----------------------------------------------------------------
// Utility functions
// ----------------------------------------------------------------

MLSample* alignToCacheLine(const MLSample* p)
{
	uintptr_t pM = (uintptr_t)p;
	pM += (uintptr_t)(kMLAlignSize - 1);
	pM &= kMLAlignMask;	
	return(MLSample*)pM;
} 

int bitsToContain(int n)
{
	int exp;
    for (exp = 0; (1 << exp) < n; exp++);
	return (exp);
}

unsigned ilog2(unsigned x) 
{
    register unsigned b=0;
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


static uint32_t gMLRandomSeed = 0;

// return single-precision floating point number on [-1, 1]
float MLRand()
{
	gMLRandomSeed = gMLRandomSeed * 0x0019660D + 0x3C6EF35F;
	uint32_t temp = (gMLRandomSeed >> 9) & 0x007FFFFF;
	temp &= 0x007FFFFF;// DSPConstants.r2;
	temp |= 0x3F800000; // DSPConstants.r1;
		
	float* pf = reinterpret_cast<float*>(&temp);
	*pf *= 2.f;
	*pf -= 3.f;
	
	return *pf;
}

void MLRandReset(void)
{
	gMLRandomSeed = 0;
}

const MLRange UnityRange = MLRange(0.f, 1.f);


float ampTodB(float a)
{
	return 20.f * log10f(a);
}	

float dBToAmp(float d)
{
	return powf(10.f, d/20.f);
}	

#pragma mark MLBiquad
float MLBiquad::processSample(float x)
{
    float in, out;
    in = x;
    out = a0*in + a1*x1 + a2*x2 - b1*y1 - b2*y2;
    x2 = x1;
    x1 = x;
    y2 = y1;
    y1 = out;
    return(out);
}

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
	float omega = kMLTwoPi * f * mOneOverSr;
	float cosOmega = cosf(omega);
	float alpha = sinf(omega) / (2.f * q);
	float b0 = 1.f + alpha;
	
	a0 = (1.f - cosOmega) * 0.5f / b0;
	a1 = (1.f - cosOmega) / b0;
	a2 = a0;
	b1 = -2.f * cosOmega / b0;
	b2 = (1.f - alpha) / b0;
}

void MLBiquad::setHipass(float f, float q)
{
	//HPF:        H(s) = s^2 / (s^2 + s/Q + 1)
	float omega = kMLTwoPi * f * mOneOverSr;
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
	float omega = kMLTwoPi * f * mOneOverSr;
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
//	float omega = kMLTwoPi * f * mOneOverSr;
//	float cosOmega = cosf(omega);
	float e = 2.718281828;
	float x = powf(e, -kMLTwoPi * f * mOneOverSr);
	a0 = 1.f - x;
	a1 = 0;
	a2 = 0;
	b1 = -x;
	b2 = 0;
}

void MLBiquad::setDifferentiate(void)
{
	a0 = 1.f;
	a1 = -1.f;
	a2 = 0;
	b1 = 0;
	b2 = 0;
}
