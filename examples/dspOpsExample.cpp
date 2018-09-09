

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

	RandomSource r;
	DSPVectorInt iv3(truncateFloatToInt(r()*DSPVector(64.f)));
	std::cout << "rand ints: " << iv3 << "\n\n";

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
	
	/*
	auto rr = rowIndex<3>();
	auto qq = repeat<3>(rr);
	std::cout << qq << "\n";
*/
	
	/*
	// ----------------------------------------------------------------
	// time FDN: scalars
	int iters = 10;
	float sr = 44100.f;
	
	MLSignal freqs({10000, 11000, 12000, 14000});
	freqs.scale(kTwoPi/sr);

	static FDN fdn
	{ 
		{"delays", {69, 70, 71, 72} },
		{"cutoffs", freqs } , // TODO more functional rewrite of MLSignal so we can create freqs inline
		{"gains", {0.99, 0.99, 0.99, 0.99} }
	};

	DSPVector input(0);
	input[0] = 1;
	
	fdn(input);
	input = 0;
	auto doFDNVector = [&](){return fdn(input);};

	// note: fdn returns a DSPVectorArray<2>, so we need to pass this template parameter to timeIterations().
	auto fdnTimeVector = timeIterations< DSPVectorArray<2> >(doFDNVector, iters);
	std::cout << "VECTOR time: " << fdnTimeVector.elapsedTime << "\n";
	std::cout << fdnTimeVector.result << "\n";
	 */
	
	

	DSPVector m(3.f);
	DSPVector n(r());
	
	DSPVector a = m + n;
	
	std::cout << "sum: " << a << "\n";
	
#ifdef _WINDOWS
	system("pause");
#endif

}

