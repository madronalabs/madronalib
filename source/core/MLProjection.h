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
#include "MLDSP.h"
#include "MLInterval.h"

// TODO replace MLRange with this stuff
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
		// constant projections 		
		extern const Projection linear;
		extern const Projection flip;
		extern const Projection clip;
		extern const Projection smoothstep;
		extern const Projection bell;
		
		// functions returning projections
		
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
	}
	
	// IntervalProjection: a functor that maps one interval to another with
	// an optional mapping projection defined on (0, 1)->(0,1).
	
	// TODO how can we write these functors to be composable as well?
	
	class IntervalProjection
	{
	public:
		// define a projection from interval a to interval b.
		// projection c is defined on [0, 1)->[0, 1) and can add clipping or nonlinear projections. 
		IntervalProjection(const Interval a, const Interval b, Projection c = projections::linear) : mA(a), mB(b), mMappingProjection(c) { build(); }
		
		inline float operator()(float f) const 
		{
			return mMappingProjection(f*mScaleA + mOffsetA)*mScaleB + mOffsetB;
		}	
		
	private:
		const Interval mA, mB;
		const Projection mMappingProjection;
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
	
	class TableProjection
	{
	public:
		TableProjection(std::initializer_list<float> values);
		float operator()(float f) const;
		
	private:
		std::vector<float> mTable;
	
	};
}
