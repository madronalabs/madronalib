//
//  MLDSPFilters.cpp
//  madronalib
//
//  Created by Randy Jones on 4/5/2016
//
//

#include "MLDSPFilters.h"

using namespace ml;


// ----------------------------------------------------------------
#pragma mark HalfBandFilter

const float HalfBandFilter::ka0 = 0.07986642623635751;
const float HalfBandFilter::ka1 = 0.5453536510711322;
const float HalfBandFilter::kb0 = 0.28382934487410993;
const float HalfBandFilter::kb1 = 0.8344118914807379;

HalfBandFilter::AllpassSection::AllpassSection() :
	a(0)
{
	clear();
}

HalfBandFilter::AllpassSection::~AllpassSection()
{
}

void HalfBandFilter::AllpassSection::clear()
{
	x0 = x1 = y0 = y1 = 0.f;
}

HalfBandFilter::HalfBandFilter()
{
	apa0.a = ka0;
	apa1.a = ka1;
	apb0.a = kb0;
	apb1.a = kb1;
	x0 = x1 = a0 = b0 = b1 = 0.f;
	k = 0;
	clear();
}

HalfBandFilter::~HalfBandFilter()
{
}

void HalfBandFilter::clear()
{
	apa0.clear();
	apa1.clear();
	apb0.clear();
	apb1.clear();
}


