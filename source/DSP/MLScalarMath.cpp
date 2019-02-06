//  MLScalarMath
//  madronalib
//
//  Created by Randy Jones on 4/15/16

#include "MLScalarMath.h"

namespace ml  
{
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

	int isNaN(float x) 
	{ 
	#ifdef _WIN32
		return (x != x);
	#else
		return isnan(x);
	#endif
	}

	int isNaN(double x) 
	{ 
	#ifdef _WIN32
		return (x != x);
	#else
		return isnan(x);
	#endif
	}

	int isInfinite(float x) 
	{ 
	#ifdef _WIN32
		return ((x > FLT_MAX) || (x < -FLT_MAX));
	#else
		return isinf(x);
	#endif
	}

	int isInfinite(double x) 
	{ 
	#ifdef _WIN32
		return ((x > DBL_MAX) || (x < -DBL_MAX));
	#else
		return isinf(x);
	#endif
	}
    
}
