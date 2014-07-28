
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_MODEL__
#define __ML_MODEL__

#include "MLProperty.h"

// an MLModel is a kind of PropertySet that is also its own PropertyListener.
// Override doPropertyChangeAction to propagate any changes from Properties to the core logic.

class MLModel : public MLPropertySet, public MLPropertyListener, public juce::Timer
{
public:
	MLModel();
	virtual ~MLModel();
    void timerCallback();
    // TODO write a Timer class. juce::Timer is the only reason Juce is needed here. temporary.
};

#endif // __ML_MODEL__

