

#include <iostream>
#include <vector>
#include <chrono>
#include <thread>

#include "../include/madronalib.h"
#include "../source/DSP/MLDSP.h"
#include "../tests/tests.h"

using namespace ml;

int main()
{
	std::cout << "DSP Ops:\n";
	
	// generate a vector using map() and columnIndex()
	std::cout << "index squared: " << map(([](float x){return x*x;}), columnIndex()) << "\n\n";		

	auto sinMadronaLib = sin(rangeClosed(0, kPi));
	std::cout << "madronalib sin: " << sinMadronaLib << "\n\n";
	 	
	// store a lambda on (DSPVector)->(DSPVector) defined using map(float)->(float)
	auto sinNative = [&](const DSPVector& x){ return map( [](float x){ return sinf(x*kPi/(kFloatsPerDSPVector - 1)); }, x); }(columnIndex());
	std::cout << "native sin: " << sinNative << "\n\n";	
	
	std::cout << "difference: " << sinNative - sinMadronaLib << "\n\n";	
	
	/*
	 // store a lambda on ()->(DSPVector) defined using fill()->(float)
	auto randFill = [&](){ return map( [](){ return ml::rand(); }); };
	std::cout << randFill();
	
	DSPVector q = randFill();
	std::cout << "\n\n" << q << "\n";
	std::cout << "max: " << max(q) << "\n";
	
//	DSPVector q = sin(rangeOpen(-kMLPi, kMLPi));
//	std::cout << "\n\n" << q << "\n";
	std::cout << "min: " << min(q) << "\n";
	 */
	
//	DSPVectorArray<4> f;
	
//	auto f = repeat<4>(columnIndex());
//	f.setRowVector<1>(columnIndex()*2);
//	f.setRowVector(3, columnIndex()*2);
					  
//	f = map( [](float x){ return x + ml::rand()*0.01; }, f );
//	f = map( [](DSPVector v, int row){ return v*row; }, f );
// 	f = row( DSPVectorArray<4>() ) * repeat<4>(columnIndex()) ;
	
//	constexpr int iters = 100;
	
//	auto doFDNVector = [&](){ return map([](DSPVector v, int row){ return sin(rangeClosed(0 + row*kPi/2, kPi + row*kPi/2)); }, DSPVectorArray<ROWS>() ); } ;	
//	auto doFDNVector = [&](){ return rowIndex( DSPVectorArray<ROWS>() ); } ;
//	auto fdnTimeVector = timeIterations< DSPVectorArray<ROWS> >(doFDNVector, iters);
	
	auto rr = rowIndex<3>();
	auto qq = repeat<3>(rr);
	std::cout << qq << "\n";

	// ----------------------------------------------------------------
	// time FDN: scalars
	int iters = 100;
	float sr = 44100.f;

	FDN fdn({69, 70, 71, 72});
	
	MLSignal freqs({10000, 11000, 12000, 14000});
	freqs.scale(kTwoPi/sr);
	fdn.setFilterCutoffs(freqs);
//	fdn.setFilterCutoffs(MLSignal({10000, 11000, 12000, 14000})*kTwoPi/sr);
	
	fdn.setFeedbackGains({0.99, 0.99, 0.99, 0.99});
	
	DSPVector input(0);
	input[0] = 1;
	
	fdn(input);
	input = 0;
	auto doFDNVector = [&](){return fdn(input);};

	// note: fdn returns a DSPVectorArray<2>, so we need to pass this template parameter to timeIterations().
	auto fdnTimeVector = timeIterations< DSPVectorArray<2> >(doFDNVector, iters);
	std::cout << "VECTOR time: " << fdnTimeVector.elapsedTime << "\n";
	std::cout << fdnTimeVector.result << "\n";
}

