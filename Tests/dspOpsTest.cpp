
//
// dspOpsTest
// a unit test made using the Catch framework in catch.hpp / tests.cpp.
//

#include <iostream>
#include <vector>
#include <map>
#include <unordered_map>
#include <chrono>
#include <thread>

#include "catch.hpp"
#include "../include/madronalib.h"
#include "../DSP/MLDSPOps.h"
#include "tests.h"

using namespace ml;

TEST_CASE("madronalib/core/dsp_ops", "[dsp_ops]")
{
	DSPVector a(rangeClosed(-kPi, kPi));
	
	auto sinN = ([&](){DSPVector v; for(int i=0; i<kFloatsPerDSPVector; ++i){ v[i] = sinf(a[i]); } return v; } );
	auto sinP = ([&](){return sin(a);});
	auto sinA = ([&](){return sinApprox(a);});
	std::vector<std::function<DSPVector(void)> > sinFunctions {sinN, sinP, sinA};
	
	auto cosN = ([&](){DSPVector v; for(int i=0; i<kFloatsPerDSPVector; ++i){ v[i] = cosf(a[i]); } return v; } );
	auto cosP = ([&](){return cos(a);});
	auto cosA = ([&](){return cosApprox(a);});
	std::vector<std::function<DSPVector(void)> > cosFunctions {cosN, cosP, cosA};
	
	auto logN = ([&](){DSPVector v; for(int i=0; i<kFloatsPerDSPVector; ++i){ v[i] = logf(a[i]); } return v; } );
	auto logP = ([&](){return log(a);});
	auto logA = ([&](){return logApprox(a);});
	std::vector<std::function<DSPVector(void)> > logFunctions {logN, logP, logA};
	
	auto expN = ([&](){DSPVector v; for(int i=0; i<kFloatsPerDSPVector; ++i){ v[i] = expf(a[i]); } return v; } );
	auto expP = ([&](){return exp(a);});
	auto expA = ([&](){return expApprox(a);});
	std::vector<std::function<DSPVector(void)> > expFunctions {expN, expP, expA};
	
	std::vector<std::vector<std::function<DSPVector(void)> > > functionVectors{sinFunctions, cosFunctions, logFunctions, expFunctions};

	SECTION("precision")
	{
		// test precision of sin, cos, log, exp and approximations
		// use native math as reference		
		std::cout << "max differences from reference:\n";
		for(auto fnVec : functionVectors)
		{						
			DSPVector native = fnVec[0]();
			DSPVector precise = fnVec[1]();
			DSPVector approx = fnVec[2]();
			
			float nativeMaxDiff = max(abs(native - native));
			float preciseMaxDiff = max(abs(native - precise));
			float approxMaxDiff = max(abs(native - approx));
			
			std::cout << "native: " << nativeMaxDiff << ", precis: " << preciseMaxDiff << ", approx: " << approxMaxDiff << " \n";	
			
			// these differences are to accommodate the exp functions, the other ones are a lot more precise.
			REQUIRE(preciseMaxDiff < 2e-6f);
			REQUIRE(approxMaxDiff < 2e-4f);
		}
	}

	SECTION("time")
	{
		// test speed of precise functions relative to native ones.
		// test speed of approximate functions relative to precise ones.
		// approximate ones should be faster!
		int iters = 100000;
		
		std::cout << "seconds for " << iters << " iterations:\n";
		for(auto fnVec : functionVectors)
		{			
			timedResult<DSPVector> fnTimeNative = timeIterations<DSPVector>(fnVec[0], iters);
			timedResult<DSPVector> fnTimePrecise = timeIterations<DSPVector>(fnVec[1], iters);
			timedResult<DSPVector> fnTimeApprox = timeIterations<DSPVector>(fnVec[2], iters);
			std::cout << "native: " << fnTimeNative.elapsedTime << ", precise: " << fnTimePrecise.elapsedTime << ", approx: " << fnTimeApprox.elapsedTime << " \n";			
			REQUIRE(fnTimeApprox.elapsedTime < fnTimePrecise.elapsedTime);
			REQUIRE(fnTimePrecise.elapsedTime < fnTimeNative.elapsedTime);
		}		
	}
}
