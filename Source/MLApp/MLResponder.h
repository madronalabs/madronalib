
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_RESPONDER_H
#define __ML_RESPONDER_H

#include "MLModel.h"

// include all widgets that report values ( make allwidgets header file? )
#include "MLButton.h"
#include "MLMenuButton.h"
#include "MLMultiButton.h"
#include "MLDial.h"
#include "MLMultiSlider.h"
#include "MLMenu.h"

// --------------------------------------------------------------------------------
#pragma mark MLResponder

// Responders listen to UI objects, and tell Models to do things.

class MLResponder  : 
	public MLButton::Listener,
	public MLMenuButton::Listener,
	public MLMultiButton::Listener,
	public MLDial::Listener,
	public MLMultiSlider::Listener
{
public:
	MLResponder() {}
    virtual ~MLResponder() {}

	// from MLButton::Listener
    virtual void buttonClicked (MLButton*) = 0;
	
	// from MLMenuButton::Listener
 	virtual void showMenu(MLSymbol menuName, MLSymbol instigatorName) = 0;
	virtual void menuItemChosen(MLSymbol menuName, int result) = 0;

 	// from MLDial::Listener
	virtual void dialDragStarted (MLDial*) = 0;
    virtual void dialValueChanged (MLDial*) = 0;
	virtual void dialDragEnded (MLDial*) = 0;
	
	// from MLMultiSlider::Listener
	//virtual void multiSliderDragStarted (MLMultiSlider* pSlider, int idx) = 0;
    virtual void multiSliderValueChanged (MLMultiSlider* , int ) = 0;
	//virtual void multiSliderDragEnded (MLMultiSlider* pSlider, int idx) = 0;

	// from MLMultiButton::Listener
    virtual void multiButtonValueChanged (MLMultiButton* , int ) = 0;
};

#endif // __ML_RESPONDER_H