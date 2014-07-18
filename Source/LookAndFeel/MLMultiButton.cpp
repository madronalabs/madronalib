
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLMultiButton.h"
#include "MLLookAndFeel.h"

MLMultiButton::MLMultiButton () :
	mpListener(0)
{
	MLWidget::setComponent(this);
	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
	setOpaque(myLookAndFeel->getDefaultOpacity());
	setBufferedToImage(myLookAndFeel->getDefaultBufferMode());
	setPaintingIsUnclipped(myLookAndFeel->getDefaultUnclippedMode());

	setNumButtons(1);
	mButtonUnderMouse = -1;
	mCurrDragButton = -1;
	mMovedInDrag = false;
	setColour ((const int)textColourId, findColour(MLLookAndFeel::labelColor));
}

MLMultiButton::~MLMultiButton()
{
	deleteAllChildren();
}

void MLMultiButton::setAttribute(MLSymbol attr, float val)
{
	static const MLSymbol valueSym("value");
	
	// debug() << "MLMultiButton " << getWidgetName() << ":" << attr << " = " << val << "\n";
	
	// TODO other multibutton attributes?
	
	if (attr.withoutFinalNumber() == valueSym)
	{
		int bIdx = attr.getFinalNumber();
		if(within(bIdx, 0, (int)mButtonValues.size()))
		{
			mButtonValues[bIdx] = val;			
            triggerChangeMessage (false);
			repaint();
		}
	}
}


void MLMultiButton::setNumButtons(int n)
{
	if (n > 0)
	{
		mButtonValues.resize(n);
		mValueNeedsUpdate.resize(n);
		for(int i=0; i<n; ++i)
		{
			mButtonValues[i] = 0.;
			mValueNeedsUpdate[i] = false;
		}
	}
}

unsigned MLMultiButton::getNumButtons()
{
	return mButtonValues.size();
}


// the colors for different MLDial parts are generated algorithmically.
void MLMultiButton::setFillColor (const Colour& c)
{
	Colour on, off;
	float g = c.getFloatGreen();
	float b = 0.;

	// on color is brighter, less saturated vesion of thumb
	b = (1.f - g) * 2.;	
	on = Colour(c.getHue(), jmax(c.getSaturation() - (b*0.1), 0.), 
		jmin((c.getBrightness() + b*2.f), 1.f), c.getFloatAlpha());
	off = c.overlaidWith(Colours::darkgrey.withAlpha(0.75f));
	
	setColour(buttonOnColourId, on);
    setColour(buttonOffColourId, off);

    lookAndFeelChanged();
}


		
#pragma mark -

void MLMultiButton::paint (Graphics& g)
{
	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
	if (isOpaque()) myLookAndFeel->drawBackground(g, this);	
	int buttons = mButtonValues.size();
    
    // colors
	const Colour offColor (findColour (MLLookAndFeel::darkFillColor));
	const Colour onColor (findColour (MLMultiButton::buttonOnColourId));
    
	Colour outlineOnColor, outlineOffColor;
	outlineOnColor = findColour(MLLookAndFeel::outlineColor).overlaidWith(onColor.withMultipliedAlpha(0.625f));
	outlineOffColor = findColour(MLLookAndFeel::outlineColor);
    
    
	// geometry
	const float cornerSize = 0.;
	int flair = 0;
	bool on, down;
	Colour buttonColor, outlineColor;
		
	// draw buttons
	// TODO use ImageBank
	//
	for (int i=0; i<buttons; ++i)
	{
		on = mButtonValues[i];
		down = ((i == mButtonUnderMouse) && (mCurrDragButton >= 0));

		flair = eMLAdornShadow | eMLAdornGlow;
		if (down) flair |= (eMLAdornPressed);
		
		buttonColor = on ? onColor : offColor;
		outlineColor = on ? outlineOnColor : outlineOffColor;
		
		myLookAndFeel->drawMLButtonShape (g, mPos.getElementBounds(i),
			cornerSize, buttonColor, outlineColor, mLineThickness, flair, 0., 0.);
			
		//debug() << "elem " << i << ": " << mPos.getElementBounds(i) << "\n";
			
	}
}


#pragma mark -


void MLMultiButton::mouseDown (const MouseEvent& e)
{
    if (isEnabled())
    {		
		mCurrDragButton = getButtonUnderPoint(e.getPosition());
		if(within(mCurrDragButton, 0, (int)mButtonValues.size()))
		{
			bool val = mButtonValues[mCurrDragButton];
	//		val = !val;
			mCurrDragValue = val;
	//		setIndexedValue(mCurrDragButton, val, true, true);		
			mMovedInDrag = false;
	//		sendDragStart();
			mouseDrag(e);
			repaint();
		}
	}
}

void MLMultiButton::mouseUp (const MouseEvent&)
{
	if (isEnabled())
	{
		if(!mMovedInDrag) // no movement, toggle drag start button 
		{
			setSelectedValue(!mCurrDragValue, mCurrDragButton, true, true);
		}
		
		mCurrDragButton = -1;
		mButtonUnderMouse = -1;
		repaint();
	}
}

void MLMultiButton::modifierKeysChanged (const ModifierKeys& )
{
}

/*
void MLMultiButton::mouseMove (const MouseEvent& e)
{
	int b = getButtonUnderPoint(e.getPosition());
	if (b != mButtonUnderMouse)
	{
		mButtonUnderMouse = b;
		repaint();
	}
}

void MLMultiButton::mouseExit (const MouseEvent& )
{
	mButtonUnderMouse = -1;
	repaint();
}*/

