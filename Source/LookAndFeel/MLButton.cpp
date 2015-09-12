
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLButton.h"
#include "MLLookAndFeel.h"

MLButton::MLButton () :
	mLabelOffset(MLPoint(0, 0)),
	mOffValue(0.f),
	mOnValue(1.f),
	mOver(false),
	mDown(false),
	mToggleState(0),
	mTriggerOnMouseDown(false)
{
	MLWidget::setComponent(this);
    setWantsKeyboardFocus (false);
    setRepaintsOnMouseActivity (false);
	setOpaque(true);
	
	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
	setBufferedToImage(myLookAndFeel->getDefaultBufferMode());
	
	setProperty("toggle", true);
	mDoRollover = false;
	
	setFillColor (Colours::lightgrey);
	setColour ((const int)textColourId, Colours::black);
	mImageOffset = Vec2(0, 0);
}

void MLButton::paint (Graphics& g)
{
	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
	int d = myLookAndFeel->getToggleButtonSize() * getSizeMultiplier();

	myLookAndFeel->drawBackground(g, this);

	// colors
	const Colour bgColor (findColour (MLLookAndFeel::backgroundColor));	
	const Colour offColor (findColour (MLButton::buttonOffColourId));	
	const Colour onColor (findColour (MLButton::buttonOnColourId));	
	const Colour offBrightColor (offColor.getHue(), offColor.getSaturation(), jmin(offColor.getBrightness() + 0.1, 1.), offColor.getFloatAlpha());		
	const Colour onBrightColor (onColor.getHue(), onColor.getSaturation(), jmin(onColor.getBrightness() + 0.1, 1.), onColor.getFloatAlpha());		
	const Colour offOverColor ((mDoRollover && mOver) ? offBrightColor : offColor);
	const Colour onOverColor ((mDoRollover && mOver) ? onBrightColor : onColor);
	const Colour bc = (mToggleState ? onOverColor : offOverColor);

	const float alpha = isEnabled() ? 1.f : 0.25f;	
	const Colour textColor (findColour (MLButton::textColourId).withMultipliedAlpha (alpha));	
	const Colour track_hard (findColour(MLLookAndFeel::outlineColor).withMultipliedAlpha (alpha));	
	const Colour brightColor = Colour(bc.getHue(), bc.getSaturation(), jmin(bc.getBrightness() + 0.1, 1.), bc.getFloatAlpha());				
	Colour buttonColor = bc.withMultipliedAlpha (isEnabled() ? 1.f : 0.25f);
	
	Colour outlineColor, outlineOnColor, outlineOffColor;
	outlineOnColor = findColour(MLLookAndFeel::outlineColor).overlaidWith(onOverColor.withMultipliedAlpha(0.5f));
	outlineOffColor = findColour(MLLookAndFeel::outlineColor).withMultipliedAlpha (alpha);
	outlineColor = mToggleState ? outlineOnColor : outlineOffColor;
	
	if (mImage.isValid())
	{
		buttonColor = buttonColor.overlaidWith(onColor.withMultipliedAlpha(0.25f));
	}
	
	// geometry
    const int width = getWidth();
    const int height = getHeight();	
	int toggleX, toggleY;
	const int toggleWidth = d;
	const int toggleHeight = d; 
	const float cornerSize = 0.;
		
	// center toggle square
	toggleX = (width - toggleWidth)/2;
	toggleY = (height - toggleHeight)/2;
	
	int flair = 0;
	if (mDown) flair |= (eMLAdornPressed);
	flair |= eMLAdornShadow;
	flair |= eMLAdornGlow;	
	
	myLookAndFeel->drawMLButtonShape (g, toggleX, toggleY, toggleWidth, toggleHeight, 
		cornerSize, buttonColor, outlineColor, mLineThickness, flair, 0., 0.);
		
	if (mImage.isValid())
	{
		float imageAlpha = mToggleState ? 0.5f : 1.f;

		g.setColour(findColour(MLLookAndFeel::labelColor).withMultipliedAlpha(imageAlpha));
		
		int ww = mImage.getWidth();
		int hh = mImage.getHeight();

		// draw image and let component clip.  
		// workaround for garbage bug with small subrects.
		g.drawImage(mImage, 
			0, 0, ww, hh,
			mImageOffset.x(), mImageOffset.y(), ww, hh,
			true);
	}
	
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

void MLButton::setFillColor(const Colour c)
{
	setColour(buttonOnColourId, brighterColor(c));
    setColour(buttonOffColourId, findColour(MLLookAndFeel::backgroundColor).overlaidWith(Colours::black.withAlpha(0.5f)));
}

void MLButton::clicked ()
{
	bool oldT = mToggleState;
	bool newT = oldT;
	if(getFloatProperty("toggle"))
	{
		newT = !oldT;
	}
	else
	{
		sendAction("click", getTargetPropertyName());
	}
	
	if(oldT != newT)
	{
		mToggleState = newT;
		setPropertyImmediate ("value", (mToggleState ? mOnValue : mOffValue));
		sendAction("change_property", getTargetPropertyName(), getProperty("value"));
	}
}

void MLButton::mouseDown (const MouseEvent& e)
{
    if (!isEnabled() || !isVisible() || isCurrentlyBlockedByAnotherModalComponent()) return;
	mDown = true;
	mOver = isMouseOver();
    if (mOver && mTriggerOnMouseDown)
	{
        clicked ();
	}
	repaint();
}

void MLButton::mouseUp (const MouseEvent& e)
{
    if (!isEnabled() || !isVisible() || isCurrentlyBlockedByAnotherModalComponent()) return;
    const bool wasDown = mDown;
	const bool wasOver = mOver;
	mDown = false;
	mOver = isMouseOver();
    if (wasDown && wasOver && ! mTriggerOnMouseDown)
	{
        clicked ();
	}
	repaint();
}

void MLButton::mouseDrag (const MouseEvent&)
{
    if (!isEnabled() || !isVisible() || isCurrentlyBlockedByAnotherModalComponent()) return;
    mOver = isMouseOver();
}

void MLButton::resizeWidget(const MLRect& b, const int u)
{
	Component* pC = getComponent();
	if(pC)
	{
 		MLWidget::resizeWidget(b, u);
		mLineThickness = u/128.f;
    }
}

void MLButton::setLineThickness(float f)
{
	mLineThickness = f;
}

void MLButton::setToggleValues(float lo, float hi)
{
	mOffValue = lo;
	mOnValue = hi;
}

void MLButton::doPropertyChangeAction(MLSymbol property, const MLProperty& val)
{
	if (property == "value")
	{
		// translate lo / hi values back to toggle state
		float v = val.getFloatValue();
		mToggleState = (v > mOffValue);
	}
    else if(property == "highlight")
    {
    }
	repaint();
}

