
//
// dspFiltersTest
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
#include "../DSP/MLDSP.h"
#include "tests.h"

using namespace ml;

class scalarBiquadTest
{
public:
	DSPVector test(const DSPVector& vx)
	{
		DSPVector vy;
		for (int n=0; n<kFloatsPerDSPVector; ++n)
		{
			float in, out;
			in = vx[n];
			out = mA0[n]*in + mA1[n]*mX1 + mA2[n]*mX2 - mB1[n]*mY1 - mB2[n]*mY2;
			mX2 = mX1;
			mX1 = in;
			mY2 = mY1;
			mY1 = out;
			vy[n] = out;
		}
		return (vy);
	}

	void setCoeffs(const MLSignal& coeffs)
	{
		mA0 = coeffs[0];
		mA1 = coeffs[1];
		mA2 = coeffs[2];
		mB1 = coeffs[3];
		mB2 = coeffs[4];
	}
private:
	float mX1, mX2, mY1, mY2;
	DSPVector mA0, mA1, mA2, mB1, mB2;
};

TEST_CASE("madronalib/core/dsp_filters", "[dsp_filters]")
{
	DSPVector x1;
	x1[0] = 1;
	
	ml::Biquad vectorBiquad;
	scalarBiquadTest scalarBiquad;
	
	// these tests need to return results and do something with them, otherwise our
	// very smart compiler will just optimize the calls out
	auto callScalarBiquad = ([&](){return (scalarBiquad.test(x1));});
	auto callVectorBiquad = ([&](){return (vectorBiquad(x1));});
	
	// set coeffs
	MLSignal c = biquadCoeffs::hiShelf(0.1f, 2.0f, 0.5f);
	vectorBiquad.setCoeffs(c);
	scalarBiquad.setCoeffs(c);
	
	SECTION("time")
	{
		int iters = 10000;
		
		std::cout << "\nfilters: time test\n";
	
		timedResult<DSPVector> fnTimeScalar = timeIterations<DSPVector>(callScalarBiquad, iters);
		timedResult<DSPVector> fnTimeVector = timeIterations<DSPVector>(callVectorBiquad, iters);
		
		std::cout << "scalar: " << fnTimeScalar.elapsedTime << " seconds for " << iters << " iterations:\n";
		std::cout << "samples / sec: " << iters*kFloatsPerDSPVector/fnTimeScalar.elapsedTime << "\n";	

		std::cout << "vector: " << fnTimeVector.elapsedTime << " seconds for " << iters << " iterations:\n";
		std::cout << "samples / sec: " << iters*kFloatsPerDSPVector/fnTimeVector.elapsedTime << "\n";	

		// really not a good test because individual runs may differdue to CPU throttling and such
		REQUIRE(fnTimeVector.elapsedTime < fnTimeScalar.elapsedTime);
	}
}

