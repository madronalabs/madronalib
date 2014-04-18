
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// Portions of this software originate from JUCE, 
// copyright 2004-2013 by Raw Material Software ltd.
// JUCE is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "MLLookAndFeel.h"

MLPoint adjust(MLPoint p)
{
	float x = floor(p.x()) + 0.5f;
	float y = floor(p.y()) + 0.5f;
	return MLPoint(x, y);
}

#pragma mark -

juce_ImplementSingleton_SingleThreaded (MLLookAndFeel)

MLLookAndFeel::MLLookAndFeel()
{
	mGridUnitSize = 24.f;
	mGradientMode = 0;
	mGradientSize = 0.25f;
	mGlobalTextScale = 1.0f;
	
	// default Madrona theme (B/W)
	setColour(backgroundColorNeutral, Colour::fromHSV(0.58f, 0.0f, 0.20f, 1.f));	
	setColour(backgroundColor, Colour::fromHSV(0.5f, 0.0f, 0.75f, 1.f));
	setColour(backgroundColor2, Colour::fromHSV(0.5f, 0.00f, 0.70f, 1.f)); // edges	
	setColour(defaultFillColor, Colour::fromHSV(0.5f, 0.0f, 0.95f, 1.f));
	
	setColour(markColor, Colour::fromHSV(0.5f, 0.0f, 0.6f, 1.f));
    setColour(labelColor, Colour::fromHSV(0.5f, 0.5f, 0.0f, 1.f));
	setColour(darkLabelColor, Colour::fromHSV(0.5f, 0.f, 0.25f, 1.f));
	setColour(outlineColor, Colour::fromHSV(0.5f, 0.f, 0.0f, 1.f)); 
	setColour(shadowColor, Colour::fromHSV(0.5f, 0.f, 0.0f, 1.f));
	//
	setColour(highlightColor, Colour::fromHSV(0.58f, 0.f, 0.70f, 1.f));
	setColour(radioOffColor, Colour(0xffc0c0bc));
	setColour(radioOnColor, Colour::fromHSV(0.15f, 0.f, 0.85f, 1.f));
	setColour(buttonOffColor, Colour::fromHSV(0.5f, 0.5f, 0.80f, 1.f));
	setColour(buttonOnColor, Colour::fromHSV(0.5f, 0.5f, 0.80f, 1.f));
	//
	setColour(lineColor, findColour(labelColor).overlaidWith(findColour(backgroundColor).withAlpha(0.5f)) );
	setColour(darkFillColor, findColour(backgroundColor).overlaidWith(findColour(shadowColor).withAlpha(0.10f)));
	setColour(darkerFillColor, findColour(backgroundColor).overlaidWith(findColour(shadowColor).withAlpha(0.5f)));
	setColour(darkestFillColor, findColour(backgroundColor).overlaidWith(findColour(shadowColor).withAlpha(0.75f)));

	// reference colors, probably constant
	setColour(redColor, Colour::fromHSV(0.99f, 0.59f, 0.92f, 1.f));
	setColour(orangeColor, Colour::fromHSV(0.06f, 0.57f, 0.90f, 1.f));
	setColour(yellowColor, Colour::fromHSV(0.16f, 0.50f, 0.88f, 1.f));
	setColour(yellowGreenColor, Colour::fromHSV(0.26f, 0.42f, 0.84f, 1.f));
	setColour(greenColor, Colour::fromHSV(0.36f, 0.36f, 0.80f, 1.f));
	setColour(greenBlueColor, Colour::fromHSV(0.46f, 0.42f, 0.82f, 1.f));
	setColour(blueColor, Colour::fromHSV(0.56f, 0.55f, 0.85f, 1.f));
	setColour(indigoColor, Colour::fromHSV(0.66f, 0.47f, 0.95f, 1.f));
	setColour(violetColor, Colour::fromHSV(0.72f, 0.50f, 0.96f, 1.f));
	setColour(violetRedColor, Colour::fromHSV(0.84f, 0.55f, 0.92f, 1.f));
	setColour(whiteColor, Colour::fromHSV(0.60f, 0.10f, 0.85f, 1.f));
	setColour(grayColor, Colour::fromHSV(0.60f, 0.10f, 0.85f, 1.f));
	setColour(darkGrayColor, Colour::fromHSV(0.60f, 0.10f, 0.45f, 1.f));
	setColour(brownColor, Colour::fromHSV(0.1f, 0.30f, 0.80f, 1.f));
	setColour(darkBrownColor, Colour::fromHSV(0.1f, 0.35f, 0.65f, 1.f));

	// initialize control colors
	// Text Button colors
    setColour (MLTextButton::buttonColourId,		Colour::fromHSV(0.5f, 0.0f, 0.85f, 1.f));
    setColour (MLTextButton::textColourId,			Colour::fromHSV(0.5f, 0.0f, 0.0f, 1.f));
	
    setColour (MLLabel::backgroundColourId,			Colour::fromHSV(0.5f, 0.0f, 0.85f, 1.f));
    setColour (MLLabel::textColourId,				Colour::fromHSV(0.5f, 0.5f, 0.0f, 1.f));
	
    setColour (ListBox::outlineColourId,            Colour::fromHSV(0.5f, 0.0f, 0.20f, 1.f));
	
    setColour (ScrollBar::thumbColourId, findColour(darkFillColor));
    setColour (ScrollBar::backgroundColourId, findColour(darkerFillColor));
	
    setColour (ProgressBar::backgroundColourId,     Colours::white.withAlpha (0.6f));
    setColour (ProgressBar::foregroundColourId,     Colours::green.withAlpha (0.7f));
 
    setColour (PopupMenu::textColourId,				findColour(labelColor));
    setColour (PopupMenu::highlightedTextColourId,  findColour(MLTextButton::buttonColourId));
    setColour (PopupMenu::highlightedBackgroundColourId, findColour(labelColor));
    
	setColour (TextEditor::focusedOutlineColourId,  findColour (TextButton::buttonColourId));

    setColour (MLSeparator::backgroundColourId,    Colours::white.withAlpha (0.0f));
    setColour (MLSeparator::foregroundColourId,    findColour(markColor));

	// get typefaces from serialised binary data

	// typeface 0: Madrona Sans regular
	MemoryInputStream s1 (MLUIBinaryData::madronasans_jucefont, MLUIBinaryData::madronasans_jucefontSize, true);
	pMadronaSans = new CustomTypeface (s1);	
	
	// typeface 1: Madrona Sans italic
	MemoryInputStream s2 (MLUIBinaryData::madronasansitalic_jucefont, MLUIBinaryData::madronasansitalic_jucefontSize, false);
	pMadronaSansItalic = new CustomTypeface(s2);
	
	mTitleFont = (pMadronaSans);
	mTitleFont.setHeight(14.8);
	mTitleFont.setExtraKerningFactor (0.05);

	mCaptionFont = (pMadronaSansItalic);
	mCaptionFont.setHeight(13.25);
	mCaptionFont.setExtraKerningFactor (0.04);

	mCaptionSmallFont = (pMadronaSansItalic);
	mCaptionSmallFont.setHeight(11.25);
	mCaptionSmallFont.setExtraKerningFactor (0.05);

	mNoticeFont = (pMadronaSansItalic);
	mNoticeFont.setHeight(22.);
	mNoticeFont.setExtraKerningFactor (0.);
	
	mNumbersFont = Font("Arial", 12, Font::plain);
	mNumbersFont.setExtraKerningFactor(0.04f);

	mDrawNumbers = true;	// default
}


MLLookAndFeel::~MLLookAndFeel()
{
	clearSingletonInstance();
}

void MLLookAndFeel::setDrawNumbers(bool n)
{
	mDrawNumbers = n;
}

void MLLookAndFeel::setAnimate(bool n)
{
	mAnimate = n;
}

bool MLLookAndFeel::getAnimate()
{
	return mAnimate;
}

bool MLLookAndFeel::getDefaultOpacity() 
{ 
	return false;
}

bool MLLookAndFeel::getDefaultBufferMode() 
{ 
	return false;
}

bool MLLookAndFeel::getDefaultUnclippedMode() 
{ 
	return false;
}

static int maxDigits(const int digits, const int precision);
static int maxDigits(const int digits, const int precision)
{
	int d;
	if (precision > 0)
	{
		d = jmax(digits, precision+1);	// max digits: larger of (integer digits) and (fraction with leading zero)
	}
	else
	{
		d = digits;
	}
	return d;
}

#pragma mark -

int MLLookAndFeel::getDigitsAfterDecimal (const float number, const int digits, const int precision)  throw()
{
	int m = maxDigits(digits, precision);
	int d = ceil(log10(abs(number)+1.));
	int p;
	if (d + precision > m)
	{
		p = m-d;
	}
	else
	{
		p = precision;
	}
	
//	printf("---------number: %-+10.2f\n", number);
//	printf("---------number: %-+10.8f\n", number);
//	printf("max: %d, digits: %d, after decimal: %d\n", m, d, p);
	return p;
}

