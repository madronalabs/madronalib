
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include <chrono>

#include "MLModel.h"

// MLModel

const int kMLModelUpdateInterval = 100;

MLModel::MLModel() : MLPropertyListener(this)
{
}

MLModel::~MLModel()
{
}

void MLModel::startModelTimer()
{
	mTimer.start([&]() { updateChangedProperties(); }, milliseconds(kMLModelUpdateInterval));
}
