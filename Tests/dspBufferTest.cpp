//
//  dspBufferTest.cpp
//  madronalib
//

#include <chrono>
using namespace std::chrono;

#include <thread>

#include "catch.hpp"
#include "mldsp.h"

#include "MLDSPBuffer.h"
#include "MLProjection.h"

using namespace ml;

namespace dspBufferTest
{
	TEST_CASE("madronalib/core/dspbuffer", "[dspbuffer]")
	{
		std::cout << "\nDSPBUFFER\n";
		
		// buffer should be next larger power-of-two size
		DSPBuffer buf;
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
	DSPBuffer testBuf;
	
	size_t samplesTransmitted = 0;
	size_t maxSamplesInBuffer = 0;
	size_t samplesReceived = 0;
	float kEndFlag = 99;

	RandomScalarSource rr;
	const int kMaxReadWriteSize = 16;
	
	IntervalProjection randToLength{ {-1, 1}, {1, kMaxReadWriteSize} };

	void transmitTest()
	{
		testBuf.resize(kTestBufferSize);
		float data[kMaxReadWriteSize];
		
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
			std::cout << "+"; // show write
			samplesTransmitted += writeLen;

			std::this_thread::sleep_for(milliseconds(2));
		}

		data[0] = kEndFlag;
		testBuf.write(data, 1);
	}
	
	void receiveTest()
	{
		bool done = false;
		float data[kMaxReadWriteSize];

		while(!done)
		{
			if(size_t k = testBuf.getReadAvailable())
			{
				if(k > maxSamplesInBuffer)
				{
					maxSamplesInBuffer = k;
				}

				size_t readLen = randToLength(rr.getFloat());
				readLen = std::min(readLen, k);
				testBuf.read(data, std::min(readLen, k));
				std::cout << "-"; // show read

				for(int i=0; i<readLen; ++i)
				{
					float f = data[i];
					if(f == kEndFlag)
					{
						done = true;
						break;
					}
					else
					{
						samplesReceived++;
						receiveSum += f;
					}
				}
			}
			else
			{
				std::cout << "."; // show wait
			}
			std::this_thread::sleep_for(milliseconds(1));
		}
	}
	
	TEST_CASE("madronalib/core/dspbuffer/threads", "[dspbuffer][threads]")
	{
		// start threads
		std::thread transmit(transmitTest);
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
		REQUIRE(samplesTransmitted == samplesReceived);
	}
	
	TEST_CASE("madronalib/core/dspbuffer/overlap", "[dspbuffer][overlap]")
	{
		DSPBuffer buf;
		buf.resize(256);
		
		DSPVector windowVec, outputVec, outputVec2;
		int overlap = kFloatsPerDSPVector/2;
		
		// write overlapping triangle windows
		makeWindow(windowVec.getBuffer(), kFloatsPerDSPVector, windows::triangle);
		for(int i=0; i<8; ++i)
		{
			buf.writeWithOverlapAdd(windowVec.getBuffer(), kFloatsPerDSPVector, overlap);
		}
		
		// read past startup
		buf.read(outputVec);
		
		// after startup, sums of windows should be constant
		buf.read(outputVec);
		buf.read(outputVec2);
		
		REQUIRE(outputVec == outputVec2);
	}
	
	TEST_CASE("madronalib/core/dspbuffer/vectors", "[dspbuffer][vectors]")
	{
		DSPBuffer buf;
		buf.resize(256);
		
		constexpr int kRows = 3;
		DSPVectorArray<kRows> inputVec, outputVec;

		// make a DSPVectorArray with a unique int at each sample
		inputVec = map( [](DSPVector v, int row){ return v + DSPVector(kFloatsPerDSPVector*row); }, repeat<kRows>(columnIndex()) );
		
		// write long enough that we will wrap
		for(int i=0; i<4; ++i)
		{
			buf.write(inputVec);
			buf.read(outputVec);
		}
		
		REQUIRE(inputVec == outputVec);
	}
}
