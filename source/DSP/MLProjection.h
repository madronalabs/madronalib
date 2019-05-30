//
//  MLProjection.h
//  Virta
//
//  Created by Randy Jones on 12/27/15.
//
//

#pragma once

#include <functional>
#include <vector>
//#include <iostream>

#include "MLScalarMath.h"

namespace ml
{
	struct Interval
	{
		float mX1, mX2;
	};
	
	inline bool within(float f, const Interval m) { return (f >= m.mX1)&&(f < m.mX2); }
}

// TODO replace MLRange (deprecated) with this everywhere

namespace ml
{
	typedef std::function<float(float)> Projection;
	
	inline Projection compose(Projection a, Projection b)
	{
		return( [=](float x){return a(b(x));} );
	}

  // useful projections defined on (0, 1)
	namespace projections
	{
		// projections with no parameters
		
    static const Projection unity{ [](float x){return x;} };
    static const Projection squared{ [](float x){return x*x;} };
		static const Projection flip{ [](float x){return 1 - x;} };
		static const Projection clip{ [](float x){return ml::clamp(x, 0.f, 1.f);} };
		static const Projection smoothstep{ [](float x){return 3*x*x - 2*x*x*x;} };
		static const Projection flatcenter{ [](float x){float c = (x - 0.5f); return 4*c*c*c + 0.5f;} };
		static const Projection bell{ [](float x){float px = x*2 - 1; return powf(2.f, -(10.f*px*px));} };

		// functions taking one or more parameters and returning projections
		
		// returns a projection that will be logarithmic when scaled and offset to (a, b).
		// works for positive a, b with a < b only.
		inline Projection log(Interval m)
		{
			return( [=](float x){float a = m.mX1; float b = m.mX2; return a*(powf((b/a), x) - 1)/(b - a);} );
		}
		
		// the inverse of the log projection.
		// works for positive a, b with a < b only.
		inline Projection exp(Interval m)
		{
			return( [=](float x){float a = m.mX1; float b = m.mX2; return logf((x*(b - a) + a)/a)/logf(b/a);} );
		}
		
		// a projection mapping an interval to another interval 
		inline Projection intervalMap(const Interval a, const Interval b)
		{
			return( [=](float x)
						 {
							 // project interval a to interval (0,1)
							 float m = (b.mX2 - b.mX1)/(a.mX2 - a.mX1);
							 return m*(x - a.mX1) + b.mX1;
						 }
						 );
		}
		
		// a projection mapping an interval to another interval with an intermediate shaping projection on [0, 1]
		inline Projection intervalMap(const Interval a, const Interval b, Projection c)
		{
			return( [=](float x)
						 {
							 // project interval a to interval (0,1)
							 const float scaleA = 1/(a.mX2 - a.mX1);
							 const float offsetA = (-a.mX1) / (a.mX2 - a.mX1);
							 // project interval (0, 1) to interval b
							 const float scaleB = (b.mX2 - b.mX1);
							 const float offsetB = b.mX1;
							 return c(x*scaleA + offsetA)*scaleB + offsetB;
						 }
						 );
		}
		
		inline Projection piecewiseLinear(std::initializer_list<float> values)
		{
			const std::vector<float> table(values);		
			
			if(table.size() > 1)
			{
				return( [=](float x)
							 {
								 float ni = table.size() - 1;
								 float nf = static_cast<float>(ni);
								 float xf = nf*clamp(x, 0.f, 1.f);		
								 int xi = static_cast<int>(xf);
								 float xr = xf - xi;
								 
								 if(x < 1.0f)
								 {
									 return lerp(table[xi], table[xi + 1], xr);
								 }
								 else
								 {
									 return table[ni];
								 }
							 }
							 );
			}
			else if(table.size() == 1)
			{
				return ( [=](float x){ return table[0]; } );
			}
			else
			{
				return ( [=](float x){ return 0.f; } );
			}
		}
	}
	
	// TODO maps on DSPVectors?
	
// TODO rename: m, n, p
	
	
	// IntervalProjection: a functor that maps one interval to another with
	// an optional mapping projection defined on (0, 1)->(0,1).
	
	// TODO DEPRECATED	
	
	class IntervalProjection
	{
	public:
		// define a projection from interval a to interval b.
		// projection c is defined on [0, 1)->[0, 1) and can add clipping or nonlinear projections. 
		//explicit IntervalProjection(const Interval a, const Interval b);
		
		explicit IntervalProjection(const Interval a, const Interval b, Projection c = projections::unity) : mA(a), mB(b), mMappingProjection(c) { build(); }
		
		inline float operator()(float f) const 
		{
			return mMappingProjection(f*mScaleA + mOffsetA)*mScaleB + mOffsetB;
		}	
		
	private:
		Interval mA, mB;
		Projection mMappingProjection { projections::unity };
		float mScaleA, mOffsetA, mScaleB, mOffsetB;
		
		void build()
		{
			// project interval a to interval (0,1)
			mScaleA = 1/(mA.mX2 - mA.mX1);
			mOffsetA = (-mA.mX1) / (mA.mX2 - mA.mX1);
			// project interval (0, 1) to interval b
			mScaleB = (mB.mX2 - mB.mX1);
			mOffsetB = mB.mX1;
		}
	};
	
	// DEPRECATED	
	
	
	class TableProjection
	{
	public:
		TableProjection(std::initializer_list<float> values)
		{
			for(float f : values)
			{
				mTable.push_back(f);
			}
		}
		
		float operator()(float f) const
		{
			float ni = mTable.size() - 1;
			float nf = static_cast<float>(ni);
			float xf = nf*clamp(f, 0.f, 1.f);		
			int xi = static_cast<int>(xf);
			float xr = xf - xi;
			
			if(ni > 0)
			{
				if(f < 1.0f)
				{
					return lerp(mTable[xi], mTable[xi + 1], xr);
				}
				else
				{
					return mTable[ni];
				}
			}
			else if(ni == 0)
			{
				// one item in table
				return mTable[0];
			}
			else
			{
				// empty table
				return 0.f;
			}
		}
		
	private:
		std::vector<float> mTable;
	
	};
}
