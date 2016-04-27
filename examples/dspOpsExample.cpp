

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

	DSPVector a = map( [](){ return ml::rand(); }, DSPVector() );
	DSPVector b = 3;
	DSPVector c = divide(a, b);
	std::cout << c;
	
	DSPVectorArray<3> aa(3);
	DSPVectorArray<3> bb = aa; //index<2>();
	
	DSPVectorArray<4> cc = 0;
	for(int i=0; i<4; ++i)
	{
		cc.setVector<2>(index()*i);
	}
	std::cout << cc;
	
	DSPVectorArray<4> f;
	f.fill(index());
	f = map( [](float x){ return x + ml::rand()*0.01; }, f );
	std::cout << f;
	
	
	const float sr = 44100.f;
	
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

	// note: fdn returns a DSPVectorArray<2>!
	auto fdnTimeVector = timeIterations<DSPVectorArray<2>>(doFDNVector, iters);
	std::cout << "VECTOR time: " << fdnTimeVector.elapsedTime << "\n";
	std::cout << fdnTimeVector.result << "\n";
}

