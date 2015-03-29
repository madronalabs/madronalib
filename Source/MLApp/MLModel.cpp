
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLModel.h"

// MLModel

const int kMLModelUpdateInterval = 100;

MLModel::MLModel() :
    MLPropertyListener(this)
{
	mpTimer = std::unique_ptr<ModelTimer>(new ModelTimer(this));
}

MLModel::~MLModel()
{
	mpTimer->stopTimer();
}

void MLModel::startModelTimer()
{
	mpTimer->startTimer(kMLModelUpdateInterval);
}

// MLModel::ModelTimer

MLModel::ModelTimer::ModelTimer(MLModel* pM) :
	mpOwnerModel(pM)
{
}

MLModel::ModelTimer::~ModelTimer()
{
	stopTimer();
}

void MLModel::ModelTimer::timerCallback()
{
    // pull property changes from the PropertySet to the PropertyListener
    mpOwnerModel->updateChangedProperties();
}


