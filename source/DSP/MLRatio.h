
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include <iostream>
#include <list>
#include "math.h"

#ifndef _ML_RATIO_H
#define _ML_RATIO_H

class MLRatio
{
public:
	int top;
	int bottom;

	MLRatio() : top(1), bottom(1) {};
	MLRatio(int a) : top(a), bottom(1) {};
	MLRatio(int a, int b) : top(a), bottom(b) {};
	~MLRatio() {} ;
	
	void simplify(void);
	bool isInteger(void) const { return(bottom == 1); }
	bool isUnity(void) const { return(bottom == top); }
	bool isZero(void) const { return(0 == top); }
	
	void set(int t, int b);
	
	MLRatio& operator*= ( const MLRatio& b );
	MLRatio& operator/= ( const MLRatio& b );
	MLRatio& operator+= ( const MLRatio& b );
	MLRatio& operator= ( const MLRatio& b );
	
	inline float getFloat() const { return (static_cast< float >(top))/(static_cast< float >(bottom)); } 
	inline explicit operator bool() { return (bottom != 0); }

private:
	
};


std::ostream& operator<< (std::ostream& out, const MLRatio & r);
MLRatio operator* (const MLRatio&,  const MLRatio&);
float operator* (const float,  const MLRatio&);
float operator* (const MLRatio&, const float);
MLRatio operator+ (const MLRatio&,  const MLRatio&);
bool operator== (const MLRatio&,  const MLRatio&);
bool operator> (const MLRatio&,  const MLRatio&);
bool operator< (const MLRatio&,  const MLRatio&);
bool operator>= (const MLRatio&,  const MLRatio&);
bool operator<= (const MLRatio&,  const MLRatio&);

class MLCommonRatios
{
	public:
		MLCommonRatios();
		~MLCommonRatios();
		
		MLRatio getClosest(float f) const;
	private:
		std::list<MLRatio> mRatios;
};

extern MLCommonRatios& getCommonRatios();




#endif // _ML_RATIO_H