char* MLLookAndFeel::formatNumber (const float number, const int digits, const int precision, const bool doSign, MLValueDisplayMode mode)  throw()
{
	const unsigned bufLength = 16;
	static char numBuf[bufLength] = {0};
	static char format[bufLength] = {0};
	float tweakedNumber;
	
	// clear buffer
	for (unsigned i=0; i<bufLength; ++i)
	{
		numBuf[i] = format[i] = 0;
	}
	
	int m = maxDigits(digits, precision);
	int p = getDigitsAfterDecimal (number, digits, precision);

	tweakedNumber = number;
	switch(mode)
	{
		case eMLNumSeconds:
		case eMLNumHertz:
		case eMLNumDecibels:
		case eMLNumPan:
			
		case eMLNumRatio:
		{
			bool done = false;
			for(int a=1; a<=8 && !done; ++a)
			{
				for(int b=1; b<=4 && !done; ++b)
				{
					if (fabs(number - (float)a/(float)b) < 0.001)
					{
						snprintf(numBuf, bufLength, "%d/%d", a, b);
						done = true;	
					}
				}
			}
			if (!done)
			{
				if (doSign)
				{
					snprintf(format, bufLength, "X-+0%1d.%1df", m, p);
				}
				else
				{
					snprintf(format, bufLength, "X-0%1d.%1df", m, p);
				}
				format[0] = 37;	// '%'			
				snprintf(numBuf, bufLength, format, tweakedNumber);
			}
		}
		break;
		
		case eMLNumPitch:
		{
			int octave = log2(number/(27.5f - 0.01f));
			float quant = (pow(2.f, (float)octave) * 27.5f);
			float distFromOctave = fabs (number - quant);
			if (distFromOctave < 0.01)
			{			
				snprintf(format, bufLength, "X-0%1d.%1df\nA%d", m, p, octave);
			}
			else
			{
				snprintf(format,bufLength,  "X-0%1d.%1df", m, p);
			}
			format[0] = 37;	// '%'			
			sprintf(numBuf, format, tweakedNumber);		
		}
		break;
		
		case eMLNumFloat:
		case eMLNumZeroIsOff:
		default:
		{
			if (doSign)
			{
				snprintf(format, bufLength, "X-+0%1d.%1df", m, p);
			}
			else
			{
				snprintf(format, bufLength, "X-0%1d.%1df", m, p);
			}
			format[0] = 37;	// '%'			
			snprintf(numBuf, bufLength, format, tweakedNumber);
		}			
		break;
	}
	
	// special things for zero	
	if (fabs(number) < 0.00001) 
	{
		switch(mode)
		{
			case eMLNumFloat:			
				if (doSign) numBuf[0] = ' ';
			break;
			case eMLNumZeroIsOff:
				snprintf(numBuf, bufLength, "off");
			break;
			default:
			break;
		}
	}
	
	return numBuf;
}

void MLLookAndFeel::drawNumber (Graphics& g, const char* number, const int x, const int y, 
	const int w, const int h, const Colour& c, const Justification& j)  throw()
{
	Font f(mNumbersFont);
	f.setHeight(h);
	g.setFont (f);
	g.setColour(c);
	
	//TEMP TODO get actual size and position
	g.drawFittedText (number, x, y, w, h, j, 1, 1.);
	
	/*
	// TEST
	Path tbounds;
	const Rectangle<float> b (x + 0.5f, y + 0.5f, w - 1., h - 1.);	
	tbounds.addRectangle(b);
	g.setColour(Colours::red);	
	g.strokePath(tbounds, PathStrokeType(0.5f));
	*/
}

const float kSignSize = 0.65f;
const float kDigitSize = 0.57f;
const float kDotSize = 0.45f;

// get width of number in em units
//
float MLLookAndFeel::getNumberWidth (const float number, const int digits, const int precision, const bool doSign)
{
	static char temp[255] = {0};
	static char format[8] = {0};
	float lx = 0.;
	float xw = 0.;
	
	int m = maxDigits(digits, precision);
	char c;

	// calc precision
	int d = ceil(log10(abs(number)+1.));
	int p;
	if (d + precision > m)
	{
		p = m-d;
	}
	else
	{
		p = precision;
	}

	if (doSign)
	{
		sprintf(format, "X-+0%1d.%1df", m, p);
	}
	else
	{
		sprintf(format, "X-0%1d.%1df", m, p);
	}
	
	format[0] = 37;	
	sprintf(temp, format, number);
	
	for(int i=0; temp[i]; i++)
	{
		c = temp[i];
		if ((c>=48)&&(c<=57)) // draw char
		{
			xw = kDigitSize;
		}
		else if (c==46) // draw dot
		{
			xw = kDotSize;
		}
		else if (c==45) // draw minus
		{
			xw = kDigitSize;
		}
		else if (c==43) // draw plus
		{
			xw = kDigitSize*1.5f;
		}
		else // draw blank
		{
			xw = 0;
		}		
		lx += xw;
	}	
	return lx;
}



float MLLookAndFeel::calcMaxNumberWidth( const int digits, const int precision, const bool doSign)
{
	int d = 0;
	int w = maxDigits(digits, precision);
	if (precision > 0) d++; // dot
	return (w*kDigitSize) + d*kDotSize + ((float)doSign)*kSignSize;
}


#pragma mark -


//==============================================================================

void MLLookAndFeel::drawButtonGlow (Graphics& g,
                                        Button& button,
										const Colour& glowColor)
{
//	Rectangle<int> r = button.getBounds();
//	g.setColour(Colours::red);
//	g.paintRect(r);
	
	const Colour glow2Color = glowColor.withMultipliedAlpha(0.f);
	
	Path outline;
	juce::Rectangle<int> bbox = button.getLocalBounds();
	outline.addRectangle(bbox);

	float cx = bbox.getCentreX();
	float cy = bbox.getCentreY();
	ColourGradient cg (glowColor, cx, cy, glow2Color, cx, 0, true); // radial
	g.setGradientFill(cg);
	g.fillPath (outline);

}

void MLLookAndFeel::drawButtonBackground (Graphics& g,
                                        Button& button,
                                        const Colour& backgroundColour,
                                        bool , // isMouseOver
                                        bool isButtonDown,
                                        float outlineThickness)
{
	const float alpha = button.isEnabled() ? 1.f : 0.33f;
	bool down = (isButtonDown);
	
    const int width = button.getWidth();
    const int height = button.getHeight();

	const int ll = button.isConnectedOnLeft();
	const int tt = button.isConnectedOnTop();
	const int rr = button.isConnectedOnRight();
	const int bb = button.isConnectedOnBottom();

	Colour buttonColor = backgroundColour.withAlpha(alpha);
	Colour blineColor = findColour(outlineColor).withAlpha(alpha);
	
	int flair = 0;
	if (ll) flair |= (eMLAdornTopLeft | eMLAdornBottomLeft);
	if (tt) flair |= (eMLAdornTopRight | eMLAdornTopLeft);
	if (rr) flair |= (eMLAdornBottomRight | eMLAdornTopRight);
	if (bb) flair |= (eMLAdornBottomLeft | eMLAdornBottomRight);
	if (down) 
	{
		flair |= (eMLAdornPressed);
	}	
	// flair |= eMLAdornFlat;  // flat buttons are all the rage

	drawMLButtonShape (g, 0, 0, width, height,
		0, buttonColor, blineColor, outlineThickness, flair, 0., 0.);
}


void MLLookAndFeel::drawButtonText (Graphics& g, MLButton& button,
	const Colour& textColor,
    bool, bool)
{
	const float alpha = button.isEnabled() ? 1.f : 0.33f;
	g.setColour (textColor.withAlpha(alpha));
	int u = getGridUnitSize();
	int m = getSmallMargin() * u; 
	int w = button.getWidth() - m*2;
	int h = button.getHeight() - m*2;
	float textSize = getButtonTextSize(button);	
	Font f(mTitleFont);
	f.setHeight(floor(textSize) + 0.75f);
	f.setExtraKerningFactor(getButtonTextKerning(textSize));
	g.setFont(f);
	g.drawFittedText (button.getButtonText(),
                      m, m, w, h,
                      Justification::centred, 1., 1.);
}

void MLLookAndFeel::drawMenuButtonText (Graphics& g, MLButton& button,
	const Colour& textColor)
{
	// draw text
	g.setColour (textColor);
	int u = getGridUnitSize();
	int m = getSmallMargin() * u; 
	int w = button.getWidth();
	int h = button.getHeight();
	int hm = h - m*2;
	float textSize = getButtonTextSize(button);
	Font f(mTitleFont);
	f.setHeight(floor(textSize) + 0.75f);
	f.setExtraKerningFactor(getButtonTextKerning(textSize));
	g.setFont(f);
	g.drawFittedText (button.getButtonText(),
                      h/2, m, w - h, hm,
                      Justification::left, 1., 1.);
	
	// draw arrow
	Path a;
	int td = h/6;
	float txc = w - h + h/2 + 0.5f;
	float tyc = h/2 + h/8;
	a.startNewSubPath(txc, tyc);
	a.lineTo(txc - td, tyc - td);
	a.lineTo(txc + td, tyc - td);
    a.closeSubPath();
	g.fillPath(a);
}

#pragma mark -

void MLLookAndFeel::setGlobalTextScale(float s)
{
	mGlobalTextScale = s;
}

void MLLookAndFeel::setGridUnitSize(float s)
{
	mGridUnitSize = s;
}

float MLLookAndFeel::getGridUnitSize()
{
	return mGridUnitSize;
}

void MLLookAndFeel::setGridUnits(double gx, double gy)
{
	mGridUnitsX = gx;
	mGridUnitsY = gy;
}

int MLLookAndFeel::getGridUnitsX()
{
	return mGridUnitsX;
}

int MLLookAndFeel::getGridUnitsY()
{
	return mGridUnitsY;
}

// big margin for setting back components from grid edges. 
//
float MLLookAndFeel::getMargin()
{
	return 0.125f;
}

// small margin for separating parts inside components. 
//
float MLLookAndFeel::getSmallMargin()
{
	return 0.0625f;
}

// return standard label text size in grid units.
//
float MLLookAndFeel::getLabelTextSize()
{
	float t = 0.25f*mGlobalTextScale;
	t *= (float)getGridUnitSize();
	return t;
}

// return standard label height in grid units.
//
float MLLookAndFeel::getLabelHeight()
{
	float t = 0.25f*mGlobalTextScale* 1.05f;
	return t;
}

float MLLookAndFeel::getLabelTextKerning(float textSize)
{
	MLRange pointsRange(6, 24);
	MLRange trackingRange(0.03, -0.03);
	pointsRange.convertTo(trackingRange);
	return pointsRange.convertAndClip(textSize);
}

float MLLookAndFeel::getButtonTextKerning(float textSize)
{
	MLRange pointsRange(6, 24);
	MLRange trackingRange(0.03, -0.03);
	pointsRange.convertTo(trackingRange);
	return pointsRange.convertAndClip(textSize);
}

float MLLookAndFeel::getDialTextSize(const MLWidget& )
{
	return 0.225f*mGlobalTextScale;
}

float MLLookAndFeel::getButtonTextSize(const MLButton& button)
{
	const MLRect& uBounds = button.getGridBounds();
	float uh = uBounds.height();
    if(uh > 0)
    {
        uh *= 0.66f;
        uh = clamp(uh, 0.25f, 2.f);
        uh *= (float)button.getWidgetGridUnitSize();
    }
    else // hack for buttons not in grid system
    {
        uh = button.getBounds().getHeight()*0.66f;
    }
	return floor(uh);
}

