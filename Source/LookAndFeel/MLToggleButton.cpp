
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLToggleButton.h"
#include "MLLookAndFeel.h"

MLToggleButton::MLToggleButton()
    : MLButton()
{
    setOpaque(false);
}

MLToggleButton::~MLToggleButton()
{
}

void MLToggleButton::setAttribute(MLSymbol attr, float val)
{
	static const MLSymbol valueSym("value");
	MLWidget::setAttribute(attr, val);
	
	bool newState = (val == getOnValue());	
	if (attr == valueSym)
	{
		// update state without notify
        juce::Button::setToggleState(newState, false);
	}
}

void MLToggleButton::paintButton(Graphics& g, bool isMouseOverButton, bool isButtonDown)
{
	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
	
	// colors	
	const Colour offColor (findColour (MLLookAndFeel::darkFillColor));		
	const Colour onColor (findColour (MLButton::buttonOnColourId));
	const Colour bc = (getToggleState() ? onColor : offColor);
    
	const float alpha = isEnabled() ? 1.f : 0.25f;	
	const Colour textColor (findColour (MLButton::textColourId).withMultipliedAlpha (alpha));	
	Colour buttonColor = bc.withAlpha (alpha);
    
	Colour outlineColor, outlineOnColor, outlineOffColor;
	outlineOnColor = findColour(MLLookAndFeel::outlineColor).overlaidWith(onColor.withMultipliedAlpha(0.625f));
	outlineOffColor = findColour(MLLookAndFeel::outlineColor);
	outlineColor = getToggleState() ? outlineOnColor : outlineOffColor;
	outlineColor = outlineColor.withAlpha (alpha);

	// geometry
    const int width = getWidth();
    const int height = getHeight();	
	int toggleSize = myLookAndFeel->getToggleButtonSize() * getWidgetGridUnitSize() * getSizeMultiplier();
	int halfSize = toggleSize/2;
		
	// get int center
	int cx = width/2 - 1;
	int cy = height/2 - 1;
	
	// get toggle rect
	int toggleX = cx - halfSize;
	int toggleY = cy - halfSize;
	int toggleWidth = halfSize*2;
	int toggleHeight = halfSize*2;
	
	int flair = 0;
	if (isButtonDown) flair |= (eMLAdornPressed);
	flair |= eMLAdornShadow;
	flair |= eMLAdornGlow;	
	
	const float cornerSize = 0.;
	if (getAttribute("split"))
	{
		// dark background
		myLookAndFeel->drawMLButtonShape (g, toggleX, toggleY, toggleWidth, toggleHeight, 
			cornerSize, offColor, outlineOffColor, kMLButtonOutlineThickness, flair, 0., 0.);
			
		// light half
		g.saveState();
		if (!getToggleState())
		{
			g.reduceClipRegion(toggleX - 1, toggleY - 1, halfSize + 1, toggleHeight + 1);
		}
		else
		{
			g.reduceClipRegion(toggleX + halfSize, toggleY - 1, halfSize + 1, toggleHeight + 1);
		}
		myLookAndFeel->drawMLButtonShape (g, toggleX, toggleY, toggleWidth, toggleHeight, 
			cornerSize, onColor, outlineOnColor, mLineThickness, flair, 0., 0.);
		g.restoreState();
	}
	else
	{
		myLookAndFeel->drawMLButtonShape (g, toggleX, toggleY, toggleWidth, toggleHeight, 
			cornerSize, buttonColor, outlineColor, mLineThickness, flair, 0., 0.);		
	}
		
    /*
	if (mImage.isValid())
	{
		float imageAlpha = getToggleState() ? 0.5f : 1.f;

		g.setColour(findColour(MLLookAndFeel::labelColor).withMultipliedAlpha(imageAlpha));
		
		int ww = mImage.getWidth();
		int hh = mImage.getHeight();

		// draw image and let component clip.  
		// workaround for garbage bug with small subrects.
		g.drawImage(mImage, 
			0, 0, ww, hh,
			mImageOffset.x(), mImageOffset.y(), ww, hh,
			true);
	}*/
	
	/*
	// TEST
	Path bounds;
	const MLRect boundsRect ( getLocalBounds());	
	bounds.addRectangle(boundsRect);
	g.setColour(Colours::green.withAlpha(0.5f));	
	g.fillPath(bounds);
	g.setColour(Colours::red);	
	g.strokePath(bounds, PathStrokeType(1.0f));
	*/
	
}

void MLToggleButton::resizeWidget(const MLRect& b, const int u)
{
	Component* pC = getComponent();
	mLineThickness = u/128.f;
	if(pC)
	{
		MLRect bb = b;
		bb.expand(-2);
		
		// make sure width is odd
		int newWidth = (bb.width()/2)*2 + 1;

		// adapt vrect to juce rect
		Rectangle<int> c(bb.left(), bb.top(), newWidth, bb.height());
		
		pC->setBounds(c);
	}
}

