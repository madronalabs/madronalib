
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLExampleController.h"

void *OSCListenerThread(void *arg);

	
MLExampleController::MLExampleController(MLPluginProcessor* const pProcessor) :
	MLPluginController(pProcessor),
	mpProcessor(static_cast<MLPluginProcessor*>(pProcessor))
{

}

MLExampleController::~MLExampleController()
{
}

void MLExampleController::initialize()
{
	MLAppView* pView = getView();
	if (pView)
	{
		pView->setAttribute("protocol", kInputProtocolMIDI);
	}
	
	startTimer(50);
}

void MLExampleController::timerCallback()
{
	updateChangedParams();
}

// --------------------------------------------------------------------------------
#pragma mark MLButton::Listener

void MLExampleController::buttonClicked (MLButton* button)
{
	assert(button);
	
	MLSymbol name = button->getParamName();
	// debug() << "MLButton clicked: " << name << "\n";

	MLAppView* view = getView();
	assert(view);
	
	if (name == "prev")
	{
		prevPreset();
	}
	else if (name == "next")
	{
		nextPreset();
	}
	else
	{
		MLPluginController::buttonClicked (button);
	}
}