float MLLookAndFeel::getToggleButtonSize()
{
	return 0.22f;
}

#pragma mark -

void MLLookAndFeel::drawTickBox (Graphics& g,
                               Component& component,
                               int x, int y, int w, int h,
                               const bool ticked,
                               const bool isEnabled,
                               const bool isMouseOverButton,
                               const bool isButtonDown)
{
    const float boxSize = w * 0.7f;

    drawGlassSphere (g, (float) x, y + (h - boxSize) * 0.5f, boxSize,
                     createMLBaseColour (component.findColour (TextButton::buttonColourId)
                                                .withMultipliedAlpha (isEnabled ? 1.0f : 0.5f),
                                       true,
                                       isMouseOverButton,
                                       isButtonDown),
                     isEnabled ? ((isButtonDown || isMouseOverButton) ? 1.1f : 0.5f) : 0.3f);

    if (ticked)
    {
        Path tick;
        tick.startNewSubPath (1.5f, 3.0f);
        tick.lineTo (3.0f, 6.0f);
        tick.lineTo (6.0f, 0.0f);

        g.setColour (isEnabled ? Colours::black : Colours::grey);

        const AffineTransform trans (AffineTransform::scale (w / 9.0f, h / 9.0f)
                                         .translated ((float) x, (float) y));

        g.strokePath (tick, PathStrokeType (2.5f), trans);
    }
}

void MLLookAndFeel::drawToggleButton (Graphics& g,
                                    ToggleButton& button,
                                    bool isMouseOverButton,
                                    bool isButtonDown)
{
    if (button.hasKeyboardFocus (true))
    {
        g.setColour (button.findColour (TextEditor::focusedOutlineColourId));
        g.drawRect (0, 0, button.getWidth(), button.getHeight());
    }

    const int tickWidth = jmin (20, button.getHeight() - 4);

    drawTickBox (g, button, 4, (button.getHeight() - tickWidth) / 2,
                 tickWidth, tickWidth,
                 button.getToggleState(),
                 button.isEnabled(),
                 isMouseOverButton,
                 isButtonDown);

    g.setColour (button.findColour (ToggleButton::textColourId));
    g.setFont (jmin (15.0f, button.getHeight() * 0.6f));

    if (! button.isEnabled())
        g.setOpacity (0.5f);

    const int textX = tickWidth + 5;

    g.drawFittedText (button.getButtonText(),
                      textX, 4,
                      button.getWidth() - textX - 2, button.getHeight() - 8,
                      Justification::centredLeft, 10);
}

void MLLookAndFeel::changeToggleButtonWidthToFitText (ToggleButton& button)
{
    Font font (jmin (15.0f, button.getHeight() * 0.6f));

    const int tickWidth = jmin (24, button.getHeight());

    button.setSize (font.getStringWidth (button.getButtonText()) + tickWidth + 8,
                    button.getHeight());
}

void MLLookAndFeel::drawProgressBar (Graphics& g, ProgressBar& progressBar,
                                            int width, int height,
                                            double progress, const String& textToShow)
{
    if (progress < 0 || progress >= 1.0)
    {
        LookAndFeel_V3::drawProgressBar (g, progressBar, width, height, progress, textToShow);
    }
    else
    {
        const Colour background (progressBar.findColour (ProgressBar::backgroundColourId));
        const Colour foreground (progressBar.findColour (ProgressBar::foregroundColourId));

    //    g.fillAll (background);
        g.setColour (foreground);

        g.fillRect (1, 1,
                    jlimit (0, width - 2, roundDoubleToInt (progress * (width - 2))),
                    height - 2);

        if (textToShow.isNotEmpty())
        {
            g.setColour (Colour::contrasting (background, foreground));
            g.setFont (height * 0.6f);

            g.drawText (textToShow, 0, 0, width, height, Justification::centred, false);
        }
    }
}

void MLLookAndFeel::drawScrollbarButton (Graphics& g,
                                       ScrollBar& scrollbar,
                                       int width, int height,
                                       int buttonDirection,
                                       bool /*isScrollbarVertical*/,
                                       bool /*isMouseOverButton*/,
                                       bool isButtonDown)
{
    Path p;

    if (buttonDirection == 0)
        p.addTriangle (width * 0.5f, height * 0.2f,
                       width * 0.1f, height * 0.7f,
                       width * 0.9f, height * 0.7f);
    else if (buttonDirection == 1)
        p.addTriangle (width * 0.8f, height * 0.5f,
                       width * 0.3f, height * 0.1f,
                       width * 0.3f, height * 0.9f);
    else if (buttonDirection == 2)
        p.addTriangle (width * 0.5f, height * 0.8f,
                       width * 0.1f, height * 0.3f,
                       width * 0.9f, height * 0.3f);
    else if (buttonDirection == 3)
        p.addTriangle (width * 0.2f, height * 0.5f,
                       width * 0.7f, height * 0.1f,
                       width * 0.7f, height * 0.9f);

    if (isButtonDown)
        g.setColour (scrollbar.findColour (ScrollBar::thumbColourId).contrasting (0.2f));
    else
        g.setColour (scrollbar.findColour (ScrollBar::thumbColourId));

    g.fillPath (p);

    g.setColour (Colour (0x80000000));
    g.strokePath (p, PathStrokeType (0.5f));
}

void MLLookAndFeel::drawScrollbar (Graphics& g,
                                 ScrollBar& scrollbar,
                                 int x, int y,
                                 int width, int height,
                                 bool isScrollbarVertical,
                                 int thumbStartPosition,
                                 int thumbSize,
                                 bool /*isMouseOver*/,
                                 bool /*isMouseDown*/)
{
 //   g.fillAll (scrollbar.findColour (ScrollBar::backgroundColourId));

    Path slotPath, thumbPath;

    const float slotIndent = jmin (width, height) > 15 ? 1.0f : 0.0f;
    const float slotIndentx2 = slotIndent * 2.0f;
    const float thumbIndent = slotIndent + 1.0f;
    const float thumbIndentx2 = thumbIndent * 2.0f;

    float gx1 = 0.0f, gy1 = 0.0f, gx2 = 0.0f, gy2 = 0.0f;

    if (isScrollbarVertical)
    {
        slotPath.addRoundedRectangle (x + slotIndent,
                                      y + slotIndent,
                                      width - slotIndentx2,
                                      height - slotIndentx2,
                                      (width - slotIndentx2) * 0.5f);

        if (thumbSize > 0)
            thumbPath.addRoundedRectangle (x + thumbIndent,
                                           thumbStartPosition + thumbIndent,
                                           width - thumbIndentx2,
                                           thumbSize - thumbIndentx2,
                                           (width - thumbIndentx2) * 0.5f);
        gx1 = (float) x;
        gx2 = x + width * 0.7f;
    }
    else
    {
        slotPath.addRoundedRectangle (x + slotIndent,
                                      y + slotIndent,
                                      width - slotIndentx2,
                                      height - slotIndentx2,
                                      (height - slotIndentx2) * 0.5f);

        if (thumbSize > 0)
            thumbPath.addRoundedRectangle (thumbStartPosition + thumbIndent,
                                           y + thumbIndent,
                                           thumbSize - thumbIndentx2,
                                           height - thumbIndentx2,
                                           (height - thumbIndentx2) * 0.5f);
        gy1 = (float) y;
        gy2 = y + height * 0.7f;
    }

    const Colour thumbColour (scrollbar.findColour (ScrollBar::thumbColourId));

    g.setGradientFill (ColourGradient (thumbColour.overlaidWith (Colour (0x44000000)), gx1, gy1,
                                       thumbColour.overlaidWith (Colour (0x19000000)), gx2, gy2, false));
    g.fillPath (slotPath);

    if (isScrollbarVertical)
    {
        gx1 = x + width * 0.6f;
        gx2 = (float) x + width;
    }
    else
    {
        gy1 = y + height * 0.6f;
        gy2 = (float) y + height;
    }

    g.setGradientFill (ColourGradient (Colours::transparentBlack,gx1, gy1,
                       Colour (0x19000000), gx2, gy2, false));
    g.fillPath (slotPath);

    g.setColour (thumbColour);
    g.fillPath (thumbPath);

    g.setGradientFill (ColourGradient (Colour (0x10000000), gx1, gy1,
                       Colours::transparentBlack, gx2, gy2, false));

    g.saveState();

    if (isScrollbarVertical)
        g.reduceClipRegion (x + width / 2, y, width, height);
    else
        g.reduceClipRegion (x, y + height / 2, width, height);

    g.fillPath (thumbPath);
    g.restoreState();

    g.setColour (Colour (0x4c000000));
    g.strokePath (thumbPath, PathStrokeType (0.4f));
}

ImageEffectFilter* MLLookAndFeel::getScrollbarEffect()
{
    return 0;
}

int MLLookAndFeel::getMinimumScrollbarThumbSize (ScrollBar& scrollbar)
{
    return jmin (scrollbar.getWidth(), scrollbar.getHeight()) * 2;
}

int MLLookAndFeel::getDefaultScrollbarWidth()
{
    return 18;
}

int MLLookAndFeel::getScrollbarButtonSize (ScrollBar& scrollbar)
{
    return 2 + (scrollbar.isVertical() ? scrollbar.getWidth()
                                       : scrollbar.getHeight());
}

//==============================================================================
void MLLookAndFeel::drawTreeviewPlusMinusBox (Graphics& g, int x, int y, int w, int h, bool isPlus, 
	bool ) // isMouseOver
{
    const int boxSize = ((jmin (16, w, h) << 1) / 3) | 1;

    x += (w - boxSize) >> 1;
    y += (h - boxSize) >> 1;
    w = boxSize;
    h = boxSize;

    g.setColour (Colour (0xe5ffffff));
    g.fillRect (x, y, w, h);

    g.setColour (Colour (0x80000000));
    g.drawRect (x, y, w, h);

    const float size = boxSize / 2 + 1.0f;
    const float centre = (float) (boxSize / 2);

    g.fillRect (x + (w - size) * 0.5f, y + centre, size, 1.0f);

    if (isPlus)
        g.fillRect (x + centre, y + (h - size) * 0.5f, 1.0f, size);
}


#pragma mark -