// change state of all buttons under drag to match first button clicked, after click.
void MLMultiButton::mouseDrag(const MouseEvent& e)
{
	int b = getButtonUnderPoint(Point<int>(e.x, e.y));
	int buttons = mButtonValues.size();
	
    if (isEnabled())
    {		
//debug() << "drag:" << mx << "," << my << "dial " << s << "\n";
		if (within(b, 0, buttons))
		{
//			const int mousePos = isHorizontal() ? mx : my;
			
//debug() << "pos range: " << posRange.getLow() << ", " << posRange.getHigh() << "\n";
//debug() << "my range: " << mRange.getLow() << ", " << mRange.getHigh() << "\n";
//debug() << "pos:" << mousePos << " range:" << scaledMousePos << "\n";

			if((mCurrDragButton >= 0) && (mCurrDragButton != b)) // set buttons under drag
			{
				mMovedInDrag = true;				
		
				int span = (b - mCurrDragButton);
				int dir = sign(span);
				for(int i=mCurrDragButton; i != b + dir; i += dir)
				{
					setSelectedValue(!mCurrDragValue, i, true, true);
				}
			}

			if (b != mButtonUnderMouse)
			{
				mButtonUnderMouse = b;
				repaint();
			}
			mCurrDragButton = b;
		}	
    }
}

int MLMultiButton::getButtonUnderPoint(const Point<int>& p)
{
	return mPos.getElementUnderPoint(Vec2(p.getX(), p.getY()));
}

/*
int MLMultiButton::getButtonUnderMouse()
{
	int r = -1;	
	r = getButtonUnderPoint( localPointToGlobal ( Desktop::getMousePosition()));
	return r;
} 
*/

void MLMultiButton::setSelectedValue (float newValue, int selector, 
	const bool sendUpdateMessage,
	const bool sendMessageSynchronously)
{
	if(!within(selector, 0, (int)mButtonValues.size())) return;
    if (mButtonValues[selector] != newValue)
    {
		mButtonValues[selector] = newValue;
		mValueNeedsUpdate[selector] = true;
		repaint();
        if (sendUpdateMessage)
		{
            triggerChangeMessage (sendMessageSynchronously);
		}
    }
}

float MLMultiButton::getValue(unsigned index)
{
	float r = 0.;
	if (index < mButtonValues.size())
	{
		r = mButtonValues[index];
	}
	return r;
}

void MLMultiButton::setListener (MLMultiButton::Listener* const pL)
{
	mpListener = pL;
}
//--------------------------------------------------------------------------------
// AsyncUpdater related methods

void MLMultiButton::triggerChangeMessage (const bool synchronous)
{
    if (synchronous)
        handleAsyncUpdate();
    else
        triggerAsyncUpdate();
}
 
void MLMultiButton::handleAsyncUpdate()
{
	if(!mpListener) return;
    cancelPendingUpdate();
	int buttons = mButtonValues.size();

	for (int j=0; j<buttons; ++j)
	{
		if (mValueNeedsUpdate[j])
		{
			mpListener->multiButtonValueChanged(this, j);
			mValueNeedsUpdate[j] = false;
		}
	}
}



#pragma mark -

void MLMultiButton::resizeWidget(const MLRect& b, const int u)
{
	Component* pC = getComponent();
	mPos.setBounds(b);
	pC->setBounds(MLToJuceRectInt(mPos.getBounds()));

	int buttons = mButtonValues.size();	
	mPos.setElements(buttons);
	mPos.setGeometry(mGeometry); 
	mPos.setSizeFlags(mSizeFlags);
	mPos.setMargin(mMarginFraction);	

	mLineThickness = u/128.f;
	
	// Vec2 panelSize = mPos.getElementSize();
	
	/*
	// setup ImageBank
	mImageBank.setImages(kMLStepDisplayImages);
	mImageBank.setDims(panelSize[0], panelSize[1]);
	mImageBank.clearPanels();
	for(int i = 0; i < panels; ++i)
	{
		mImageBank.addPanel(myPos.getElementPosition(i));
	}
	*/
	
	/*
	// colors
	Colour stepOnColor (findColour (MLStepDisplay::stepOnColourId));	
	Colour stepOffColor (findColour (MLStepDisplay::stepOffColourId));	
	Colour outlineOnColor = findColour(MLLookAndFeel::outlineColor).overlaidWith(stepOnColor.withAlpha(0.5f));
	Colour outlineOffColor = findColour(MLLookAndFeel::outlineColor);
	Colour stepColor, outlineColor;
	
	// draw images to ImageBank
	for (int i=0; i<kMLStepDisplayImages; ++i)
	{
		Image& img = mImageBank.getImage(i);
		Graphics g(img);	
		
		float val = (float)i / (float)(kMLStepDisplayImages-1);
		stepColor = stepOffColor.overlaidWith(stepOnColor.withAlpha(val));
		outlineColor = outlineOffColor.overlaidWith(outlineOnColor.withAlpha(val));

		const Colour onAlphaColor = stepOnColor.withMultipliedAlpha(val);
		const Colour blinkerColor = stepOffColor.overlaidWith(onAlphaColor);
		const Colour myOutlineColor = outlineOffColor.overlaidWith(outlineOnColor.withMultipliedAlpha(val));

		const float outlineThickness = 0.75f;
		myLookAndFeel->drawMLButtonShape (g, 0, 0, panelSize[0], panelSize[1],
			r, blinkerColor, myOutlineColor, outlineThickness, eMLAdornNone, 0., 0.);	
			
	}
	*/

}




