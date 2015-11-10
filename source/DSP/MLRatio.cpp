
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLRatio.h"

std::ostream& operator<< (std::ostream& out, const MLRatio & r)
{
	out << r.top;
	out << '/';
	out << r.bottom;
	return out;
}

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

void MLRatio::simplify(void)
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

void MLRatio::set(int t, int b)
{
	top = t;
	bottom = b;
}
	
MLRatio& MLRatio::operator*= (const MLRatio& b)
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

MLRatio& MLRatio::operator/= (const MLRatio& b)
{
	top = top * b.bottom;
	bottom = bottom * b.top;
	simplify();
	return *this;
}

MLRatio& MLRatio::operator+= (const MLRatio& b)
{
	top = top * b.bottom + b.top * bottom;
	bottom = bottom * b.bottom;
	simplify();
	return *this;
}

MLRatio& MLRatio::operator= (const MLRatio& b)
{
	top = b.top;
	bottom = b.bottom;
	return *this;
}
	
MLRatio operator* (const MLRatio& a, const MLRatio& b)
{
	MLRatio p = a;
	p *= b;
	p.simplify();
	return p;
}

float operator* (const float f, const MLRatio& b)
{
	return f*(float)b.top/(float)b.bottom;
}

float operator* (const MLRatio& b, const float f)
{
	return f*(float)b.top/(float)b.bottom;
}

MLRatio operator+ (const MLRatio& a, const MLRatio& b)
{
	MLRatio p = a;
	p += b;
	return p;
}

bool operator== (const MLRatio& a,  const MLRatio& b)
{
	return ((a.top == b.top) && (a.bottom == b.bottom));
}
	
bool operator> (const MLRatio& a,  const MLRatio& b)
{
	return (a.getFloat() > b.getFloat());
}
	
bool operator< (const MLRatio& a,  const MLRatio& b)
{
	return (a.getFloat() < b.getFloat());
}
	
bool operator>= (const MLRatio& a,  const MLRatio& b)
{
	return (a.getFloat() >= b.getFloat());
}
	
bool operator<= (const MLRatio& a,  const MLRatio& b)
{
	return (a.getFloat() <= b.getFloat());
}
		
// ----------------------------------------------------------------
// MLCommonRatios

const int kRecips = 16;
static const int recips[kRecips] = {12, 14, 15, 16, 20, 25, 32, 36, 42, 50, 64, 100, 128, 256, 512, 1024}; 

MLCommonRatios::MLCommonRatios() :
	mRatios(std::list<MLRatio>())
{
	mRatios.push_back(MLRatio(0, 1));
	mRatios.push_back(MLRatio(1, 1));

	// add small ratios
	int maxDiv = 11;
	for(int n = 1; n <= maxDiv; ++n)
	{
		for(int d = 2; d <= maxDiv; ++d)
		{
			if (GCD(n, d) == 1)
			{
				mRatios.push_back(MLRatio(n, d));
				mRatios.push_back(MLRatio(d, n));
			}
		}
	}
	
	// add 1/n list above max
	for(int n=0; n < kRecips; ++n)
	{
		mRatios.push_back(MLRatio(1, recips[n]));
		mRatios.push_back(MLRatio(recips[n], 1));
	}
}

MLCommonRatios::~MLCommonRatios()
{

}

MLRatio MLCommonRatios::getClosest(float f) const
{
	float minDistance = 16384.f;
	float distance;
	MLRatio result(1, 1);
	for (std::list<MLRatio>::const_iterator i = mRatios.begin(); i != mRatios.end(); ++i)
	{
		MLRatio b = *i;
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


MLCommonRatios& getCommonRatios()
{
	static MLCommonRatios theCommonRatios;
	return theCommonRatios;
}


