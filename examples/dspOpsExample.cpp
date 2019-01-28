

#include <iostream>
#include <vector>
#include <chrono>
#include <thread>

#include "../include/madronalib.h"
#include "MLDSP.h"

#ifdef _WINDOWS
#include "Windows.h"
#endif

using namespace ml;
 
constexpr float mySinFillFn(int n){ return const_math::sin(n*kTwoPi/(kFloatsPerDSPVector));  }
	
int main()
{
#ifdef _WINDOWS
	AllocConsole();
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);
#endif

	std::cout << "DSP Ops:\n";
	
 	// columnIndex()
	DSPVector ci = columnIndex();
	std::cout << "index: " << ci << "\n\n";

	// generate a vector using map() and columnIndex()
	std::cout << "index squared: " << map(([](float x) {return x*x;}), ci) << "\n\n";

	auto sinMadronaLib = sin(rangeOpen(0, kTwoPi));
	std::cout << "madronalib sin: " << sinMadronaLib << "\n\n";
	 	
	// store a lambda on (DSPVector)->(DSPVector) defined using map(float)->(float)
	auto sinNative = [&](const DSPVector& x){ return map( [](float x){ return sinf(x*kTwoPi/(kFloatsPerDSPVector)); }, x); }(columnIndex());
	std::cout << "native sin: " << sinNative << "\n\n";	
	
	std::cout << "difference from native: " << sinNative - sinMadronaLib << "\n\n";
	
	// constexpr fill. unfortunately this cannot be made to work with a lambda in C++11.
	ConstDSPVector kSinVec(mySinFillFn);
	std::cout << "constexpr sin table: " << kSinVec << "\n\n";
	
	std::cout << "difference from native: " << sinNative - kSinVec << "\n\n";

	DSPVectorInt iv1(23);
	std::cout << "int fill: " << iv1 << "\n\n";

	DSPVectorInt iv2(truncateFloatToInt(columnIndex()));
	std::cout << "int index: " << iv2 << "\n\n";

	NoiseGen r;
	DSPVectorInt iv3(truncateFloatToInt(r()*DSPVector(64)));
	std::cout << "rand ints in [-64, 64]: " << iv3 << "\n\n";
	
	// for filters example / test
	FDN<4> f;
	// NOTE: the minimum possible delay time is kFloatsPerDSPVector.
	f.setDelayTimesInSamples({{67, 73, 91, 103}}); 
	f.setFilterCutoffs({{0.1f, 0.2, 0.3f, 0.4f}});
	f.mFeedbackGains = {{0.5f, 0.5f, 0.5f, 0.5f}};
	DSPVector silence, impulse;
	impulse[0] = 1.f;
	std::cout << "silence:" << silence << "\n";
	std::cout << "impulse:" << impulse << "\n";
	std::cout << f(impulse) << "\n";
	std::cout << f(silence) << "\n";
	std::cout << f(silence) << "\n";
	
	OnePole op;
	op.mCoeffs = OnePole::coeffs(0.15f);
	std::cout << op(impulse) << "\n";
	std::cout << op(silence) << "\n";
	std::cout << op(silence) << "\n";
	
	// clear filter 
	op = OnePole();
	// restore coeffs
	op.mCoeffs = OnePole::coeffs(0.15f);
	std::cout << op(silence) << "\n";
	std::cout << op(impulse) << "\n";
	
	/*
	
	// TEMP -> projections example
	Projection p1(projections::intervalMap({0, 10}, {10, 1000}));
	std::cout << "\n\n";
	for(int i=0; i<=10; ++i)
	{
		std::cout << i << " : " << p1(i) << "\n";
	}
	
	Projection p(projections::intervalMap({0, 10}, {10, 1000}, projections::log({10, 1000})));
	std::cout << "\n\n";
	for(int i=0; i<=10; ++i)
	{
		std::cout << i << " : " << p(i) << "\n";
	}
	
	Projection pp(projections::intervalMap({0, 10}, {10, 1000}, projections::flatcenter));
	std::cout << "\n\n";
	for(int i=0; i<=10; ++i)
	{
		std::cout << i << " : " << pp(i) << "\n";
	}
	
	Projection q(compose(projections::piecewiseLinear({1, 97, 2}), projections::intervalMap({0, 10}, {0, 1})));
	std::cout << "\n\n";
	for(int i=0; i<=10; ++i)
	{
		std::cout << i << " : " << q(i) << "\n";
	}
	
	*/
	
	TickGen ticks(20);
	
	Lopass lp1;
	lp1.mCoeffs = Lopass::coeffs(0.25, 1.0);
	
	DSPVector ticksLo = (ticks());
	
	
	HalfBandFilter f2;
	DSPVector ticks2 = f2.downsample(ticks(), ticks());
	
	std::cout << "\n\n";
	std::cout << ticksLo << "\n\n";
	Upsampler2x<1, 2> upper;
	std::cout << upper( [&](const DSPVector v){return repeat<2>(v);}, ticksLo) << "\n\n\n";

	Downsampler2x<1, 1> downer;
	
	DSPVector tick;
	tick[4] = 1.0f;
	auto identity = ([&](DSPVector v){return v;});
	std::cout << downer(identity, tick) << "\n" << downer(identity, DSPVector()) << "\n\n";

	
#ifdef _WINDOWS
	system("pause");
#endif

}

