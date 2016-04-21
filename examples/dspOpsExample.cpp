

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
	DSPVector b = a + 3;
	std::cout << b;
	
	float sr = 44100.f;
	
	TickSource ticks(10);
	Biquad lopass(biquadCoeffs::onePole(10000./sr));
	FixedDelay delay(99);
	
	DSPVector t1 = (ticks());
	DSPVector t2 = delay(t1)*0.5f;
	DSPVector t3 = t1 + t2;
	
	std::cout << t3 << "\n";	
	
	t1 = (ticks());
	t2 = delay(t1)*0.5f;
	t3 = t1 + t2;

	std::cout << t3 << "\n";	
	
	// ----------------------------------------------------------------
	// time FDN: scalars
	int iters = 100;
	FDN fdn(4, 1000);
	MLSignal delayTimes({69, 70, 71, 72});
	fdn.setDelaysInSamples(delayTimes);	

	DSPVector input(0);
	input[0] = 1;
	
	DSPVector y = fdn(input);
	input = 0;
	
	std::cout << "input: " << input << "\n";
	
	auto doFDNVector = [&](){return fdn(input);};
	timedResult<DSPVector> fdnTimeVector = timeIterations<DSPVector>(doFDNVector, iters);
	std::cout << "VECTOR time: " << fdnTimeVector.elapsedTime << "\n";
	std::cout << fdnTimeVector.result << "\n";
}

