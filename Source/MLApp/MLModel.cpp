
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLModel.h"


// MLModel

const int kMLModelUpdateInterval = 10;

MLModel::MLModel() :
    MLPropertyListener(this)
{
	mpTimer = std::tr1::shared_ptr<ModelTimer>(new ModelTimer(this));
	mpTimer->startTimer(kMLModelUpdateInterval);
}

MLModel::~MLModel()
{
	mpTimer->stopTimer();
}


// MLModel::ModelTimer

MLModel::ModelTimer::ModelTimer(MLModel* pM) :
	mpModel(pM)
{
}

MLModel::ModelTimer::~ModelTimer()
{
	stopTimer();
}

void MLModel::ModelTimer::timerCallback()
{
    // pull property changes from the PropertySet to the PropertyListener
    mpModel->updateChangedProperties();
}


