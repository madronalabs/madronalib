//  MLScalarMath
//  madronalib
//
//  Created by Randy Jones on 4/15/16
//
// templates for common scalar math functions on int, float, double.
// other small scalar utilities. 

#pragma once
#include <math.h>
#include <stdint.h>

namespace ml 
{
	// ----------------------------------------------------------------
	#pragma mark scalar-type templates
	
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
	
	template <class c>
	inline c lerp(const c& a, const c& b, const c& m)
	{
		return(a + m*(b-a));
	}
	
	template <class c>
	inline bool (within)(const c& x, const c& min, const c& max)
	{
		return ((x >= min) && (x < max));
	}
	
	// ----------------------------------------------------------------
	#pragma mark utility functions on scalars

	int ilog2(int x);
	int isNaN(float x) ;
	int isNaN(double x) ;
	int isInfinite(float x); 
	int isInfinite(double x) ;

	inline float smoothstep(float a, float b, float x)
	{
		x = clamp((x - a)/(b - a), 0.f, 1.f); 
		return x*x*(3 - 2*x);
	}
	
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
	
	int ilog2(int n);
	
	inline float lerpBipolar(const float a, const float b, const float c, const float m)
	{
		float absm = fabsf(m);	// TODO fast abs etc
		float pos = m > 0.;
		float neg = m < 0.;
		float q = pos*c + neg*a;
		return (b + (q - b)*absm);
	}		
	
	inline float herp(const float* t, float phase)
	{
		// 4-point, 3rd-order Hermite interpolation
		const float c = (t[2] - t[0]) * 0.5f;
		const float v = t[1] - t[2];
		const float w = c + v;
		const float a = w + v + (t[3] - t[1]) * 0.5f;
		const float b = w + a;
		return (((a * phase) - b) * phase + c) * phase + t[1];
	}
	
	// amp <-> dB conversions, where ratio of the given amplitude is to 1.	
	inline float ampTodB(float a)
	{
		return 20.f * log10f(a);
	}	
	
	inline float dBToAmp(float dB)
	{
		return powf(10.f, dB/20.f);
	}	
	
	// global random generator
	float rand(void);
	uint32_t rand32(void);
	void randReset(void);
	

} // namespace ml
