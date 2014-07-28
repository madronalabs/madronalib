
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLModel.h"

// --------------------------------------------------------------------------------
// MLModel

MLModel::MLModel() :
    MLPropertyListener(this)
{
    startTimer(30);
}

MLModel::~MLModel()
{

}

void MLModel::timerCallback()
{
    // pull property changes from the PropertySet to the PropertyListener
    updateChangedProperties();
}


