//
//  MLInterval.h
//  madronalib
//
//  Created by Randy Jones on 12/27/15.
//
//

#pragma once

// TODO replace MLRange with this stuff
namespace ml
{
	class Interval
	{
	public:
		Interval(float x1, float x2) : mX1(x1), mX2(x2) {}
		Interval(int x1, int x2) : mX1(x1), mX2(x2) {}
		
		float mX1, mX2;
	};
	
	inline bool within(float f, const Interval m) { return (f >= m.mX1)&&(f < m.mX2); }
}
