
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2018 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#include "MLProjection.h"

namespace ml
{
	inline void mapIndices(float* pDest, size_t size, Projection p)
	{
		for(int i=0; i<size; ++i)
		{
			pDest[i] = p(i);
		}
	}
	
	inline void makeWindow(float* pDest, size_t size, Projection windowShape)
	{
		IntervalProjection domainToUnity{ {0.f, size - 1.f}, {0.f, 1.f} };
		mapIndices(pDest, size, compose(windowShape, domainToUnity));
	}
	
	namespace windows
	{
		const Projection rectangle( [](float x){ return 1.f; } );
		const Projection triangle( [](float x){ return (x > 0.5f) ? (2.f - 2.f*x) : (2.f*x); } );
		const Projection raisedCosine( [](float x){ return 0.5f - 0.5f*cosf(kTwoPi*x); } );
		const Projection hamming( [](float x){ return 0.54f - 0.46f*cosf(kTwoPi*x); } );
		const Projection blackman( [](float x){ return 0.42f - 0.5f*cosf(kTwoPi*x) + 0.08f*cosf(2.f*kTwoPi*x); } );
		const Projection flatTop( [](float x){
			const float a0 = 0.21557895;
			const float a1 = 0.41663158;
			const float a2 = 0.277263158;
			const float a3 = 0.083578947;
			const float a4 = 0.006947368;
			return a0 - a1*cosf(kTwoPi*x) + a2*cosf(2.f*kTwoPi*x) - a3*cosf(3.f*kTwoPi*x) + a4*cosf(4.f*kTwoPi*x); } );
	}
}

