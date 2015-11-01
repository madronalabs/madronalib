
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLLabel.h"
#include "MLLookAndFeel.h"

// a Label text that is not editable.

//==============================================================================
MLLabel::MLLabel (const char* labelText) :
	mInverse(0), 
	mDrawImage(0), 
	mImageMode(imageModeOpaque),
	mJustification (Justification::centredTop),
	mResizeToText(true),
	mSizeMultiplier(1.0f)
{
	MLWidget::setComponent(this);
	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
		
	// labels are always opaque for better text rendering
	setOpaque(true);
	
	setBufferedToImage(myLookAndFeel->getDefaultBufferMode());
	
	setPaintingIsUnclipped(myLookAndFeel->getDefaultUnclippedMode());
    setRepaintsOnMouseActivity (false);
	setInterceptsMouseClicks(false, false);
	setJustification(Justification::centred);
	
	mFont = myLookAndFeel->getFont(eMLCaption);
	
	if (labelText)
	{
		setProperty("text", labelText);
	}
}

MLLabel::~MLLabel()
{
	//debug() << "deleting MLLabel " << getName() << "\n";
}

void MLLabel::setFont (const Font& newFont)
{
	mFont = newFont;
	repaint();
}

void MLLabel::setInverse(bool i)
{
	mInverse = i;
}

void MLLabel::setImage(const Image& m)
{
	mDrawImage = true;
	mImage = m;
}

void MLLabel::setImageMode(eMLImageMode mode)
{
	mImageMode = mode;
}

void MLLabel::setJustification(const Justification& j)
{
	mJustification = j;
	mRichStr.setJustification(mJustification);
	repaint();
}

void MLLabel::setDrawable (const Drawable* pD)
{
	mpDrawable = pD->createCopy();
}

void MLLabel::paint (Graphics& g)
{
	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
	int w = getWidth();
	int h = getHeight();
	
	const Colour fc (findColour (textColourId));	
	const Colour bc (findColour (backgroundColourId));
	float alpha = isEnabled() ? 1. : 0.25f;

	myLookAndFeel->drawBackground(g, this);
	
	// we are in local coords with the origin at the widget's top left
	// myLookAndFeel->drawUnitGrid(g, this);

	// draw image
	if (mpDrawable)
	{
        // nothing special for disabled here
		mpDrawable->draw(g, 1.0f);
	}
	
	const std::string& text = getStringProperty("text");
	if (text.length() > 0)
	{
		float p = getFloatProperty("padding");
		g.setColour (fc.withAlpha (alpha));
		g.setFont (mFont);
		g.drawFittedText (String(text.c_str()), p, p, w - p*2., h - p*2.,
			mJustification, 2, 1.0);		
	}
}

void MLLabel::resizeWidget(const MLRect& b, const int u)
{
	const std::string& labelText = getStringProperty("text");

	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
	const float size = myLookAndFeel->getLabelTextSize() * mSizeMultiplier;
	const float kern = myLookAndFeel->getLabelTextKerning(size);
	
	mFont.setHeight(size);
	mFont.setExtraKerningFactor(kern);
	mRichStr.setText(String(labelText.c_str()));
	mRichStr.setJustification(mJustification);
	mRichStr.setFont(mFont);

	if (mResizeToText && !mpDrawable)
	{	
		// get text width by creating a text layout
		TextLayout t;
		t.createLayout(mRichStr, kInfWidth); 
		float tw = t.getWidth();
		tw *= 1.33f; // slop
		tw = max(tw, size); // for very short texts
		Component* pC = getComponent();
		if(pC)
		{
			MLRect bb = b;
			
			if (!mpDrawable)
			{
				//adjust to text width
				bb.stretchWidthTo(tw);
			}
			
			// adapt vrect to juce rect
			Rectangle<int> c(bb.left(), bb.top(), bb.width(), bb.height() );
			pC->setBounds(c);
		}
	}
	else
	{
		// resize image
		MLWidget::resizeWidget(b, u);
		if (mpDrawable)
		{
			Rectangle<int> imageSpace = getLocalBounds();
			mpDrawable->setTransformToFit (imageSpace.toFloat(), RectanglePlacement::centred);
		}
	}
}

