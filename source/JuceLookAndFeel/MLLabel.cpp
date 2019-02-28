
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLLabel.h"
#include "MLLookAndFeel.h"
#include "MLAppView.h"

// a Label text that is not editable.

//==============================================================================
MLLabel::MLLabel (MLWidget* pContainer, const char* labelText) :
MLWidget(pContainer),
mInverse(0),
mDrawImage(0),
mImageMode(imageModeOpaque),
mJustification (Justification::centredTop),
mResizeToText(true),
mSizeMultiplier(1.0f)
{
	MLWidget::setComponent(this);
	MLLookAndFeel* myLookAndFeel = (&(getRootViewResources(this).mLookAndFeel));
	
	// labels are always opaque for better text rendering
	setOpaque(true);
	
	setBufferedToImage(false);//(myLookAndFeel->getDefaultBufferMode());
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
	////debug() << "deleting MLLabel " << getName() << "\n";
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
	MLLookAndFeel* myLookAndFeel = (&(getRootViewResources(this).mLookAndFeel));
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
	
	const ml::Text propText = getTextProperty("text");
	if (propText.lengthInBytes() > 0)
	{
		float p = getFloatProperty("padding");
		g.setColour (fc.withAlpha (alpha));
		g.setFont (mFont);
		g.drawFittedText (String(propText.getText()), p, p, w - p*2., h - p*2.,
											mJustification, 2, 1.0);
	}
}

void MLLabel::resizeWidget(const MLRect& b, const int u)
{
	const ml::Text labelText = getTextProperty("text");
	
	MLLookAndFeel* myLookAndFeel = (&(getRootViewResources(this).mLookAndFeel));
	const float size = myLookAndFeel->getLabelTextSize() * mSizeMultiplier;
	const float kern = myLookAndFeel->getLabelTextKerning(size);
	
	mFont.setHeight(size);
	mFont.setExtraKerningFactor(kern);
	mRichStr.setText(String(labelText.getText()));
	mRichStr.setJustification(mJustification);
	mRichStr.setFont(mFont);
	
	if (mResizeToText && !mpDrawable)
	{
		// get text width by creating a text layout
		TextLayout t;
		t.createLayout(mRichStr, kInfWidth);
		float tw = t.getWidth();
		tw *= 1.33f; // slop
		tw = ml::max(tw, size); // for very short texts
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

