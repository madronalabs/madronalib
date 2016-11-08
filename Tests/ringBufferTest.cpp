//
//  ringBufferTest.cpp
//  madronalib
//
//  Created by Randy Jones on 11/8/2016
//
//

#include "catch.hpp"
#include "../include/madronalib.h"
#include "pa_ringbuffer.h" // TODO make templated version

class TestEvent
{
public:	
	TestEvent();
	TestEvent(int time, float value);

	float mValue1;
	int mTime;
};

const int kMaxControlEventsPerBlock = 1024;

TestEvent::TestEvent() :
mValue1(0.),
mTime(0)
{
}

TestEvent::TestEvent(int time, float value) :
mValue1(value),
mTime(time)
{
}

TEST_CASE("madronalib/core/ringbuffer", "[ringbuffer]")
{
	// TODO lock-free queue template
	std::vector<TestEvent> eventData;	
	PaUtilRingBuffer eventQueue;
	
	// setup event queue
	eventData.resize(kMaxControlEventsPerBlock);
	PaUtil_InitializeRingBuffer( &eventQueue, sizeof(TestEvent), kMaxControlEventsPerBlock, &(eventData[0]) );

	int remaining = PaUtil_GetRingBufferReadAvailable(&eventQueue);
	
	// write some events
	int elems = 20;
	for(int i=0; i<elems; ++i)
	{
		int time = i;
		float v1 = (time > 8) ? (36.2 - i + 0.5f) : 99.;
		TestEvent e(time, v1);
		PaUtil_WriteRingBuffer( &eventQueue, &e, 1 );
	}
	
	AvailableElementsVector<TestEvent> v(&eventQueue);
	
	std::cout << "size: " << v.size() << "\n";
	
	AvailableElementsVector<TestEvent>::iterator i1 = v.begin();
	AvailableElementsVector<TestEvent>::iterator i2 = i1;
	i1 += 1;
	i2 += 5;

	std::cout << "---------------------------\n\n";
				
	std::cout << (*i1).mTime << " : " << (*i1).mValue1 << "\n";

	std::cout << "---------------------------\n\n";

//	TestEvent* pi2 = static_cast<TestEvent*>(*i2);		
//	std::cout << i2->mTime << " : " << i2->mValue1 << "\n";

	std::cout << "---------------------------\n\n";

	std::iter_swap(i1, i2);
	
	std::sort(v.begin(), v.end(), [](TestEvent a, TestEvent b){ return a.mValue1 < b.mValue1; });
 
	for(AvailableElementsVector<TestEvent>::iterator it = v.begin(); it != v.end(); it++)
	{
		// TEMP
		TestEvent& pE = (*it);		
		std::cout << pE.mTime << " : " << pE.mValue1 << "\n";
	}
	
	
	std::cout << "---------------------------\n\n";
	
	
	
	
	std::cout << "OKX\n";
}

