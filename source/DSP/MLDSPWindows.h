//
//  MLDSPWindows.h
//  madronalib
//
//  Created by Randy Jones on 5/16/16.
//
//

#ifndef MLDSPWindows_h
#define MLDSPWindows_h

#include "MLSignal.h"

namespace ml
{
	namespace windows
	{
		inline float unityRange(int size, int i) 
		{ 
			float hw = 2.f/(size - 1); 
			return i*hw - 1; 
		}
		
		inline MLSignal rectangle(int size, int divisions = 1)
		{
			auto f = [&](int i){ return fabs(unityRange(size, i)*divisions) < 0.5 ? 1 : 0 ; };
			return MLSignal (size, f);
		}
		
		inline MLSignal triangle(int size, int divisions = 1)
		{
			auto f = [&](int i){ float x = fabs(unityRange(size, i)*divisions); return x < 1 ? 1 - x : 0 ; };
			return MLSignal (size, f);
		}
		
		inline MLSignal raisedCosine(int size, int divisions = 1)
		{
			auto f = [&](int i){ float x = fabs(unityRange(size, i)*divisions); return 0.5f*(cosf(x*kPi) + 1.0f); };
			return MLSignal (size, f);		
		}
	}
}


#endif /* MLDSPWindows_h */
