//
//  signalTest.cpp
//  madronalib
//
//  Created by Randy Jones on 7/30/15.
//
//

#include "catch.hpp"
#include "../include/madronalib.h"

TEST_CASE("madronalib/core/signal/creation", "[signal][creation]")
{
	// signal comparisons are by value
	MLSignal a;
	MLSignal b;
	REQUIRE(a == b);
	
	// 
	const int testSize(16);
	a.setDims(testSize);
	b.setDims(testSize);
	REQUIRE(a == b);

	for(int i=0; i<testSize; ++i)
	{
		a[i] = 0.5f;
	}
	
	for(int i=0; i<testSize; ++i)
	{
		b[i] = a[i]*2.f;
	}
	
	float sum = 0;
	for(int i=0; i<testSize; ++i)
	{
		sum += b[i];
	}
	
	std::cout << "MLSignal sum: " << sum  << "\n";
}

