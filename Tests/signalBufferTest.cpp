//
//  signalBufferTest.cpp
//  madronalib
//


#include "catch.hpp"
#include "../include/madronalib.h"

#include "MLSignalBuffer.h"

using namespace ml;


namespace signalBufferTest
{
	
TEST_CASE("madronalib/core/DSP/signalbuffer", "[DSP][signalbuffer]")
{
	std::cout << "\nSIGNALBUFFER\n";
	
	// buffer should be next larger power-of-two size
	SignalBuffer buf;
	buf.resize(197);
	REQUIRE(buf.getWriteAvailable() == 256);
	
	// write to near end
	std::vector<float> nines;
	nines.resize(256);
	std::fill(nines.begin(), nines.end(), 9.f);
	buf.write(nines.data(), 250);
	buf.read(nines.data(), 250);

	// write indices with wrap
	DSPVector v1(columnIndex());
	buf.write(v1.getConstBuffer(), kFloatsPerDSPVector);
	DSPVector v2{};
	buf.read(v2.getBuffer(), kFloatsPerDSPVector);
	std::cout << v2 << "\n";
	REQUIRE(buf.getReadAvailable() == 0);
	REQUIRE(v2 == v1);
	
	
	
	
	
	// TODO DSPBuffer syntax
}

}
