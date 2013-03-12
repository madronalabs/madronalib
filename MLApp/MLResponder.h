
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

// TODO make pure virtual!!!

class MLResponder  : 
	public MLButton::Listener,
	public MLMenuButton::Listener,
	public MLMultiButton::Listener,
	public MLDial::Listener,
	public MLMultiSlider::Listener
{
public:

	MLResponder();
    ~MLResponder();

	// from MLButton::Listener
    virtual void buttonClicked (MLButton*){}
	
	// from MLMenuButton::Listener
 	virtual void showMenu(MLSymbol , MLMenuButton* ){}

 	// from MLDial::Listener
    virtual void dialValueChanged (MLDial*){}
	virtual void dialDragStarted (MLDial*){}
	virtual void dialDragEnded (MLDial*){}
	
	// from MLMultiSlider::Listener
	virtual void multiSliderDragStarted (MLMultiSlider* , int ) {}
	virtual void multiSliderDragEnded (MLMultiSlider* , int ) {}
    virtual void multiSliderValueChanged (MLMultiSlider* , int ) {}

	// from MLMultiButton::Listener
    virtual void multiButtonValueChanged (MLMultiButton* , int ) {}
	
protected:
	std::map<MLSymbol, MLMenuPtr> mMenuMap; 
	
private:	

};

#endif // __ML_RESPONDER_H