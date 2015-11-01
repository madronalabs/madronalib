
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLMenuButton.h"
#include "MLLookAndFeel.h"

MLMenuButton::MLMenuButton () :
    MLButton (),
	mStyle(kRightArrowStyle)
{
	mTriggerOnMouseDown = true;
}

MLMenuButton::~MLMenuButton()
{
}

void MLMenuButton::setStyle(int t)
{
	mStyle = t;
}

void MLMenuButton::paint(Graphics& g)
{
	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
	const Colour c (findColour (MLTextButton::buttonColourId));
	const Colour t (findColour (MLTextButton::textColourId));
	myLookAndFeel->drawBackground(g, this);
	
	if(mStyle != kTextOnlyStyle)
	{
		myLookAndFeel->drawButtonBackground(g, *this, c, mOver, mToggleState, mLineThickness);
	}
	
	switch(mStyle)
	{
		case kPlainStyle:
			myLookAndFeel->drawButtonText(g, *this, t, mOver, mDown);
			break;
		case kRightArrowStyle:
			myLookAndFeel->drawMenuButtonText(g, *this, t);
			break;
		case kTextOnlyStyle:
			myLookAndFeel->drawButtonText(g, *this, t, mOver, mDown);
			break;
	}
}

void MLMenuButton::clicked ()
{
	sendAction("show_menu", getTargetPropertyName());
	setPropertyImmediate("value", 1);
}

void MLMenuButton::doPropertyChangeAction(MLSymbol property, const MLProperty& val)
{
	if (property == "text")
	{
		// TODO this file-specific stuff should not be here. 
		std::string processedText;
		const std::string str = val.getStringValue();
		if(getFloatProperty("strip"))
		{
			processedText = ml::stringUtils::stripExtension(ml::stringUtils::getShortName(str));
		}
		else
		{
			processedText = str;
		}
		setProperty("processed_text", processedText);		
		repaint();
	}
	else
	{
		MLButton::doPropertyChangeAction(property, val);
	}
}

