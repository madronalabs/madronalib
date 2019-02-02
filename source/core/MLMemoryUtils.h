
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2018 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

namespace ml
{
	// ----------------------------------------------------------------
	// SmallStackBuffer - allocate some memory on the stack if we don't need much,
	// otherwise use the heap.
	
	template< class T, int MAX_STACK_ELEMS >
	class SmallStackBuffer
	{
	public:
		SmallStackBuffer(int size)
		{
			if(size <= MAX_STACK_ELEMS)
			{
				mpData = mLocalData;
				std::fill(mpData, mpData + size, T());
			}
			else
			{
				mpData = new T[size];
			}
		}
		
		~SmallStackBuffer()
		{
			if(mpData != mLocalData) 
			{
				delete mpData;
			}
		}
		
		T* data() { return mpData; }
				
	private:
		T* mpData;
		T mLocalData[MAX_STACK_ELEMS];
	};
}

