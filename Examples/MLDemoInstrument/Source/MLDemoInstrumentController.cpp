
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLDemoInstrumentController.h"

void *OSCListenerThread(void *arg);

	
MLDemoInstrumentController::MLDemoInstrumentController(MLPluginProcessor* const pProcessor) :
	MLPluginController(pProcessor),
	mpProcessor(static_cast<MLPluginProcessor*>(pProcessor))
{

}

MLDemoInstrumentController::~MLDemoInstrumentController()
{
}

void MLDemoInstrumentController::initialize()
{
	MLAppView* pView = getView();
	if (pView)
	{
		pView->setAttribute("protocol", kInputProtocolMIDI);
	}
	
	startTimer(50);
}

void MLDemoInstrumentController::timerCallback()
{
	updateChangedParams();
	
	const juce::ScopedLock lock(mViewLock);
	if(getView() != nullptr)
	{
		viewSignals();
		debug().display();
	}
}


// --------------------------------------------------------------------------------
#pragma mark MLButton::Listener

void MLDemoInstrumentController::buttonClicked (MLButton* button)
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
