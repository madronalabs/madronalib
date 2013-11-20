
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLDrawableButton.h"
#include "MLLookAndFeel.h"

//==============================================================================
MLDrawableButton::MLDrawableButton (const String& name, const MLDrawableButton::ButtonStyle buttonStyle)
    : MLButton (name),
      style (buttonStyle)
{
    if (buttonStyle == ImageOnButtonBackground)
    {
        backgroundOff = Colour (0xffbbbbff);
        backgroundOn = Colour (0xff3333ff);
    }
    else
    {
        backgroundOff = Colours::transparentBlack;
        backgroundOn = findColour(MLLookAndFeel::backgroundColor);
    }
}

MLDrawableButton::MLDrawableButton ()
    : MLButton (""),
      style (ImageFitted)
{
	backgroundOff = Colours::transparentBlack;
	backgroundOn = findColour(MLLookAndFeel::backgroundColor);
}

MLDrawableButton::~MLDrawableButton()
{
}

void MLDrawableButton::setAttribute(MLSymbol , float )
{

}


//==============================================================================
void MLDrawableButton::setImage (const Drawable* img)
{
    jassert (img != 0); 
    if (img)        
	{
		normalImage = img->createCopy();
	}
}

//==============================================================================
void MLDrawableButton::setButtonStyle (const MLDrawableButton::ButtonStyle newStyle)
{
    if (style != newStyle)
    {
        style = newStyle;
    }
}

void MLDrawableButton::setBackgroundColours (const Colour& toggledOffColour, const Colour& toggledOnColour)
{
    if (backgroundOff != toggledOffColour
         || backgroundOn != toggledOnColour)
    {
        backgroundOff = toggledOffColour;
        backgroundOn = toggledOnColour;
        repaint();
    }
}

const Colour& MLDrawableButton::getBackgroundColour() const throw()
{
    return getToggleState() ? backgroundOn : backgroundOff;
}

void MLDrawableButton::paintButton (Graphics& g, bool isMouseOverButton, bool isButtonDown)
{
 	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
	if (isOpaque()) myLookAndFeel->drawBackground(g, this);	
	if (style == ImageOnButtonBackground)
    {
        myLookAndFeel->drawButtonBackground (g, *this,
			getBackgroundColour(), isMouseOverButton, isButtonDown, mLineThickness);
    }	

	if (style == ImageFitted)
	{
		float u = myLookAndFeel->getGridUnitSize();
		if(isButtonDown)
		{
			myLookAndFeel->drawButtonGlow (g, *this, backgroundOn);
		}
	
		Rectangle<int> imageSpace = getLocalBounds();								  
		if (isButtonDown)
		{
			imageSpace.translate(0, u/32.);
		}			  
		
		normalImage->setTransformToFit (imageSpace.toFloat(), RectanglePlacement::centred);
		normalImage->drawAt(g, 0, 0, 1.);

	}
}

//==============================================================================

void MLDrawableButton::resizeWidget(const MLRect& b, const int)
{
	Component* pC = getComponent();
	if(pC)
	{
	//	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
	//	float m = myLookAndFeel->getMargin() * myLookAndFeel->getGridUnitSize();
		
		// adapt vrect to juce rect
		Rectangle<int> c(b.left(), b.top(), b.width(), b.height());

/*		
		if(!isConnectedOnLeft())
		{
			c.setLeft(c.x() + m);
		}
		if(!isConnectedOnTop())
		{
			c.setTop(c.y() + m);
		}
		if(!isConnectedOnRight())
		{
			c.setWidth(c.getWidth() - m);
		}
		if(!isConnectedOnBottom())
		{
			c.setHeight(c.getHeight() - m);
		}
*/
		
		pC->setBounds(c);
	}
}

