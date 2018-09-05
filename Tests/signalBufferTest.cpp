//
//  signalBufferTest.cpp
//  madronalib
//

#include <chrono>
using namespace std::chrono;

#include <thread>

#include "catch.hpp"
#include "../include/madronalib.h"

#include "MLSignalBuffer.h"
#include "MLProjection.h"

using namespace ml;

namespace signalBufferTest
{
	
	TEST_CASE("madronalib/core/signalbuffer", "[signalbuffer]")
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
	}
	
	const int kTestBufferSize = 256;
	const int kTestWrites = 200;
	
	float transmitSum = 0;
	float receiveSum = 0;
	
	// buffer shared between threads
	SignalBuffer testBuf;
	
	DSPVector inputVec, outputVec;
	
	size_t samplesTransmitted = 0;
	size_t maxSamplesInBuffer = 0;
	size_t samplesReceived = 0;
	float kEndFlag = 99;

	void transmitTest()
	{
		testBuf.resize(kTestBufferSize);
		RandomScalarSource rr;
		
		float data[10];
		IntervalProjection randToLength{ {-1, 1}, {3, 6} };
		
		for(int i=0; i<kTestWrites; ++i)
		{
			size_t writeLen = randToLength(rr.getFloat());
			
			for(int j=0; j<writeLen; ++j)
			{
				float f = rr.getFloat();
				data[j] = f;
				transmitSum += f;
			}
			
			testBuf.write(data, writeLen);
			samplesTransmitted += writeLen;

			std::cout << "+";
			std::this_thread::sleep_for(milliseconds(5));
		}

		data[0] = kEndFlag;
		testBuf.write(data, 1);
	}
	
	void receiveTest()
	{
		bool done = false;
		
		while(!done)
		{
			if(size_t k = testBuf.getReadAvailable())
			{
				if(k > maxSamplesInBuffer)
				{
					maxSamplesInBuffer = k;
				}
				float f;
				testBuf.read(&f, 1);
				if(f == kEndFlag)
				{
					done = true;
				}
				else
				{
					samplesReceived++;
					receiveSum += f;
				}
			}
			
			std::cout << "-";
			std::this_thread::sleep_for(milliseconds(1));
		}
	}
	
	TEST_CASE("madronalib/core/signalbuffer/threads", "[signalbuffer][threads]")
	{
		// start writing to queue and let writer get ahead a bit
		std::thread transmit(transmitTest);
		std::this_thread::sleep_for(milliseconds(25));
		
		// start reading
		std::thread receive(receiveTest);
		
		// wait for threads to finish
		transmit.join();
		receive.join();
		
		std::cout << "\ntransmit sum: " << transmitSum << "\nreceive sum: " << receiveSum << "\n";
		std::cout << "total samples transmitted: " << samplesTransmitted << "\n";
		std::cout << "total samples received: " << samplesReceived << "\n";
		std::cout << "buffer size: " << kTestBufferSize << "\n";
		std::cout << "max samples in buffer: " << maxSamplesInBuffer << "\n";

		REQUIRE(testBuf.getReadAvailable() == 0);
		REQUIRE(transmitSum == receiveSum);
	}
}
