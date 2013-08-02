
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLLabel.h"
#include "MLLookAndFeel.h"

// a Label text that is not editable.

//==============================================================================
MLLabel::MLLabel (const char* labelText) :
	mText(String::empty),
	mInverse(0), 
	mDrawImage(0), 
	mImageMode(imageModeOpaque),
	mJustification (Justification::centredTop),
	mResizeToText(true),
	mSizeMultiplier(1.0f)
{
	MLWidget::setComponent(this);
	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
		
	setOpaque(myLookAndFeel->getDefaultOpacity());
	setBufferedToImage(myLookAndFeel->getDefaultBufferMode());
	setPaintingIsUnclipped(myLookAndFeel->getDefaultUnclippedMode());
    setRepaintsOnMouseActivity (false);
	setInterceptsMouseClicks(false, false);
	setJustification(Justification::centred);
	
	mFont = myLookAndFeel->getFont(eMLCaption);
	mRichStr.setFont(mFont);
	
	if (labelText)
	{
		setText(labelText);
	}
	else 
	{
		setText("");
	}
}

MLLabel::~MLLabel()
{

}

void MLLabel::setFont (const Font& newFont)
{
	mFont = newFont;
	mRichStr.setFont(newFont);
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

void MLLabel::resized()
{
}

void MLLabel::setStringAttribute(MLSymbol , const std::string& val)
{
	setText(val.c_str());
}

void MLLabel::setDrawable (const Drawable* pD)
{
	mpDrawable = pD->createCopy();
}

void MLLabel::paint (Graphics& g)
{
	enterPaint();
	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
	int w = getWidth();
	int h = getHeight();
	
	const Colour fc (findColour (textColourId));	
	const Colour bc (findColour (backgroundColourId));	

	if (isOpaque())
		myLookAndFeel->drawBackground(g, this);
		
	if((int)getAttribute("background"))
	{
		g.setColour(bc);
		g.fillRect(getLocalBounds());	
	}
	
	// draw image
	if (mpDrawable)
	{
		mpDrawable->draw(g, 1.0f);
	}
	
	if (mText.length() > 0)
	{
		g.setColour (fc);//.withMultipliedAlpha (alpha));
		g.setFont (mFont);
		g.drawFittedText (mText, 0, 0, w, h, 
			mJustification, 2, 1.0);		
	}
	
	/*
	// TEST show bounds
	Path tbounds;
	const Rectangle<int> & boundsRect ( getLocalBounds());	
	tbounds.addRectangle(boundsRect);
	g.setColour(Colours::red);	
	g.strokePath(tbounds, PathStrokeType(0.5f));
	*/
}

void MLLabel::setText (const char* newText)
{
	mText = String(newText);
	mRichStr.setText(mText);
	repaint();
}

void MLLabel::resizeWidget(const MLRect& b, const int u)
{
	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
	float size = myLookAndFeel->getLabelTextSize() * mSizeMultiplier;
	mFont.setHeight(size);
	
	// don't use kerning for now. JUCE calculates the size wrong. 
	// mFont.setExtraKerningFactor(myLookAndFeel->getLabelTextKerning(size));
	mFont.setExtraKerningFactor(0.);
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

