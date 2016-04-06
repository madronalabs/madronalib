//
//  MLVectorOps.cpp
//  madronalib
//
//  Created by Randy Jones on 4/5/2016
//
//

#include "MLVectorOps.h"

using namespace ml;

void DSPVector::setToConstant(float k)
{
	const __m128 vk = _mm_set1_ps(k); 	
	float* py1 = mData.asFloat;
	
	for (int n = 0; n < kDSPVectorSizeSSE; ++n)
	{
		_mm_store_ps(py1, vk);
		py1 += kSSEVecSize;
	}
}
