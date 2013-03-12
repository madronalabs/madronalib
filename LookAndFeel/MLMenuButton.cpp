
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLMenuButton.h"
#include "MLLookAndFeel.h"


MLMenuButton::MLMenuButton ()
    : MLButton (),
	mMenuTextStyle(true)
{
 	setRepaintsOnMouseActivity(false);
}

MLMenuButton::~MLMenuButton()
{
}

void MLMenuButton::setAttribute(MLSymbol attr, float val)
{
	static const MLSymbol valueSym("value");
	MLWidget::setAttribute(attr, val);
	if (attr == valueSym)
	{
		repaint();
	}
}

void MLMenuButton::setStringAttribute(MLSymbol , const std::string& val)
{
	setButtonText(val.c_str());
	repaint();
}

void MLMenuButton::paintButton (Graphics& g,
                              bool isMouseOverButton,
                              bool isButtonDown)
{
	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
	myLookAndFeel->drawBackground(g, this);
	const Colour c (findColour (MLTextButton::buttonColourId));	
	const Colour t (findColour (MLTextButton::textColourId));	

    myLookAndFeel->drawButtonBackground (g, *this,
		c,
		isMouseOverButton,
		isButtonDown);
	
	if(mMenuTextStyle)
	{
		myLookAndFeel->drawMenuButtonText (g, *this, t);
	}
	else
	{
		myLookAndFeel->drawButtonText (g, *this,
			t,
			isMouseOverButton,
			isButtonDown);
	}

		/*
		// TEST
		debug() << "painting button " << getButtonText() << ", height" << getHeight() << ":\n";							
		Path bounds;
		bounds.addRectangle(getLocalBounds());
		g.setColour(Colours::yellow.withAlpha(0.5f));	
		g.fillPath(bounds);
		g.setColour(Colours::red);	
		g.strokePath(bounds, PathStrokeType(1.0f));
		*/
		
}

void MLMenuButton::colourChanged()
{
    repaint();
}

// make menu buttons trigger right away
//
void MLMenuButton::mouseDown(const MouseEvent& e)
{
	Button::mouseDown(e);
	
	// TODO set click state down until menu gone	
	if (mpListener)
	{
		mpListener->showMenu(getParamName(), this);
	}	
}


