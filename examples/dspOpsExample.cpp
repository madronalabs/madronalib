

#include <iostream>
#include <vector>
#include <chrono>
#include <thread>

#include "../include/madronalib.h"
#include "../source/DSP/MLDSP.h"


using namespace ml;

int main()
{
	std::cout << "DSP Ops:\n";
	
	/*
	// generate a vector using map() and index()
	std::cout << map(([](float x){return x*x;}), index()) << "\n";	
	
	// store a lambda on (DSPVector)->(DSPVector) defined using a loop
	auto sinNative = ([&](DSPVector x){DSPVector v; for(int i=0; i<kFloatsPerDSPVector; ++i){ v[i] = sinf(x[i]); } return v; } );
	std::cout << sinNative(index());
	
	// store a lambda on (DSPVector)->(DSPVector) defined using map(float)->(float)
	auto sinNativeMap = [&](DSPVector x){ return map( [](float x){ return sinf(x); }, x); };
	std::cout << sinNativeMap(index());
	
	// store a lambda on ()->(DSPVector) defined using fill()->(float)
	auto randFill = [&](){ return fill( [](){ return ml::rand(); }); };
	std::cout << randFill();
	
	DSPVector q = randFill();
	std::cout << "\n\n" << q << "\n";
	std::cout << "max: " << max(q) << "\n";
	
//	DSPVector q = sin(rangeOpen(-kMLPi, kMLPi));
//	std::cout << "\n\n" << q << "\n";
	std::cout << "min: " << min(q) << "\n";
	 */
	
	DSPVector a = fill( [](){ return ml::rand(); } );
	DSPVector b = abs(a);
	std::cout << b;

	float sr = 44100.f;
	
	TickSource ticks(8);
	Biquad lopass(biquadCoeffs::onePole(10000./sr));
	SampleDelay delay(1);
	
	DSPVector t1 = (ticks());
	DSPVector t2 = lopass(t1);

	std::cout << t1 << "\n";
	std::cout << t2 << "\n";	

}