//==============================================================================
Font MLLookAndFeel::getPopupMenuFont()
{
	return mTitleFont;
}

void MLLookAndFeel::getIdealPopupMenuItemSize (const String& text,
                                             const bool , // isSeparator TODO
                                             int standardMenuItemHeight,
                                             int& idealWidth,
                                             int& idealHeight)
{
	/*
    if (isSeparator)
    {
        idealWidth = 50;
        idealHeight = standardMenuItemHeight > 0 ? standardMenuItemHeight / 2 : 10;
    }
    else
	*/
    {
        Font font (getPopupMenuFont());
		float h = (float)standardMenuItemHeight*kPopupMenuTextScale;
		font.setHeight(floor(h)+0.75f);
		//font.setExtraKerningFactor(getButtonTextKerning(h));
		
        idealHeight = standardMenuItemHeight > 0 ? standardMenuItemHeight : roundFloatToInt (font.getHeight() * 1.3f);
        idealWidth = font.getStringWidth (text) + idealHeight * 2;
    }
}

void MLLookAndFeel::drawPopupMenuBackground (Graphics& g, int width, int height)
{
	ColourGradient cg (findColour(MLLookAndFeel::backgroundColor2), 0, 0, 
		findColour(MLLookAndFeel::backgroundColor), 0, height, false);
	g.setGradientFill(cg);
	g.fillRect (0, 0, width, height);
}

void MLLookAndFeel::drawPopupMenuUpDownArrow (Graphics& g,
                                            int width, int height,
                                            bool isScrollUpArrow)
{
    const Colour background (findColour (PopupMenu::backgroundColourId));

    g.setGradientFill (ColourGradient (background, 0.0f, height * 0.5f,
                                       background.withAlpha (0.0f),
                                       0.0f, isScrollUpArrow ? ((float) height) : 0.0f,
                                       false));

    g.fillRect (1, 1, width - 2, height - 2);

    const float hw = width * 0.5f;
    const float arrowW = height * 0.3f;
    const float y1 = height * (isScrollUpArrow ? 0.6f : 0.3f);
    const float y2 = height * (isScrollUpArrow ? 0.3f : 0.6f);

    Path p;
    p.addTriangle (hw - arrowW, y1,
                   hw + arrowW, y1,
                   hw, y2);

    g.setColour (findColour (PopupMenu::textColourId).withAlpha (0.5f));
    g.fillPath (p);
}

void MLLookAndFeel::drawPopupMenuItem (Graphics& g, const Rectangle<int>& area,
                                        const bool isSeparator, const bool isActive,
                                        const bool isHighlighted, const bool isTicked,
                                        const bool hasSubMenu, const String& text,
                                        const String& shortcutKeyText,
                                        const Drawable* icon, const Colour* const textColourToUse)
{
    if (isSeparator)
    {
        Rectangle<int> r (area.reduced (5, 0));
        r.removeFromTop (r.getHeight() / 2 - 1);
        
        g.setColour (Colour (0x33000000));
        g.fillRect (r.removeFromTop (1));
        
        g.setColour (Colour (0x66ffffff));
        g.fillRect (r.removeFromTop (1));
    }
    else
    {
        Colour textColour (findColour (PopupMenu::textColourId));
        
        if (textColourToUse != nullptr)
            textColour = *textColourToUse;
        
        Rectangle<int> r (area.reduced (1));
        
        if (isHighlighted)
        {
            g.setColour (findColour (PopupMenu::highlightedBackgroundColourId));
            g.fillRect (r);
            
            g.setColour (findColour (PopupMenu::highlightedTextColourId));
        }
        else
        {
            g.setColour (textColour);
        }
        
        if (! isActive)
            g.setOpacity (0.3f);
        
        Font font (getPopupMenuFont());
        
        const float maxFontHeight = area.getHeight() / 1.3f;

        float fh = (float)maxFontHeight;
		font.setExtraKerningFactor(getButtonTextKerning(fh));
        
        if (font.getHeight() > maxFontHeight)
            font.setHeight (maxFontHeight);
        
        g.setFont (font);
        
        Rectangle<float> iconArea (r.removeFromLeft ((r.getHeight() * 5) / 4).reduced (3).toFloat());
        
        if (icon != nullptr)
        {
            icon->drawWithin (g, iconArea, RectanglePlacement::centred | RectanglePlacement::onlyReduceInSize, 1.0f);
        }
        else if (isTicked)
        {
            const Path tick (getTickShape (1.0f));
            g.fillPath (tick, tick.getTransformToScaleToFit (iconArea, true));
        }
        
        if (hasSubMenu)
        {
            const float arrowH = 0.6f * getPopupMenuFont().getAscent();
            
            const float x = (float) r.removeFromRight ((int) arrowH).getX();
            const float halfH = (float) r.getCentreY();
            
            Path p;
            p.addTriangle (x, halfH - arrowH * 0.5f,
                           x, halfH + arrowH * 0.5f,
                           x + arrowH * 0.6f, halfH);
            
            g.fillPath (p);
        }
        
        r.removeFromRight (3);
        g.drawFittedText (text, r, Justification::centredLeft, 1);
        
        if (shortcutKeyText.isNotEmpty())
        {
            Font f2 (font);
            f2.setHeight (f2.getHeight() * 0.75f);
            f2.setHorizontalScale (0.95f);
            g.setFont (f2);
            
            g.drawText (shortcutKeyText, r, Justification::centredRight, true);
        }
    }
}

#pragma mark -


//==============================================================================
int MLLookAndFeel::getMenuWindowFlags()
{
    return ComponentPeer::windowHasDropShadow;
}

void MLLookAndFeel::drawMenuBarBackground (Graphics& g, int width, int height,
                                         bool, MenuBarComponent& menuBar)
{
    const Colour baseColour (createMLBaseColour (menuBar.findColour (PopupMenu::backgroundColourId), false, false, false));

    if (menuBar.isEnabled())
    {
        drawMLButtonShape (g,
                              -4.0f, 0.0f,
                              width + 8.0f, (float) height,
                              0.0f,
                              baseColour,
							  findColour(outlineColor),
                              0.4f,
								0, 0., 0.);
    }
    else
    {
     //   g.fillAll (baseColour);
    }
}

Font MLLookAndFeel::getMenuBarFont (MenuBarComponent& menuBar, int /*itemIndex*/, const String& /*itemText*/)
{
    return Font (menuBar.getHeight() * 0.7f);
}

int MLLookAndFeel::getMenuBarItemWidth (MenuBarComponent& menuBar, int itemIndex, const String& itemText)
{
    return getMenuBarFont (menuBar, itemIndex, itemText)
            .getStringWidth (itemText) + menuBar.getHeight();
}

void MLLookAndFeel::drawMenuBarItem (Graphics& g,
                                   int width, int height,
                                   int itemIndex,
                                   const String& itemText,
                                   bool isMouseOverItem,
                                   bool isMenuOpen,
                                   bool /*isMouseOverBar*/,
                                   MenuBarComponent& menuBar)
{
    if (! menuBar.isEnabled())
    {
        g.setColour (menuBar.findColour (PopupMenu::textColourId)
                            .withMultipliedAlpha (0.5f));
    }
    else if (isMenuOpen || isMouseOverItem)
    {
    //    g.fillAll (menuBar.findColour (PopupMenu::highlightedBackgroundColourId));
        g.setColour (menuBar.findColour (PopupMenu::highlightedTextColourId));
    }
    else
    {
        g.setColour (menuBar.findColour (PopupMenu::textColourId));
    }

    g.setFont (getMenuBarFont (menuBar, itemIndex, itemText));
    g.drawFittedText (itemText, 0, 0, width, height, Justification::centred, 1);
}

//==============================================================================
void MLLookAndFeel::drawComboBox (Graphics& g, int width, int height,
                                const bool isButtonDown,
                                int buttonX, int buttonY,
                                int buttonW, int buttonH,
                                ComboBox& box)
{
 //   g.fillAll (box.findColour (ComboBox::backgroundColourId));

    if (box.isEnabled() && box.hasKeyboardFocus (false))
    {
        g.setColour (box.findColour (TextButton::buttonColourId));
        g.drawRect (0, 0, width, height, 2);
    }
    else
    {
        g.setColour (box.findColour (ComboBox::outlineColourId));
        g.drawRect (0, 0, width, height);
    }

    const float outlineThickness = box.isEnabled() ? (isButtonDown ? 1.2f : 0.5f) : 0.3f;

    const Colour baseColour (createMLBaseColour (box.findColour (ComboBox::buttonColourId),
                                               box.hasKeyboardFocus (true),
                                               false, isButtonDown)
                               .withMultipliedAlpha (box.isEnabled() ? 1.0f : 0.5f));

    drawGlassLozenge (g,
                      buttonX + outlineThickness, buttonY + outlineThickness,
                      buttonW - outlineThickness * 2.0f, buttonH - outlineThickness * 2.0f,
                      baseColour, outlineThickness, -1.0f,
                      true, true, true, true);

    if (box.isEnabled())
    {
        const float arrowX = 0.3f;
        const float arrowH = 0.2f;

        Path p;
        p.addTriangle (buttonX + buttonW * 0.5f,            buttonY + buttonH * (0.45f - arrowH),
                       buttonX + buttonW * (1.0f - arrowX), buttonY + buttonH * 0.45f,
                       buttonX + buttonW * arrowX,          buttonY + buttonH * 0.45f);

        p.addTriangle (buttonX + buttonW * 0.5f,            buttonY + buttonH * (0.55f + arrowH),
                       buttonX + buttonW * (1.0f - arrowX), buttonY + buttonH * 0.55f,
                       buttonX + buttonW * arrowX,          buttonY + buttonH * 0.55f);

        g.setColour (Colour (0x99000000));
        g.fillPath (p);
    }
}

Font MLLookAndFeel::getComboBoxFont (ComboBox& box)
{
     return Font (jmin (18.0f, (float)floor(box.getHeight() * 0.7f)+0.5f));
}

Label* MLLookAndFeel::createComboBoxTextBox (ComboBox&)
{
    return new Label (String::empty, String::empty);
}


#pragma mark -


//==============================================================================
								

Button* MLLookAndFeel::createDialButton (const bool isIncrement)
{
    return new TextButton (isIncrement ? "+" : "-", String::empty);
}

