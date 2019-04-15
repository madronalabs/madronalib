
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

#include "madronalib.h"
#include "mldsp.h"

#include "catch.hpp"
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
	
	std::vector< std::pair<std::string, std::vector<std::function<DSPVector(void)> > > > functionVectors
	{ {"sin", sinFunctions}, {"cos", cosFunctions}, {"log", logFunctions}, {"exp", expFunctions} };
	
	SECTION("precision")
	{
		// test precision of sin, cos, log, exp and approximations
		// use native math as reference		
		std::cout << "max differences from reference:\n";

		for(auto fnVec : functionVectors)
		{						
			DSPVector native = fnVec.second[0]();
			DSPVector precise = fnVec.second[1]();
			DSPVector approx = fnVec.second[2]();
			
			float nativeMaxDiff = max(abs(native - native));
			float preciseMaxDiff = max(abs(native - precise));
			float approxMaxDiff = max(abs(native - approx));
			
			std::cout << fnVec.first << " native: " << nativeMaxDiff << ", precis: " << preciseMaxDiff << ", approx: " << approxMaxDiff << " \n";	
			
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
		int iters = 1000;
		
		std::cout << "nanoseconds for " << iters << " iterations:\n";
		int i = 0;
		for(auto fnVec : functionVectors)
		{			
			timedResult<DSPVector> fnTimeNative = timeIterations<DSPVector>(fnVec.second[0], iters);
			timedResult<DSPVector> fnTimePrecise = timeIterations<DSPVector>(fnVec.second[1], iters);
			timedResult<DSPVector> fnTimeApprox = timeIterations<DSPVector>(fnVec.second[2], iters);
			std::cout << fnVec.first << " native: " << fnTimeNative.ns << ", precise: " << fnTimePrecise.ns << ", approx: " << fnTimeApprox.ns << " \n";
			
#ifdef NDEBUG
			REQUIRE(fnTimeApprox.ns <= fnTimePrecise.ns);
			REQUIRE(fnTimePrecise.ns <= fnTimeNative.ns);
#endif
			i++;
		}		
	}
}
