// madronaLib: a C++ framework for DSP applications.
// Copyright (c) 2020 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include <iostream>
#include <list>
#include "math.h"

#pragma once

namespace ml 
{
	inline int GCD(int a, int b)
	{
		if (a == 0) return b;
		while(b > 0)
		{
			if (a > b)
				a = a - b;
			else
				b = b - a;
		}
		return a;
	}
	
	class Ratio
	{
	public:
		int top;
		int bottom;

		Ratio() : top(1), bottom(1) {};
		Ratio(int a) : top(a), bottom(1) {};
		Ratio(int a, int b) : top(a), bottom(b) {};
		~Ratio() {};

		inline void simplify(void)
		{
			if (bottom == top)
			{
				top = 1;
				bottom = 1;
			}
			else
			{
				int g = GCD(bottom, top);
				if (g != 1)
				{
					bottom /= g;
					top /= g;
				}
			}
		}
		
		inline bool isInteger(void) const { return(bottom == 1); }
		inline bool isUnity(void) const { return(bottom == top); }
		inline bool isZero(void) const { return(0 == top); }

		//void set(int t, int b);

		inline Ratio& operator*= (const Ratio& b)
		{
			if (top == b.bottom)
			{
				top = b.top;
			}
			else if (bottom == b.top)
			{
				bottom = b.bottom;
			}
			else
			{
				top = top * b.top;
				bottom = bottom * b.bottom;
				simplify();
			}
			return *this;
		}
		
		inline Ratio& operator/= (const Ratio& b)
		{
			top = top * b.bottom;
			bottom = bottom * b.top;
			simplify();
			return *this;
		}
		
		inline Ratio& operator+= (const Ratio& b)
		{
			top = top * b.bottom + b.top * bottom;
			bottom = bottom * b.bottom;
			simplify();
			return *this;
		}
		
		inline Ratio& operator= (const Ratio& b)
		{
			top = b.top;
			bottom = b.bottom;
			return *this;
		}
		
		inline float getFloat() const { return (static_cast<float>(top)) / (static_cast<float>(bottom)); }
		inline explicit operator bool() { return (bottom != 0); }
	};


	inline std::ostream& operator<< (std::ostream& out, const Ratio & r)
	{
		out << r.top;
		out << '/';
		out << r.bottom;
		return out;
	}


	inline Ratio operator* (const Ratio& a, const Ratio& b)
	{
		Ratio p = a;
		p *= b;
		p.simplify();
		return p;
	}
	
	inline float operator* (const float f, const Ratio& b)
	{
		return f*(float)b.top/(float)b.bottom;
	}
	
	inline float operator* (const Ratio& b, const float f)
	{
		return f*(float)b.top/(float)b.bottom;
	}
	
	inline Ratio operator+ (const Ratio& a, const Ratio& b)
	{
		Ratio p = a;
		p += b;
		return p;
	}
	
	inline bool operator== (const Ratio& a,  const Ratio& b)
	{
		return ((a.top == b.top) && (a.bottom == b.bottom));
	}
	
	inline bool operator> (const Ratio& a,  const Ratio& b)
	{
		return (a.getFloat() > b.getFloat());
	}
	
	inline bool operator< (const Ratio& a,  const Ratio& b)
	{
		return (a.getFloat() < b.getFloat());
	}
	
	inline bool operator>= (const Ratio& a,  const Ratio& b)
	{
		return (a.getFloat() >= b.getFloat());
	}
	
	inline bool operator<= (const Ratio& a,  const Ratio& b)
	{
		return (a.getFloat() <= b.getFloat());
	}

	
	// ----------------------------------------------------------------
	// CommonRatios
	
	class CommonRatios
	{
	private:
		static constexpr size_t kRecips = 16;
		static constexpr int recips[kRecips]  {12, 14, 15, 16, 20, 25, 32, 36, 42, 50, 64, 100, 128, 256, 512, 1024};
	
	public:
		std::list<Ratio> mRatios;
		
		CommonRatios() :
		mRatios(std::list<Ratio>())
		{
			mRatios.push_back(Ratio(0, 1));
			mRatios.push_back(Ratio(1, 1));
			
			// add small ratios
			int maxDiv = 11;
			for(int n = 1; n <= maxDiv; ++n)
			{
				for(int d = 2; d <= maxDiv; ++d)
				{
					if (GCD(n, d) == 1)
					{
						mRatios.push_back(Ratio(n, d));
						mRatios.push_back(Ratio(d, n));
					}
				}
			}
			
			// add 1/n list above max
			for(int n=0; n < kRecips; ++n)
			{
				mRatios.push_back(Ratio(1, recips[n]));
				mRatios.push_back(Ratio(recips[n], 1));
			}
		}
		
		~CommonRatios() = default;
		
		static CommonRatios& theCommonRatios()
		{
			static CommonRatios _CommonRatios;
			return _CommonRatios;
		}
		
		static Ratio getClosest(float f)
		{
			float minDistance = 16384.f;
			float distance;
			Ratio result(1, 1);
			
			CommonRatios& c(theCommonRatios());
			
			for (std::list<Ratio>::const_iterator i = c.mRatios.begin(); i != c.mRatios.end(); ++i)
			{
				Ratio b = *i;
				float fb = (b).getFloat();
				distance = fabsf(fb - f);
				if (distance < minDistance)
				{
					minDistance = distance;
					result = b;
				}
				if (distance < 0.00001f)
				{
					break;
				}
				
			}
			return result;
		}
	};	
} // namespace ml
