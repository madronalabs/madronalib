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

const int kTestBufferSize = 64; // must be power of two!

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
	eventData.resize(kTestBufferSize);
	PaUtil_InitializeRingBuffer( &eventQueue, sizeof(TestEvent), kTestBufferSize, &(eventData[0]) );

	// write and read some events to get near the end of the physical buffer
	int padding = kTestBufferSize - 10;
	for(int i=0; i<padding; ++i)
	{
		TestEvent e(i, 23.0);
		PaUtil_WriteRingBuffer( &eventQueue, &e, 1 );
	}	
	while(PaUtil_GetRingBufferReadAvailable(&eventQueue))
	{
		TestEvent e;
		PaUtil_ReadRingBuffer( &eventQueue, &e, 1 );
	}
	
	// write some events wrapped around buffer end 
	const int testElems = 20;
	for(int i=0; i<testElems; ++i)
	{
		int time = i;
		float v1 = (time > 8) ? (36.2 - i + 0.5f) : 99.;
		TestEvent e(time, v1);
		PaUtil_WriteRingBuffer( &eventQueue, &e, 1 );
	}
	
	// test sorting a divided vector of current elements through RingBufferElementsVector
	RingBufferElementsVector<TestEvent> v(&eventQueue);
	RingBufferElementsVector<TestEvent>::iterator i1 = v.begin();
	RingBufferElementsVector<TestEvent>::iterator i2 = i1;
	i1 += 1;
	i2 += 5;
	std::iter_swap(i1, i2);
	std::sort(v.begin(), v.end(), [](TestEvent a, TestEvent b){ return a.mValue1 < b.mValue1; });

	// read out elements one at a time and check for sorted order
	bool sorted = true;
	float valueA = -MAXFLOAT;
	float valueB;
	bool empty = false;
	int count = 0;
	while(!empty)
	{	
		TestEvent e;
		empty = !(PaUtil_ReadRingBuffer( &eventQueue, &e, 1 ));
		if(!empty) 
		{			
			valueB = e.mValue1;		
			if(valueA > valueB) 
			{
				sorted = false;
				std::cout << "not sorted! (" << valueA << " > " << valueB << ") \n";
			}
			valueA = valueB;
			count++;
		}
	}
	REQUIRE(sorted);	
	REQUIRE(count == testElems);
}

