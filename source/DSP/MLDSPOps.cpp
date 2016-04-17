//
//  MLSSPOps.cpp
//  madronalib
//
//  Created by Randy Jones on 4/5/2016
//
//

#include "MLDSPOps.h"

using namespace ml;

std::ostream& operator<< (std::ostream& out, const ml::DSPVector& v)
{
	out << "[";
	for(int i=0; i<kFloatsPerDSPVector; ++i)
	{
		out << v[i] << " ";
	}
	out << "]\n";
	
	return out;
}