ImageEffectFilter* MLLookAndFeel::getDialEffect()
{
    return 0;
}

//==============================================================================

Font MLLookAndFeel::getLabelFont (Label& label)
{
    return label.getFont();
}

Font MLLookAndFeel::getTextButtonFont (TextButton& button)
{
    return button.getFont();
}

//==============================================================================
Button* MLLookAndFeel::createFilenameComponentBrowseButton (const String& text)
{
    return new TextButton (text, TRANS("click to browse for a different file"));
}

void MLLookAndFeel::layoutFilenameComponent (FilenameComponent& filenameComp,
                                           ComboBox* filenameBox,
                                           Button* browseButton)
{
    browseButton->setSize (80, filenameComp.getHeight());

    TextButton* const tb = dynamic_cast <TextButton*> (browseButton);

    if (tb != 0)
        tb->changeWidthToFitText();

    browseButton->setTopRightPosition (filenameComp.getWidth(), 0);

    filenameBox->setBounds (0, 0, browseButton->getX(), filenameComp.getHeight());
}

//==============================================================================
void MLLookAndFeel::drawCornerResizer (Graphics& g,
                                     int w, int h,
                                     bool /*isMouseOver*/,
                                     bool /*isMouseDragging*/)
{
    const float lineThickness = jmin (w, h) * 0.075f;

    for (float i = 0.0f; i < 1.0f; i += 0.3f)
    {
        g.setColour (Colours::lightgrey);

        g.drawLine (w * i,
                    h + 1.0f,
                    w + 1.0f,
                    h * i,
                    lineThickness);

        g.setColour (Colours::darkgrey);

        g.drawLine (w * i + lineThickness,
                    h + 1.0f,
                    w + 1.0f,
                    h * i + lineThickness,
                    lineThickness);
    }
 }

void MLLookAndFeel::drawResizableFrame (Graphics&, int /*w*/, int /*h*/,
                                      const BorderSize<int>& /*borders*/)
{
}

/*
//==============================================================================
void MLLookAndFeel::drawResizableWindowBorder (Graphics& g, int w, int h,
                                             const BorderSize<int>& border, ResizableWindow&)
{
	// do nothing

}
*/

void MLLookAndFeel::drawDocumentWindowTitleBar (DocumentWindow& window,
                                              Graphics& g, int w, int h,
                                              int titleSpaceX, int titleSpaceW,
                                              const Image* icon,
                                              bool drawTitleTextOnLeft)
{
    const bool isActive = window.isActiveWindow();

    g.setGradientFill (ColourGradient (window.getBackgroundColour(),
                                       0.0f, 0.0f,
                                       window.getBackgroundColour().contrasting (isActive ? 0.15f : 0.05f),
                                       0.0f, (float) h, false));
  //  g.fillAll();

    Font font (h * 0.65f, Font::bold);
    g.setFont (font);

    int textW = font.getStringWidth (window.getName());
    int iconW = 0;
    int iconH = 0;

    if (icon != 0)
    {
        iconH = (int) font.getHeight();
        iconW = icon->getWidth() * iconH / icon->getHeight() + 4;
    }

    textW = jmin (titleSpaceW, textW + iconW);
    int textX = drawTitleTextOnLeft ? titleSpaceX
                                    : jmax (titleSpaceX, (w - textW) / 2);

    if (textX + textW > titleSpaceX + titleSpaceW)
        textX = titleSpaceX + titleSpaceW - textW;

    if (icon != 0)
    {
        g.setOpacity (isActive ? 1.0f : 0.6f);
        g.drawImageWithin (*icon, textX, (h - iconH) / 2, iconW, iconH,
                           RectanglePlacement::centred, false);
        textX += iconW;
        textW -= iconW;
    }

    if (window.isColourSpecified (DocumentWindow::textColourId) || isColourSpecified (DocumentWindow::textColourId))
        g.setColour (findColour (DocumentWindow::textColourId));
    else
        g.setColour (window.getBackgroundColour().contrasting (isActive ? 0.7f : 0.4f));

    g.drawText (window.getName(), textX, 0, textW, h, Justification::centredLeft, true);
}

void MLLookAndFeel::positionDocumentWindowButtons (DocumentWindow&,
                                                 int titleBarX,
                                                 int titleBarY,
                                                 int titleBarW,
                                                 int titleBarH,
                                                 Button* minimiseButton,
                                                 Button* maximiseButton,
                                                 Button* closeButton,
                                                 bool positionTitleBarButtonsOnLeft)
{
    const int buttonW = titleBarH - titleBarH / 8;

    int x = positionTitleBarButtonsOnLeft ? titleBarX + 4
                                          : titleBarX + titleBarW - buttonW - buttonW / 4;

    if (closeButton != 0)
    {
        closeButton->setBounds (x, titleBarY, buttonW, titleBarH);
        x += positionTitleBarButtonsOnLeft ? buttonW : -(buttonW + buttonW / 4);
    }

    if (positionTitleBarButtonsOnLeft)
        swapVariables (minimiseButton, maximiseButton);

    if (maximiseButton != 0)
    {
        maximiseButton->setBounds (x, titleBarY, buttonW, titleBarH);
        x += positionTitleBarButtonsOnLeft ? buttonW : -buttonW;
    }

    if (minimiseButton != 0)
        minimiseButton->setBounds (x, titleBarY, buttonW, titleBarH);
}

int MLLookAndFeel::getDefaultMenuBarHeight()
{
    return 24;
}

//==============================================================================
void MLLookAndFeel::drawStretchableLayoutResizerBar (Graphics& g,
                                                   int w, int h,
                                                   bool /*isVerticalBar*/,
                                                   bool isMouseOver,
                                                   bool isMouseDragging)
{
    float alpha = 0.5f;

    if (isMouseOver || isMouseDragging)
    {
    //    g.fillAll (Colour (0x190000ff));
        alpha = 1.0f;
    }

    const float cx = w * 0.5f;
    const float cy = h * 0.5f;
    const float cr = jmin (w, h) * 0.4f;

    g.setGradientFill (ColourGradient (Colours::white.withAlpha (alpha), cx + cr * 0.1f, cy + cr,
                                       Colours::black.withAlpha (alpha), cx, cy - cr * 4.0f,
                                       true));

    g.fillEllipse (cx - cr, cy - cr, cr * 2.0f, cr * 2.0f);
}

//==============================================================================
void MLLookAndFeel::drawGroupComponentOutline (Graphics& g, int width, int height,
                                             const String& text,
                                             const Justification& position,
                                             GroupComponent& group)
{
    const float textH = 15.0f;
    const float indent = 3.0f;
    const float textEdgeGap = 4.0f;
    float cs = 5.0f;

    Font f (textH);

    Path p;
    float x = indent;
    float y = f.getAscent() - 3.0f;
    float w = jmax (0.0f, width - x * 2.0f);
    float h = jmax (0.0f, height - y  - indent);
    cs = jmin (cs, w * 0.5f, h * 0.5f);
    const float cs2 = 2.0f * cs;

    float textW = text.isEmpty() ? 0 : jlimit (0.0f, jmax (0.0f, w - cs2 - textEdgeGap * 2), f.getStringWidth (text) + textEdgeGap * 2.0f);
    float textX = cs + textEdgeGap;

    if (position.testFlags (Justification::horizontallyCentred))
        textX = cs + (w - cs2 - textW) * 0.5f;
    else if (position.testFlags (Justification::right))
        textX = w - cs - textW - textEdgeGap;

    p.startNewSubPath (x + textX + textW, y);
    p.lineTo (x + w - cs, y);

    p.addArc (x + w - cs2, y, cs2, cs2, 0, float_Pi * 0.5f);
    p.lineTo (x + w, y + h - cs);

    p.addArc (x + w - cs2, y + h - cs2, cs2, cs2, float_Pi * 0.5f, float_Pi);
    p.lineTo (x + cs, y + h);

    p.addArc (x, y + h - cs2, cs2, cs2, float_Pi, float_Pi * 1.5f);
    p.lineTo (x, y + cs);

    p.addArc (x, y, cs2, cs2, float_Pi * 1.5f, float_Pi * 2.0f);
    p.lineTo (x + textX, y);

    const float alpha = group.isEnabled() ? 1.0f : 0.5f;

    g.setColour (group.findColour (GroupComponent::outlineColourId)
                    .withMultipliedAlpha (alpha));

    g.strokePath (p, PathStrokeType (2.0f));

    g.setColour (group.findColour (GroupComponent::textColourId)
                    .withMultipliedAlpha (alpha));
    g.setFont (f);
    g.drawText (text,
                roundFloatToInt (x + textX), 0,
                roundFloatToInt (textW),
                roundFloatToInt (textH),
                Justification::centred, true);
}

//==============================================================================
void MLLookAndFeel::paintToolbarBackground (Graphics& g, int w, int h, Toolbar& toolbar)
{
    const Colour background (toolbar.findColour (Toolbar::backgroundColourId));

    g.setGradientFill (ColourGradient (background, 0.0f, 0.0f,
                                       background.darker (0.1f),
                                       toolbar.isVertical() ? w - 1.0f : 0.0f,
                                       toolbar.isVertical() ? 0.0f : h - 1.0f,
                                       false));
 //   g.fillAll();

}

Button* MLLookAndFeel::createToolbarMissingItemsButton (Toolbar& /*toolbar*/)
{
    return createTabBarExtrasButton();
}

void MLLookAndFeel::paintToolbarButtonBackground (Graphics& , int /*width*/, int /*height*/,
                                                bool , bool ,
                                                ToolbarItemComponent& )
{
	/*
    if (isMouseDown)
        g.fillAll (component.findColour (Toolbar::buttonMouseDownBackgroundColourId, true));
    else if (isMouseOver)
        g.fillAll (component.findColour (Toolbar::buttonMouseOverBackgroundColourId, true));
	*/
}

void MLLookAndFeel::paintToolbarButtonLabel (Graphics& g, int x, int y, int width, int height,
                                           const String& text, ToolbarItemComponent& component)
{
    g.setColour (component.findColour (Toolbar::labelTextColourId, true)
                    .withAlpha (component.isEnabled() ? 1.0f : 0.25f));

    const float fontHeight = jmin (14.0f, height * 0.85f);
    g.setFont (fontHeight);

    g.drawFittedText (text,
                      x, y, width, height,
                      Justification::centred,
                      jmax (1, height / (int) fontHeight));
}

