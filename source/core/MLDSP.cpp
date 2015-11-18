
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLDSP.h"

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

