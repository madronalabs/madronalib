
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLMenuButton.h"
#include "MLLookAndFeel.h"


MLMenuButton::MLMenuButton () :
    MLButton (),
	mMenuTextStyle(true)
{
 	setRepaintsOnMouseActivity(false);
}

MLMenuButton::~MLMenuButton()
{
}

/*
void MLMenuButton::setStringAttribute(MLSymbol sym, const std::string& val)
{
	MLWidget::setStringAttribute(sym, val);
    if((int)getAttribute("strip") > 0)
    {
        setButtonText(stripExtension(getShortName(val)).c_str());	    
    }
    else
    {
        setButtonText(val.c_str());
	}
    repaint();
}
*/

void MLMenuButton::paintButton (Graphics& g,
                              bool isMouseOverButton,
                              bool isButtonDown)
{
	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
	myLookAndFeel->drawBackground(g, this);
	const Colour c (findColour (MLTextButton::buttonColourId));	
	const Colour t (findColour (MLTextButton::textColourId));
    
    bool isActive = getFloatProperty("value") > 0.5f;
	/*
    myLookAndFeel->drawButtonBackground (g, *this,
		c,
		isMouseOverButton,
		isActive, mLineThickness);
	*/
	//MLTEST
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
}

void MLMenuButton::colourChanged()
{
    repaint();
}

/*
// make menu buttons trigger right away
//
void MLMenuButton::mouseDown(const MouseEvent& e)
{
	Button::mouseDown(e);
	if (mpListener)
	{
        setProperty("value", 1);
        repaint();
        
		// send our Widget name to listener as menu instigator
		mpListener->showMenu(getTargetPropertyName(), getWidgetName());
	}	
}

*/