/*
//==============================================================================
void MLLookAndFeel::createFileChooserHeaderText (const String& title,
                                               const String& instructions,
                                               GlyphArrangement& text,
                                               int width)
{
    text.clear();

    text.addJustifiedText (Font (14.0f), title,
                           8.0f, 22.0f, width - 16.0f,
                           Justification::centred);

    text.addJustifiedText (Font (14.0f), instructions,
                           8.0f, 24.0f + 16.0f, width - 16.0f,
                           Justification::centred);
}
*/

Button* MLLookAndFeel::createFileBrowserGoUpButton()
{
    MLDrawableButton* goUpButton = new MLDrawableButton ("up", MLDrawableButton::ImageOnButtonBackground);

    Path arrowPath;
    arrowPath.addArrow (Line<float>(50.0f, 100.0f, 50.0f, 0.0f), 40.0f, 100.0f, 50.0f);

    DrawablePath arrowImage;
    arrowImage.setFill (Colours::black.withAlpha (0.4f));
    arrowImage.setPath (arrowPath);

    goUpButton->setImage (&arrowImage);

    return goUpButton;
}

void MLLookAndFeel::layoutFileBrowserComponent (FileBrowserComponent& browserComp,
                                              DirectoryContentsDisplayComponent* fileListComponent,
                                              FilePreviewComponent* previewComp,
                                              ComboBox* currentPathBox,
                                              TextEditor* filenameBox,
                                              Button* goUpButton)
{
    const int x = 8;
    int w = browserComp.getWidth() - x - x;

    if (previewComp != 0)
    {
        const int previewWidth = w / 3;
        previewComp->setBounds (x + w - previewWidth, 0, previewWidth, browserComp.getHeight());

        w -= previewWidth + 4;
    }

    int y = 4;

    const int controlsHeight = 22;
    const int bottomSectionHeight = controlsHeight + 8;
    const int upButtonWidth = 50;

    currentPathBox->setBounds (x, y, w - upButtonWidth - 6, controlsHeight);
    goUpButton->setBounds (x + w - upButtonWidth, y, upButtonWidth, controlsHeight);

    y += controlsHeight + 4;

    Component* const listAsComp = dynamic_cast <Component*> (fileListComponent);
    listAsComp->setBounds (x, y, w, browserComp.getHeight() - y - bottomSectionHeight);

    y = listAsComp->getBottom() + 4;
    filenameBox->setBounds (x + 50, y, w - 50, controlsHeight);
}



//==============================================================================
static void createRoundedPath (Path& p,
                               const float x, const float y,
                               const float w, const float h,
                               const float cs,
                               const bool curveTopLeft, const bool curveTopRight,
                               const bool curveBottomLeft, const bool curveBottomRight) throw()
{
    const float cs2 = 2.0f * cs;

    if (curveTopLeft)
    {
        p.startNewSubPath (x, y + cs);
        p.addArc (x, y, cs2, cs2, float_Pi * 1.5f, float_Pi * 2.0f);
    }
    else
    {
        p.startNewSubPath (x, y);
    }

    if (curveTopRight)
    {
        p.lineTo (x + w - cs, y);
        p.addArc (x + w - cs2, y, cs2, cs2, 0.0f, float_Pi * 0.5f);
    }
    else
    {
        p.lineTo (x + w, y);
    }

    if (curveBottomRight)
    {
        p.lineTo (x + w, y + h - cs);
        p.addArc (x + w - cs2, y + h - cs2, cs2, cs2, float_Pi * 0.5f, float_Pi);
    }
    else
    {
        p.lineTo (x + w, y + h);
    }

    if (curveBottomLeft)
    {
        p.lineTo (x + cs, y + h);
        p.addArc (x, y + h - cs2, cs2, cs2, float_Pi, float_Pi * 1.5f);
    }
    else
    {
        p.lineTo (x, y + h);
    }

    p.closeSubPath();
}


static inline float distance(const float x1, const float y1, const float x2, const float y2);
static inline float distance(const float x1, const float y1, const float x2, const float y2)
{
	return (sqrt((x2 - x1)*(x2 - x1) + (y2 - y1)*(y2 - y1)));
}

static void spikyPathTo(Path& p, const float x2, const float y2, const int doSpike,  float sx,  float sy);
static void spikyPathTo(Path& p, const float x2, const float y2, const int doSpike,  float sx,  float sy)
{
	if (doSpike)
	{
		const MLPoint prev = floatPointToInt(p.getCurrentPosition());
		const float x1 = prev.x();
		const float y1 = prev.y();
	
		const float x3 = (x1 + x2) / 2.;
		const float y3 = (y1 + y2) / 2.;
		
		sx += 0.5f;
		sy -= 0.5f;
		
		float baseX, baseY;		
		
		// set half width of spike base
		baseX = min((double)abs(y3 - sy), abs(x2 - x1) / 2.);
		baseY = min((double)abs(x3 - sx), abs(y2 - y1) / 2.);
		if (x2 > x1) baseX *= -1;
		if (y2 > y1) baseY *= -1;
		baseX = floor(baseX);
		baseY = floor(baseY);
		
		p.lineTo(floor(x3 + baseX) + 0.5, floor(y3 + baseY) + 0.5);
		p.lineTo(sx, sy);
		p.lineTo(floor(x3 - baseX) + 0.5, floor(y3 - baseY) + 0.5);
		p.lineTo(x2, y2);
								
	}
	else
	{
		p.lineTo(x2, y2);
	}
}

 
#pragma mark -
//==============================================================================


// create a rectangular path with optional rounded corners and spikes. 
void MLLookAndFeel::createMLRectangle (Path& p,
                               const float x, const float y,
                               const float w, const float h,
                               const float cs,
                               const unsigned int flair, 
							   const float sx, 
							   const float sy,
							   const bool isOutline)  throw()
{
	const unsigned int curveTopLeft = !(flair & eMLAdornTopLeft);
	const unsigned int curveTopRight = !(flair & eMLAdornTopRight);
	const unsigned int curveBottomRight = !(flair & eMLAdornBottomRight);
	const unsigned int curveBottomLeft = !(flair & eMLAdornBottomLeft);
	const unsigned int spikeLeft = flair & eMLAdornLeft;
	const unsigned int spikeTop = flair & eMLAdornTop;
	const unsigned int spikeRight = flair & eMLAdornRight;
	const unsigned int spikeBottom = flair & eMLAdornBottom;
	
	float ix = floor(x);
	float iy = floor(y);
	float iw = floor(w);
	float ih = floor(h);
    const float cs2 = 2.0f*cs;
	const float cs3 = cs2;
    
	if (isOutline)
	{
		ix += 0.5f;
		iy += 0.5f;
		iw -= 1.0f;
		ih -= 1.0f;
	}
    if (curveTopLeft)
    {
        p.startNewSubPath (ix, iy + cs);
        p.addArc (ix, iy, cs2, cs2, float_Pi * 1.5f, float_Pi * 2.0f);
    }
    else
    {
        p.startNewSubPath (ix, iy);
    }

    if (curveTopRight)
    {
		spikyPathTo (p, ix + iw - cs, iy, spikeTop, sx, sy);
        p.addArc (ix + iw - cs2, iy, cs2, cs2, 0.0f, float_Pi * 0.5f);
    }
    else
    {
        spikyPathTo (p, ix + iw, iy, spikeTop, sx, sy);
    }

    if (curveBottomRight)
    {
        spikyPathTo (p, ix + iw, iy + ih - cs, spikeRight, sx, sy);
        p.addArc (ix + iw - cs3, iy + ih - cs3, cs3, cs3, float_Pi * 0.5f, float_Pi);
    }
    else
    {
        spikyPathTo (p, ix + iw, iy + ih, spikeRight, sx, sy);
    }

    if (curveBottomLeft)
    {
        spikyPathTo (p, ix + cs, iy + ih, spikeBottom, sx, sy);
        p.addArc (ix, iy + ih - cs3, cs3, cs3, float_Pi, float_Pi * 1.5f);
    }
    else
    {
        spikyPathTo (p, ix, iy + ih, spikeBottom, sx, sy);
    }

	if (spikeLeft)
	{
		spikyPathTo (p, ix, iy, 1, sx, sy);
		p.closeSubPath();
	}
	else
	{
		p.closeSubPath();
	}
}

//==============================================================================
void MLLookAndFeel::drawShadowLine  (Graphics& g,
                                        float ax, float ay, float bx, float by,
                                        const Colour& color,
                                        const int width)
{										
	// determine direction of shadow using clockwise rule
	const float dx = bx - ax;
	const float dy = by - ay;
	const float m = sqrt(dx*dx + dy*dy);
	const float vx = -dy / m;
	const float vy = dx / m;
	const float oneOverWidth = 1.0 / width;
	float pax, pay, pbx, pby;
	
	Path outline;	
	float d, opacity;
	pax = ax;
	pay = ay;
	pbx = bx;
	pby = by;
	for (int i=0; i<width; i++)
	{
		outline.clear();			
		outline.startNewSubPath(pax, pay);
		outline.lineTo(pbx, pby);
		d = ((float)(width - i)*oneOverWidth); // [0. - 1.]
		opacity = d*d*kMLShadowOpacity;
		g.setColour (color.withAlpha(opacity));
		g.strokePath (outline, PathStrokeType (1.));	
		pax += vx;
		pay += vy;
		pbx += vx;
		pby += vy;
	}
}


#pragma mark -
//==============================================================================
	

void MLLookAndFeel::setBackgroundGradient(Graphics& g, Point<int>& gStart, Point<int>& gEnd)
{
	Colour c1 = findColour (MLLookAndFeel::backgroundColor2);
	Colour c2 = findColour (MLLookAndFeel::backgroundColor);
	
	if (mGradientMode < 2)
	{
		ColourGradient cg;
		cg.point1 = Point<float>(0, gStart.getY());
		cg.point2 = Point<float>(0, gEnd.getY());
		cg.isRadial = false;
		switch(mGradientMode)
		{
			case 0:		
			default:			
				cg.addColour(0., c1);
				cg.addColour(1., c2);
			break;
			case 1:
				cg.addColour(0., c1);
				cg.addColour(mGradientSize, c2);
				cg.addColour(1. - mGradientSize, c2);
				cg.addColour(1., c1);
			break;
		}
		g.setGradientFill(cg);
	}
	else if (mGradientMode == 2)
	{
		g.setColour(c2);
	}
}

