
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#include "MLPropertySet.h"
#include "MLTimer.h"

// an MLModel is a kind of PropertySet that is also its own PropertyListener.
// Subclasses override doPropertyChangeAction to propagate any changes from Properties to the core logic.

class MLModel : public MLPropertySet, public MLPropertyListener
{
public:
	MLModel();
	virtual ~MLModel();
	
	// starts periodic listening to parameter changes, as made by UI, for example.
	// this listening will happen on the message thread. A Model doing DSP in a realtime thread
	// must not turn this on! instead call updateChangedProperties where appropriate.
	void startModelTimer();
	
private:
	ml::Timer mTimer;
};
