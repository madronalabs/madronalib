//
//  ringBufferTest.cpp
//  madronalib
//
//  Created by Randy Jones on 11/8/2016
//
//

#include "catch.hpp"
#include "../include/madronalib.h"
#include "pa_ringbuffer.h" // TODO make templated version of ringbuffer

class TestEvent
{
public:	
	TestEvent();
	TestEvent(int time, float value);
	float mValue1;
	int mTime;
};

const int kMaxControlEventsPerBlock = 64; // must be power of two!

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

	// write and read some events
	int padding = kMaxControlEventsPerBlock - 10;
	for(int i=0; i<padding; ++i)
	{
		TestEvent e(i, 23.0);
		PaUtil_WriteRingBuffer( &eventQueue, &e, 1 );
	}
	
	while(PaUtil_GetRingBufferReadAvailable(&eventQueue))
	{
		TestEvent e;
		PaUtil_ReadRingBuffer( &eventQueue, &e, 1 );
		std::cout << e.mTime << "\n";
	}
	
	// write some events wrapped around buffer end 
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

	std::iter_swap(i1, i2);
	
	std::cout << "---------------------------\n\n";
	for(auto it = v.begin(); it != v.end(); it++)
	{	
		std::cout << (*it).mTime << " : " << (*it).mValue1 << "\n";
	}
	
	std::sort(v.begin(), v.end(), [](TestEvent a, TestEvent b){ return a.mValue1 < b.mValue1; });
 
	std::cout << "---------------------------\n\n";
	for(auto it = v.begin(); it != v.end(); it++)
	{	
		std::cout << (*it).mTime << " : " << (*it).mValue1 << "\n";
	}
}