// draw a background gradient over the component's rect, positioned
// so that backgrounds of different components will match each other
// without seams. 
//
void MLLookAndFeel::drawBackground(Graphics& g, Component* pC)
{
	const Rectangle<int> lb = pC->getLocalBounds();
	
	// get screen coords of gradient
	Component* wc = pC->getTopLevelComponent();
	Point<int> windowPos = wc->getScreenPosition();
	Point<int> compPosInWindow = pC->getScreenPosition() - windowPos;
	Point<int> gStart = Point<int>(0 ,0) - compPosInWindow;
	Point<int> gEnd = Point<int>(wc->getWidth(), wc->getHeight()) - compPosInWindow;
	
	setBackgroundGradient(g, gStart, gEnd);
	g.fillRect (lb);
}

void MLLookAndFeel::drawBackgroundRect(Graphics& g, Component* pC, MLRect r)
{
	const Rectangle<int> lb = MLToJuceRectInt(r);

	// get screen coords of gradient
	Component* wc = pC->getTopLevelComponent();
	Point<int> windowPos = wc->getScreenPosition();
	Point<int> compPosInWindow = pC->getScreenPosition() - windowPos;
	Point<int> gStart = Point<int>(0 ,0) - compPosInWindow;
	Point<int> gEnd = Point<int>(wc->getWidth(), wc->getHeight()) - compPosInWindow;
	
	setBackgroundGradient(g, gStart, gEnd);
	g.fillRect (lb);
}

// unit grid for testing
void MLLookAndFeel::drawUnitGrid(Graphics& g)
{
	int u = getGridUnitSize(); 
	Path p, q;
	p.addRectangle(0, 0, u, u);
	g.setColour(Colours::navy.withAlpha(0.25f));
	for(int i=0; i<mGridUnitsX; ++i)
	{
		for(int j=0; j<mGridUnitsY; ++j)
		{
			q = p;
			q.applyTransform (AffineTransform::translation (u*i, u*j));
			g.strokePath(q, PathStrokeType (1.f));	
		}
	}
}


//==============================================================================
void MLLookAndFeel::drawMLButtonShape  (Graphics& g,
                                        const MLRect& r,
                                        float maxCornerSize,
                                        const Colour& baseColor,
                                        const Colour& myOutlineColor,
                                        const float strokeWidth,
                                        const unsigned flair,
										const float sx, 
										const float sy) throw()
{
	drawMLButtonShape(g, r.left(), r.top(), r.width(), r.height(),
										maxCornerSize,
                                        baseColor,
                                        myOutlineColor,
                                        strokeWidth,
                                        flair,
										sx, 
										sy);
}


void MLLookAndFeel::drawMLButtonShape  (Graphics& g,
                                        float x, float y, float w, float h,
                                        float maxCornerSize,
                                        const Colour& baseColor,
                                        const Colour& myOutlineColor,
                                        const float strokeWidth,
                                        const unsigned flair,
										const float sx, 
										const float sy) throw()
{
	if (h <= 0. || w <= 0.) return;

	const float sat = baseColor.getSaturation();
	const float b = baseColor.getBrightness();
	const float a = baseColor.getFloatAlpha();
		
 //	const Colour baseOpaque = baseColor.withAlpha(1.f);
 	const Colour whiteAlpha = Colours::white.withAlpha(a);
 	const Colour blackAlpha = Colours::black.withAlpha(a);
// 	const Colour darkest = baseColor.withSaturation(0.99).withBrightness(0.25);
 	const Colour darkest = baseColor.withSaturation(min(1., sat*2.)).withBrightness(max(0., b-0.5));
	
	const Colour light0Color = baseColor.overlaidWith(whiteAlpha.withMultipliedAlpha(0.1f));
	const Colour light1Color = baseColor.overlaidWith(whiteAlpha.withMultipliedAlpha(0.2f));
	const Colour dark0Color = baseColor.overlaidWith(darkest.withMultipliedAlpha(0.25f));
	const Colour dark1Color = baseColor.overlaidWith(darkest.withMultipliedAlpha(0.35f));

	const Colour glow1Color = (baseColor.overlaidWith(whiteAlpha.withAlpha(0.5f))).withMultipliedAlpha(0.25f);
	const Colour glow2Color = baseColor.withMultipliedAlpha(0.5f);
	const Colour glow3Color = myOutlineColor.withMultipliedAlpha(0.5f);
	
	const bool pressed = flair & eMLAdornPressed;
	const bool glow = flair & eMLAdornGlow;
	const bool flat = flair & eMLAdornFlat;
	const float gradPixelY = (1. / h);
	const unsigned gradPixels = 2;
	float gradWidthTop, gradWidthBottom;
	gradWidthTop = gradPixels * gradPixelY;
	gradWidthTop = clamp(gradWidthTop, 0.125f, 1.f);
	gradWidthBottom = gradWidthTop;
	
    Path outline;
    createMLRectangle (outline, x, y, w, h, maxCornerSize, flair, sx, sy, true);
    
//    outline.addRectangle(x, y, w, h);
    
    /*
    Path xHairs;
    xHairs.startNewSubPath(x, y + h/2);
    xHairs.lineTo(x + w, y + h/2);
    xHairs.startNewSubPath(x + w/2, y);
    xHairs.lineTo(x + w/2, y + h);
	*/
    
//    Path fill;
//    createMLRectangle (fill, x, y, w, h, maxCornerSize, flair, sx, sy, false);
	
	Colour c1, c2, c3, c4;
	Colour cg1, cg2;
	
	if (pressed)
	{
		c1 = dark0Color;
		c2 = dark0Color;
		c3 = dark1Color;
		c4 = dark1Color;
	}
	else
	{
		c1 = baseColor;
		c2 = baseColor;
		c3 = baseColor;
		c4 = baseColor;
	}
		
	if (flat)
	{
		c1 = c3 = c4 = c2;
	}

	/*
	// light outline anti-shadow
	if ((flair & eMLAdornShadow) && (strokeWidth > 0.05f))
	{
		g.setColour (Colours::white.withMultipliedAlpha(a*0.5f));
		g.saveState();
		g.setOrigin(0., 0.5);
		g.strokePath (outline, PathStrokeType (strokeWidth));
		g.restoreState();
	}
	*/

	// draw dark fill vert grad
	{
		ColourGradient cg (c1, 0, y, c4, 0, y + h + 1, false);
		if (gradWidthTop > 0)
			cg.addColour (gradWidthTop, c2);
		if (gradWidthBottom < 1.)
			cg.addColour (1. - gradWidthBottom, c3);
		g.setGradientFill(cg);
		g.fillPath (outline); 
	}

	// shadow 
	if (pressed)
	{	
		Path outline2;
		int shadowPixels = min(kMLShadowThickness*2.f, w/4);
		int thin = shadowPixels*0.25;

		float d, opacity;
		float oneOverWidth = 1.f / (float)shadowPixels;

		g.saveState();
		g.reduceClipRegion (outline);		
		for (int i=0; i<shadowPixels; i++)
		{
			createMLRectangle (outline2, x+i-thin, y+i, w-i*2+thin*2, h*2, maxCornerSize+i, flair, sx, sy, true); 
			d = (float)(shadowPixels - i) * oneOverWidth; // 0. - 1.
			opacity = d * d * d * kMLShadowOpacity;
			g.setColour (darkest.withAlpha(opacity));
			g.strokePath (outline2, PathStrokeType (2.f));	
		}
		g.restoreState();
	}		
	
	if (glow)
	{
		Rectangle<float> bbox = outline.getBounds();
		float cx = bbox.getCentreX();
		float cy = bbox.getCentreY();
		ColourGradient cg (glow1Color, cx, cy, glow3Color, x, y, true); // radial
		cg.addColour (0.75, glow2Color);
		g.setGradientFill(cg);
		g.fillPath (outline);
	}

	// draw outline
	if (strokeWidth > 0.05f)
	{
		g.setColour (myOutlineColor);
		g.strokePath (outline, PathStrokeType (strokeWidth));
//		g.strokePath (xHairs, PathStrokeType (strokeWidth)); // TEST
	}

    
}

// --------------------------------------------------------------------------------
#pragma mark -
#pragma mark alerts
//

AlertWindow* MLLookAndFeel::createAlertWindow (const String& title,
                                             const String& message,
                                             const String& button1,
                                             const String& button2,
                                             const String& button3,
                                             AlertWindow::AlertIconType iconType,
                                             int numButtons,
                                             Component* associatedComponent)
{
    AlertWindow* aw = new AlertWindow (title, message, iconType, associatedComponent);

    if (numButtons == 1)
    {
        aw->addButton (button1, 0,
                       KeyPress (KeyPress::escapeKey, 0, 0),
                       KeyPress (KeyPress::returnKey, 0, 0));
    }
    else
    {
        const KeyPress button1ShortCut (CharacterFunctions::toLowerCase (button1[0]), 0, 0);
        KeyPress button2ShortCut (CharacterFunctions::toLowerCase (button2[0]), 0, 0);
        if (button1ShortCut == button2ShortCut)
            button2ShortCut = KeyPress();

        if (numButtons == 2)
        {
            aw->addButton (button1, 1, KeyPress (KeyPress::returnKey, 0, 0), button1ShortCut);
            aw->addButton (button2, 0, KeyPress (KeyPress::escapeKey, 0, 0), button2ShortCut);
        }
        else if (numButtons == 3)
        {
            aw->addButton (button1, 1, button1ShortCut);
            aw->addButton (button2, 2, button2ShortCut);
            aw->addButton (button3, 0, KeyPress (KeyPress::escapeKey, 0, 0));
        }
    }
	
	if (associatedComponent)
	{
		int h1 = associatedComponent->getHeight();
		int h2 = aw->getHeight();
		Point<int> pos = aw->getPosition();
		aw->setTopLeftPosition(pos.getX(), pos.getY() + (h1 + h2)/2);															
	}
	
//	aw->setHeight(aw->getHeight() - 50);
	
	ComponentPeer* p = aw->getPeer();
	p->setAlwaysOnTop(true);

    return aw;
}

