

#include <iostream>
#include <vector>
#include <chrono>
#include <thread>

#include "mldsp.h"

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
	f.setDelaysInSamples({{67, 73, 91, 103}}); 
	f.setFilterCutoffs({{0.1f, 0.2f, 0.3f, 0.4f}});
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
	
	
	TickGen ticks(16);
	
	TestSineGen sineGen;
	DSPVector sinewave = sineGen(DSPVector(110.f/44100.f));
	
	TestSineGen sineModGen;
	DSPVector sineMod = sineModGen(DSPVector(1.f/44100.f));
	
	Lopass lp1;
	lp1.mCoeffs = Lopass::coeffs(0.25, 1.0);
	
	
	DSPVector tick;
	tick[0] = 1;
	

	// upsampler for a generator with 1 input row
	Upsample2xFunction<1> upper;
	std::cout << "\n\n" << upper([&](const DSPVector x) { return sineGen(x); }, DSPVector(440.f / 44100.f)) << "\n\n";


	// IntegerDelay p(100);
	//std::cout << "\n\n" << p(DSPVector(), DSPVector()) << "\n" << p(DSPVector(), DSPVector()) << "\n\n";
	
//	DSPVector y = lp1(tick);
	auto lpTestFn = [&](const DSPVector x){ return (x); };
	

	FeedbackDelayFunction feedbackFn;
	
	// set the delay time. a time < kFloatsPerDSPVector will not work.
	DSPVector vDelayTime2(65.f);
	std::cout << feedbackFn(tick, lpTestFn, vDelayTime2 ) << "\n";
	for(int i=0; i<4; ++i)
	{
		std::cout << feedbackFn(DSPVector(), lpTestFn, vDelayTime2 ) << "\n";
	}
	
	std::cout << "\n\n\n\n";
	
	DSPVector tick2;
	tick2[20] = 1; // after PitchbendableDelay warmup

	PitchbendableDelay pd1;	
	DSPVector vDelayTime3(4.f);
	std::cout << pd1(tick2, vDelayTime3) << "\n";
	for(int i=0; i<4; ++i)
	{
		std::cout << pd1(DSPVector(), vDelayTime3) << "\n";
	}
	std::cout << pd1(tick2, vDelayTime3) << "\n";
	std::cout << pd1(tick2, 4.f) << "\n";
	
	std::cout << "\n\n\n";
	
	auto vHiCoeffs = HiShelf::vcoeffs({{0.25f, 1.f, 1.f}}, {{0.3f, 1.f, 2.f}}); // why two sets of braces? see aaltoverb
	std::cout << vHiCoeffs;

#ifdef _WINDOWS
	system("pause");
#endif

}

