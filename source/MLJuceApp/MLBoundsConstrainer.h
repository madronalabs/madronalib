// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_BOUNDS_CONSTRAINER__
#define __ML_BOUNDS_CONSTRAINER__

#include "JuceHeader.h"
using namespace juce;

#include "MLDSP.h"
#include "MLUI.h"
#include "MLDebug.h"

//#include "juce_ComponentBoundsConstrainer.h"

class MLBoundsConstrainer  : public juce::ComponentBoundsConstrainer
{
public:
    MLBoundsConstrainer();
	~MLBoundsConstrainer();
	
	void checkBounds (juce::Rectangle<int>& bounds,
		const Rectangle<int>& old,
		const Rectangle<int>& limits,
		const bool isStretchingTop,
		const bool isStretchingLeft,
		const bool isStretchingBottom,
		const bool isStretchingRight);
		
	void setZoomable(bool z) { mZoomable = z; }
private:		
	bool mZoomable;	

};

#endif // __ML_BOUNDS_CONSTRAINER__