/*
void MLLookAndFeel::drawAlertBox (Graphics& g,
                                AlertWindow& alert,
                                const Rectangle<int>& textArea,
                                TextLayout& )
{
    const int iconWidth = 80;

    const Colour background (alert.findColour (MLLookAndFeel::backgroundColor));
    const Colour background2 (alert.findColour (MLLookAndFeel::backgroundColor2));
	int height = alert.getHeight();
	int width = alert.getWidth();
	ColourGradient cg (background, 0, 0, background2, 0, height, false);
	g.setGradientFill(cg);
	g.fillRect (0, 0, width, height);

    Justification alignment (Justification::horizontallyCentred);

    int iconSize = jmin (iconWidth + 50, alert.getHeight() + 20);

    if (alert.containsAnyExtraComponents() || alert.getNumButtons() > 2)
        iconSize = jmin (iconSize, textArea.getHeight() + 50);

    const MLRect iconRect (iconSize / -10,
                              iconSize / -10,
                              iconSize,
                              iconSize);

	g.setColour (alert.findColour (AlertWindow::textColourId));

	
    textLayout.drawWithin (g,
                           textArea.x() + iconSpaceUsed,
                           textArea.y(),
                           textArea.getWidth() - iconSpaceUsed,
                           textArea.getHeight(),
                           alignment.getFlags() | Justification::top);
	
	
    g.setColour (alert.findColour (AlertWindow::outlineColourId));
    g.drawRect (0, 0, alert.getWidth(), alert.getHeight());
}

*/

int MLLookAndFeel::getAlertBoxWindowFlags()
{
    return ComponentPeer::windowAppearsOnTaskbar
            | ComponentPeer::windowHasDropShadow 
			| ComponentPeer::windowIsTemporary;
}

int MLLookAndFeel::getAlertWindowButtonHeight()
{
    return 20;
}

// --------------------------------------------------------------------------------
#pragma mark -
//


//==============================================================================
void MLLookAndFeel::drawGlassSphere (Graphics& g,
                                   const float x, const float y,
                                   const float diameter,
                                   const Colour& colour,
                                   const float outlineThickness) throw()
{
    if (diameter <= outlineThickness)
        return;

    Path p;
    p.addEllipse (x, y, diameter, diameter);

    {
        ColourGradient cg (Colours::white.overlaidWith (colour.withMultipliedAlpha (0.3f)), 0, y,
                           Colours::white.overlaidWith (colour.withMultipliedAlpha (0.3f)), 0, y + diameter, false);

        cg.addColour (0.4, Colours::white.overlaidWith (colour));

		g.setGradientFill(cg);

        g.fillPath (p);
    }

    {
		g.setGradientFill (ColourGradient (Colours::white, 0, y + diameter * 0.06f,
                                       Colours::transparentWhite, 0, y + diameter * 0.3f, false));
        g.fillEllipse (x + diameter * 0.2f, y + diameter * 0.05f, diameter * 0.6f, diameter * 0.4f);
    }

    {
        ColourGradient cg (Colours::transparentBlack,
                           x + diameter * 0.5f, y + diameter * 0.5f,
                           Colours::black.withAlpha (0.5f * outlineThickness * colour.getFloatAlpha()),
                           x, y + diameter * 0.5f, true);

        cg.addColour (0.7, Colours::transparentBlack);
        cg.addColour (0.8, Colours::black.withAlpha (0.1f * outlineThickness));

		g.setGradientFill(cg);
        g.fillPath (p);
    }

    g.setColour (Colours::black.withAlpha (0.5f * colour.getFloatAlpha()));
    g.drawEllipse (x, y, diameter, diameter, outlineThickness);
}

//==============================================================================
void MLLookAndFeel::drawGlassPointer (Graphics& g,
                                    const float x, const float y,
                                    const float diameter,
                                    const Colour& colour, const float outlineThickness,
                                    const int direction) throw()
{
    if (diameter <= outlineThickness)
        return;

    Path p;
    p.startNewSubPath (x + diameter * 0.5f, y);
    p.lineTo (x + diameter, y + diameter * 0.6f);
    p.lineTo (x + diameter, y + diameter);
    p.lineTo (x, y + diameter);
    p.lineTo (x, y + diameter * 0.6f);
    p.closeSubPath();

    p.applyTransform (AffineTransform::rotation (direction * (float_Pi * 0.5f), x + diameter * 0.5f, y + diameter * 0.5f));

    {
        ColourGradient cg (Colours::white.overlaidWith (colour.withMultipliedAlpha (0.3f)), 0, y,
                           Colours::white.overlaidWith (colour.withMultipliedAlpha (0.3f)), 0, y + diameter, false);

        cg.addColour (0.4, Colours::white.overlaidWith (colour));

		g.setGradientFill(cg);
        g.fillPath (p);
    }

    {
        ColourGradient cg (Colours::transparentBlack,
                           x + diameter * 0.5f, y + diameter * 0.5f,
                           Colours::black.withAlpha (0.5f * outlineThickness * colour.getFloatAlpha()),
                           x - diameter * 0.2f, y + diameter * 0.5f, true);

        cg.addColour (0.5, Colours::transparentBlack);
        cg.addColour (0.7, Colours::black.withAlpha (0.07f * outlineThickness));

		g.setGradientFill(cg);
        g.fillPath (p);
    }

    g.setColour (Colours::black.withAlpha (0.5f * colour.getFloatAlpha()));
    g.strokePath (p, PathStrokeType (outlineThickness));
}

//==============================================================================
void MLLookAndFeel::drawGlassLozenge (Graphics& g,
                                    const float x, const float y,
                                    const float width, const float height,
                                    const Colour& colour,
                                    const float outlineThickness,
                                    const float cornerSize,
                                    const bool flatOnLeft,
                                    const bool flatOnRight,
                                    const bool flatOnTop,
                                    const bool flatOnBottom) throw()
{
    if (width <= outlineThickness || height <= outlineThickness)
        return;

    const int intX = (int) x;
    const int intY = (int) y;
    const int intW = (int) width;
    const int intH = (int) height;

    const float cs = cornerSize < 0 ? jmin (width * 0.5f, height * 0.5f) : cornerSize;
    const float edgeBlurRadius = height * 0.75f + (height - cs * 2.0f);
    const int intEdge = (int) edgeBlurRadius;

    Path outline;
    createRoundedPath (outline, x, y, width, height, cs,
                        ! (flatOnLeft || flatOnTop),
                        ! (flatOnRight || flatOnTop),
                        ! (flatOnLeft || flatOnBottom),
                        ! (flatOnRight || flatOnBottom));

    {
        ColourGradient cg (colour.darker (0.2f), 0, y,
                           colour.darker (0.2f), 0, y + height, false);

        cg.addColour (0.03, colour.withMultipliedAlpha (0.3f));
        cg.addColour (0.4, colour);
        cg.addColour (0.97, colour.withMultipliedAlpha (0.3f));

		g.setGradientFill(cg);
        g.fillPath (outline);
    }

    ColourGradient cg (Colours::transparentBlack, x + edgeBlurRadius, y + height * 0.5f,
                       colour.darker (0.2f), x, y + height * 0.5f, true);

    cg.addColour (jlimit (0.0, 1.0, 1.0 - (cs * 0.5f) / edgeBlurRadius), Colours::transparentBlack);
    cg.addColour (jlimit (0.0, 1.0, 1.0 - (cs * 0.25f) / edgeBlurRadius), colour.darker (0.2f).withMultipliedAlpha (0.3f));

    if (! (flatOnLeft || flatOnTop || flatOnBottom))
    {
        g.saveState();
		g.setGradientFill(cg);
        g.reduceClipRegion (intX, intY, intEdge, intH);
        g.fillPath (outline);
        g.restoreState();
    }

    if (! (flatOnRight || flatOnTop || flatOnBottom))
    {
        cg.point1.setX (x + width - edgeBlurRadius);
        cg.point2.setX (x + width);

        g.saveState();
        g.setGradientFill (cg);
        g.reduceClipRegion (intX + intW - intEdge, intY, 2 + intEdge, intH);
        g.fillPath (outline);
        g.restoreState();
    }

    {
        const float leftIndent = flatOnLeft ? 0.0f : cs * 0.4f;
        const float rightIndent = flatOnRight ? 0.0f : cs * 0.4f;

        Path highlight;
        createRoundedPath (highlight,
                           x + leftIndent,
                           y + cs * 0.1f,
                           width - (leftIndent + rightIndent),
                           height * 0.4f, cs * 0.4f,
                           ! (flatOnLeft || flatOnTop),
                           ! (flatOnRight || flatOnTop),
                           ! (flatOnLeft || flatOnBottom),
                           ! (flatOnRight || flatOnBottom));

        g.setGradientFill(ColourGradient (colour.brighter (10.0f), 0, y + height * 0.06f,
                          Colours::transparentWhite, 0, y + height * 0.4f, false));
        g.fillPath (highlight);
    }

    g.setColour (colour.darker().withMultipliedAlpha (1.5f));
    g.strokePath (outline, PathStrokeType (outlineThickness));
}


	
//==============================================================================
const Font & MLLookAndFeel::getFont(int style)
{
	switch(style)
	{
		case eMLPlain:
		return mPlainFont; 
		break;

		case eMLItalic:
		return mItalicFont; 
		break;

		case eMLTitle:
		return mTitleFont; 
		break;

		case eMLCaption:
		return mCaptionFont; 
		break;

		case eMLCaptionSmall:
		return mCaptionSmallFont; 
		break;

		case eMLNotice:
		return mNoticeFont; 
		break;

		default:
		return mFallbackFont;	
	}

}

// --------------------------------------------------------------------------------
#pragma mark drawing resources
//

// visual resources for app. 
void MLLookAndFeel::addPicture(MLSymbol name, const void* data, size_t dataSize)
{
	DrawablePtr newPic (Drawable::createFromImageData(data, dataSize));
	if (newPic != nullptr)
	{
		mPictures[name] = newPic;
		mPictureData[name] = (void *)data;
	}
}

const Drawable* MLLookAndFeel::getPicture(MLSymbol name)
{
	return &(*mPictures[name]);
	// TODO add ? for pictures not found. 
}
