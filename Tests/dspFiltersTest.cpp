
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
#include "MLDSP.h"
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

class ScalarSVFLopassTest
{
public:
	ScalarSVFLopassTest() { setCoeffs(ml::svfCoeffs::silent()); clear(); }
	void clear() { ic1eq = ic2eq = 0.f; }
	void setCoeffs(const MLSignal& coeffs) { g0 = coeffs[0]; g1 = coeffs[1]; g2 = coeffs[2]; }
	
	inline float operator()(const float v0)
	{
		float t0 = v0 - ic2eq;
		float t1 = g0*t0 + g1*ic1eq;
		float t2 = g2*t0 + g0*ic1eq;
		float v2 = t2 + ic2eq;
		ic1eq += 2.0f*t1;
		ic2eq += 2.0f*t2;
		return v2;
	}
	
	float g0, g1, g2;
	float ic1eq, ic2eq;
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
	
	// set biquad coeffs
	MLSignal c = biquadCoeffs::hiShelf(0.1f, 2.0f, 0.5f);
	vectorBiquad.setCoeffs(c);
	scalarBiquad.setCoeffs(c);
	
	// tests for 2 SVFs in series
	ml::SVFLopass vectorSVF1;
	ml::SVFLopass vectorSVF2;
	ScalarSVFLopassTest scalarSVF1;
	ScalarSVFLopassTest scalarSVF2;

	auto callScalarSVF = ([&]()
	{
		DSPVector r;
		for (int n=0; n<kFloatsPerDSPVector; ++n)
		{
			r[n] = scalarSVF1(scalarSVF2(x1[n]));
		}
		return r;
	}
												);
	auto callVectorSVF = ([&](){return (vectorSVF1(vectorSVF2(x1)));});

	// set SVF coeffs
	MLSignal c2 = ml::svfCoeffs::atOmegaAndK(0.05f, 1.0f);
	vectorSVF1.setCoeffs(c2);
	vectorSVF2.setCoeffs(c2);
	scalarSVF1.setCoeffs(c2);
	scalarSVF2.setCoeffs(c2);

	SECTION("time biquad")
	{
		int iters = 1000;
		
		std::cout << "\nfilters: time test\n";
	
		timedResult<DSPVector> fnTimeScalar = timeIterations<DSPVector>(callScalarBiquad, iters);
		timedResult<DSPVector> fnTimeVector = timeIterations<DSPVector>(callVectorBiquad, iters);
		
		std::cout << "scalar biquad: " << fnTimeScalar.ns << " nanoseconds for " << iters << " iterations:\n";
		std::cout << "samples / ns: " << iters*kFloatsPerDSPVector/fnTimeScalar.ns << "\n";
		
		std::cout << "vector biquad: " << fnTimeVector.ns << " nanoseconds for " << iters << " iterations:\n";
		std::cout << "samples / ns: " << iters*kFloatsPerDSPVector/fnTimeVector.ns << "\n";
		
#ifdef NDEBUG
		REQUIRE(fnTimeVector.ns <= fnTimeScalar.ns);
#endif
	}
		
	SECTION("time SVF")
	{
		int iters = 1000;
		
		timedResult<DSPVector> fnTimeScalarSVF = timeIterations<DSPVector>(callScalarSVF, iters);
		timedResult<DSPVector> fnTimeVectorSVF = timeIterations<DSPVector>(callVectorSVF, iters);
		
		std::cout << "scalar SVF: " << fnTimeScalarSVF.ns << " nanoseconds for " << iters << " iterations:\n";
		std::cout << "samples / ns: " << iters*kFloatsPerDSPVector/fnTimeScalarSVF.ns << "\n";
		
		std::cout << "vector SVF: " << fnTimeVectorSVF.ns << " nanoseconds for " << iters << " iterations:\n";
		std::cout << "samples / ns: " << iters*kFloatsPerDSPVector/fnTimeVectorSVF.ns << "\n";
		
#ifdef NDEBUG
		REQUIRE(fnTimeVectorSVF.ns <= fnTimeScalarSVF.ns);
#endif
	}
}

