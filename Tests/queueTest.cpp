//
//  queueTest.cpp
//  madronalib
//
//  Created by Randy Jones on 11/8/2016
//
//

#include <chrono>
using namespace std::chrono;

#include <thread>

#include "catch.hpp"
#include "madronalib.h"


struct TestEvent
{
	int mTime;
	float mValue1;
};

using namespace ml;
const int kTestBufferSize = 100;


namespace queueTest{
	
	TEST_CASE("madronalib/core/queue", "[queue]")
	{
		std::cout << "\n\nQUEUE\n";
		// setup event queue
		Queue<TestEvent> eventQueue(kTestBufferSize);
		
		// write and read some events to get near the end of the physical buffer
		int padding = kTestBufferSize - 10;
		for(int i=0; i<padding; ++i)
		{
			eventQueue.push(TestEvent{i, i + 23.0f});
		}
		
		// show usage for peek
		while(eventQueue.elementsAvailable() && (eventQueue.peek().mValue1 < 46))
		{
			TestEvent g;
			eventQueue.pop(g);
			std::cout << g.mValue1 << " ";
		}
		std::cout << "\n";
		
		TestEvent f;
		while(eventQueue.pop(f))
		{
			std::cout << f.mValue1 << " ";
		}
		
		std::cout << "\n";
	}
	
	const int kTestSize = 200;
	const int kTestCount = 500;
	int transmitSum = 0;
	int receiveSum = 0;
	
	Queue<TestEvent> testQueue(kTestSize);
	
	void transmitTest()
	{
		RandomScalarSource randGen;
		for(int i=0; i<kTestCount; ++i)
		{
			int r = randGen.getUInt32();
			transmitSum += r;
			testQueue.push(TestEvent{r, 1.f});
			
			std::cout << "+";
			std::this_thread::sleep_for(milliseconds(1));
		}
	}
	
	void receiveTest()
	{
		size_t maxQueueSize = 0;
		TestEvent ev;
		while(testQueue.pop(ev))
		{
			size_t k = testQueue.elementsAvailable();
			if(k > maxQueueSize)
			{
				maxQueueSize = k;
			}
			receiveSum += ev.mTime;
			
			std::cout << "-";
			std::this_thread::sleep_for(milliseconds(1));
		}
		std::cout << "\n max queue size: " << maxQueueSize << "\n";
	}
	
	TEST_CASE("madronalib/core/queue/threads", "[queue][threads]")
	{
		// start writing to queue and let writer get ahead a bit
		std::thread transmit(transmitTest);
		std::this_thread::sleep_for(milliseconds(10));
		
		// start reading
		std::thread receive(receiveTest);
		
		transmit.join();
		receive.join();
		
		std::cout << "\ntransmit sum: " << transmitSum << ", receive sum: " << receiveSum << "\n";
		
		REQUIRE(testQueue.wasEmpty());
		REQUIRE(transmitSum == receiveSum);
	}
}
