
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// Portions of this software originate from JUCE, 
// copyright 2004-2013 by Raw Material Software ltd.
// JUCE is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "MLDial.h"
#include "MLLookAndFeel.h"

const int kDragStepSize = 16;
const int kMinGestureDuration = 250;
const int kMouseWheelStepSize = 16;
static const int kMinimumDialSizeForJump = 26;
static const float kRotaryStartDefault = kMLPi*-0.75f;
static const float kRotaryEndDefault = kMLPi*0.5f;

//==============================================================================
MLDial::MLDial () : 
	dialBeingDragged (kNoDial),
	dialToDrag (kNoDial),
	mGestureInProgress(false),

    mBottomValue (0), mTopValue (1), interval (0), doubleClickReturnValue(0.0),
	valueWhenLastDragged(0), valueOnMouseDown(0),
    numDecimalPlaces (7),
 	mOverTrack(false),
	isMouseDown(false),
	isMouseWheelMoving(false),
	//
	mLastDragTime(0), mLastWheelTime(0),
	mLastDragX(0), mLastDragY(0),
	mFilteredMouseSpeed(0.),
	mMouseMotionAccum(0),
	mFineTicksAccum(0),
	//
	pixelsForFullDragExtent (250),
    style (MLDial::LinearHorizontal),
	mValueDisplayMode(eMLNumFloat),
	//
    doubleClickToValue (false),
    isVelocityBased (false),
    userKeyOverridesVelocity (true),
    rotaryStop (true),
    incDecButtonsSideBySide (false),
    sendChangeOnlyOnRelease (false),
    popupDisplayEnabled (false),
    menuShown (false),
	mouseWasHidden(false),	
    scrollWheelEnabled (true),
    snapsToMousePos (true),
	//
	mHilightColor(Colours::white),
	//
	mWarpMode (kJucePluginParam_Linear),
	mZeroThreshold(0. - (2<<16)),
	mTopLeft(false),
	mDrawThumb(true),

	mDoSign(false),
	mDoNumber(true),
	mDigits(3), mPrecision(2),
	mBipolar(false),

	mTextSize(0.),
	mMaxNumberWidth(0.),

	mTrackThickness(kMLTrackThickness),
	mTicks(2),
	mTicksOffsetAngle(0.),
	mDiameter(0),
	mMargin(0),
	mTickSize(0),
	mShadowSize(0),

	//
	mSnapToDetents(true),
	mCurrentDetent(-1),
	mPrevLFDrawNumbers(false), // TODO
	//
	mParameterLayerNeedsRedraw(true),
	mStaticLayerNeedsRedraw(true),		
	mThumbLayerNeedsRedraw(true)

{
	mpTimer = std::unique_ptr<GestureTimer>(new GestureTimer(this));

	MLWidget::setComponent(this);
	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
	setOpaque(myLookAndFeel->getDefaultOpacity());
    
	setBufferedToImage(myLookAndFeel->getDefaultBufferMode());
	setPaintingIsUnclipped(myLookAndFeel->getDefaultUnclippedMode());

	setWantsKeyboardFocus (false);
    setRepaintsOnMouseActivity (false);
	
	setRotaryParameters(kRotaryStartDefault, kRotaryEndDefault, true);
	setVelocityBasedMode(true);
}

MLDial::~MLDial()
{
}

// MLWidget methods

void MLDial::doPropertyChangeAction(MLSymbol property, const MLProperty& val)
{
	if (property == "value")
	{
        mParameterLayerNeedsRedraw = true;
        mThumbLayerNeedsRedraw = true;
	}
    else if(property == "highlight")
    {
        mParameterLayerNeedsRedraw = true;
    }
    else if(property == "highlight_color")
    {
        mParameterLayerNeedsRedraw = true;
    }
	repaint();
}

void MLDial::sendValueOfDial(WhichDial s, float val)
{
	// TODO min and max thumbs are unimplemented
	
	//debug() << "in constrain: " << newValue << "\n";
	float oldValue = getFloatProperty("value");
	float newValue = constrainValue (val);
	//debug() << "    out constrain: " << newValue << "\n";
	
	if(newValue != oldValue)
	{
		setPropertyImmediate("value", newValue);
		sendAction("change_property", getTargetPropertyName(), getProperty("value"));
	}
}

// TODO use properties
void MLDial::setDialStyle (const MLDial::DialStyle newStyle)
{
    if (style != newStyle)
    {
        style = newStyle;
        lookAndFeelChanged();
    }
}

// TODO use properties
void MLDial::setRotaryParameters (const float startAngleRadians,
                                  const float endAngleRadians,
                                  const bool stopAtEnd)
{
	float start = startAngleRadians;
	float end = endAngleRadians;

	while(start < 0.)
	{
		start += 2*kMLPi;
	}
	while(end < start)
	{
		end += 2*kMLPi;
	}
	
    rotaryStart = start;
    rotaryEnd = end;
    rotaryStop = stopAtEnd;
}

void MLDial::setVelocityBasedMode (const bool velBased) throw()
{
    isVelocityBased = velBased;
}

void MLDial::setMouseDragSensitivity (const int distanceForFullScaleDrag)
{
    jassert (distanceForFullScaleDrag > 0);

    pixelsForFullDragExtent = distanceForFullScaleDrag;
}

void MLDial::setChangeNotificationOnlyOnRelease (const bool onlyNotifyOnRelease) throw()
{
    sendChangeOnlyOnRelease = onlyNotifyOnRelease;
}

void MLDial::setDialSnapsToMousePosition (const bool shouldSnapToMouse) throw()
{
    snapsToMousePos = shouldSnapToMouse;
}

//==============================================================================
void MLDial::colourChanged()
{
    lookAndFeelChanged();
}

void MLDial::lookAndFeelChanged()
{
    repaintAll();
}

//==============================================================================

void MLDial::setWarpMode(const JucePluginParamWarpMode w)
{	
	mWarpMode = w;
}

void MLDial::setRange (const float newMin,
                       const float newMax,
                       const float newInt,
                       const float zeroThresh,
					   JucePluginParamWarpMode warpMode)
{
	mBottomValue = newMin;
	mTopValue = newMax;
	interval = newInt;
	mZeroThreshold = zeroThresh;
	mWarpMode = warpMode;
	mFlip = (mBottomValue > mTopValue);
	
	// figure out the number of decimal places needed to display all values at this
	// interval setting.
	numDecimalPlaces = 7;
	if (newInt != 0)
	{
		int v = abs ((int) (newInt * 10000000));
		while ((v % 10) == 0)
		{
			--numDecimalPlaces;
			v /= 10;
		}
	}

	// clip the current values to the new range..
	if (style != MLDial::TwoValueHorizontal && style != MLDial::TwoValueVertical)
	{
		setPropertyImmediate("value", constrainValue(getFloatProperty("value")));
	}
	else
	{
		setPropertyImmediate("min_value", constrainValue(getFloatProperty("min_value")));
		setPropertyImmediate("max_value", constrainValue(getFloatProperty("max_value")));
	}
	
    mDigits = ceil(log10(jmax((std::abs(newMin) + 1.), (std::abs(newMax) + 1.))));
	mDoSign = ((newMax < 0) || (newMin < 0));
	mPrecision = ceil(log10(1. / newInt) - 0.0001);	
	
	// debug() << getWidgetName() << "SET RANGE: digits: " << mDigits << " precision: " << 	mPrecision << " sign: " << mDoSign << "\n";
	// debug() << "PRECISION:" << mPrecision << ", INTERVAL:" << newInt << "\n";

}

void MLDial::setDefault (const float newDefault)
{
	setDoubleClickReturnValue(true, newDefault);
}

float MLDial::clipToOtherDialValues(float val, WhichDial s)
{
	float v = getFloatProperty("value");
	float vMin = getFloatProperty("valueMin");
	float vMax = getFloatProperty("valueMax");
	float newValue = val;
	switch(s)
	{
		case kMainDial:
			if (style == MLDial::ThreeValueHorizontal || style == MLDial::ThreeValueVertical)
			{
				newValue = clamp (newValue, vMin, vMax);
			}
			break;
		case kMinDial:
			if (style == MLDial::TwoValueHorizontal || style == MLDial::TwoValueVertical)
				newValue = min (newValue, vMax);
			else
				newValue = min (v, newValue);
			break;
		case kMaxDial:
			if (style == MLDial::TwoValueHorizontal || style == MLDial::TwoValueVertical)
				newValue = max (vMin, newValue);
			else
				newValue = max (v, newValue);
			break;
		default:
			break;
	}
	return newValue;
}

void MLDial::setDoubleClickReturnValue (const bool isDoubleClickEnabled,
                                        const float valueToSetOnDoubleClick) throw()
{
    doubleClickToValue = isDoubleClickEnabled;
    doubleClickReturnValue = valueToSetOnDoubleClick;
}

float MLDial::getDoubleClickReturnValue (bool& isEnabled_) const throw()
{
    isEnabled_ = doubleClickToValue;
    return doubleClickReturnValue;
}

const String MLDial::getTextFromValue (float v)
{
    if (numDecimalPlaces > 0)
        return String (v, numDecimalPlaces) + textSuffix;
    else
        return String (roundDoubleToInt (v)) + textSuffix;
}

float MLDial::getValueFromText (const String& text)
{
    String t (text.trimStart());

    if (t.endsWith (textSuffix))
        t = t.substring (0, t.length() - textSuffix.length());

    while (t.startsWithChar ('+'))
        t = t.substring (1).trimStart();

    return t.initialSectionContainingOnly ("0123456789.,-")
            .getDoubleValue();
}

float MLDial::proportionOfLengthToValue (float proportion)
{
	float min = getBottomValue();
	float max = getTopValue();
	float r, rangeExp;
	float p = proportion;

	if (mWarpMode == kJucePluginParam_Exp)
	{
		rangeExp = p*(log(max)/log(min) - 1) + 1;
		r = pow(min, rangeExp);
		if (r < mZeroThreshold)
		{
			r = 0.;
		}		
	}
	else
	{
		r = mBottomValue + (mTopValue - mBottomValue) * p;
	}

	return r;
}

float MLDial::valueToProportionOfLength (float value) const
{
	float min = getBottomValue();
	float max = getTopValue();
	float x;
	
    if(value > mZeroThreshold)
    {
        if (mWarpMode == kJucePluginParam_Exp)
        {
            value = clamp(value, min, max);
            x = log(value/min) / log(max/min);
        }
        else
        {
            x = (value - mBottomValue) / (mTopValue - mBottomValue);
        }
    }
    else
    {
        x = 0.f;
    }
	
	return x;
}

#pragma mark -

unsigned MLDial::nearestDetent (float attemptedValue) const
{
	unsigned r = 0;
	
	int detents = mDetents.size();	
	if (detents)
	{
		float p1 = valueToProportionOfLength(attemptedValue);
		int i_min = 0;
		float d_min = 9999999.;
		
		for (int i=0; i<detents; ++i)
		{
            float dv = mDetents[i].mValue;
            float td; // distance to detent
            if(dv > mZeroThreshold)
            {
                td = fabs(valueToProportionOfLength(dv) - p1);
            }
            else
            {
                td = p1;
            }
        
            if (td < d_min)
            {
                i_min = i;
                d_min = td;
            }
		}
		r = i_min;
	}
	return r;
}

void MLDial::valueChanged()
{
	mParameterLayerNeedsRedraw = true;
	mThumbLayerNeedsRedraw = true;
}

void MLDial::enablementChanged()
{
	repaintAll();
}

void MLDial::setScrollWheelEnabled (const bool enabled) throw()
{
    scrollWheelEnabled = enabled;
}

float MLDial::constrainValue (float value) const throw()
{
	float valueOut = value;
	float fInt = interval*(mFlip ? -1.f : 1.f);
	
	// quantize to chunks of interval
	int detents = mDetents.size();
    if (!detents)
	{
        valueOut = mBottomValue + fInt * floor((valueOut - mBottomValue)/fInt + 0.5f);
	}
	
	if(!mFlip)
	{
		valueOut = clamp(value, mBottomValue, mTopValue);
	}
	else
	{
		valueOut = clamp(value, mTopValue, mBottomValue);
	}
	
	if (valueOut <= mZeroThreshold)
	{
		valueOut = 0.;
	}
	
    return valueOut;
}

float MLDial::getLinearDialPos (const float value)
{
    float dialPosProportional;
	float ret = 0.;

	if (value < mBottomValue)
	{
		dialPosProportional = 0.0;
	}
	else if (value > mTopValue)
	{
		dialPosProportional = 1.0;
	}
	else
	{
		dialPosProportional = valueToProportionOfLength (value);
		jassert (dialPosProportional >= 0 && dialPosProportional <= 1.0);
	}

	float start, extent;
    if (isVertical())
	{
        dialPosProportional = 1.0 - dialPosProportional;
		start = trackRect.top() + 1;
		extent = trackRect.height() - 1;
	}
	else
	{
		start = trackRect.left() + 1;
		extent = trackRect.width() - 1;
	}
	ret = (float) (start + dialPosProportional*extent);
    return ret;
}

bool MLDial::isHorizontal() const throw()
{
    return style == MLDial::LinearHorizontal
        || style == MLDial::LinearBar
        || style == MLDial::TwoValueHorizontal
        || style == MLDial::ThreeValueHorizontal
        || style == MLDial::MultiHorizontal;
}

bool MLDial::isVertical() const throw()
{
    return style == MLDial::LinearVertical
        || style == MLDial::TwoValueVertical
        || style == MLDial::ThreeValueVertical
        || style == MLDial::MultiVertical;
}

bool MLDial::isTwoOrThreeValued() const throw()
{
    return style == MLDial::TwoValueVertical
        || style == MLDial::ThreeValueVertical
        || style == MLDial::TwoValueHorizontal
        || style == MLDial::ThreeValueHorizontal;
}

bool MLDial::isTwoValued() const throw()
{
    return style == MLDial::TwoValueVertical
        || style == MLDial::TwoValueHorizontal;
}

bool MLDial::isMultiValued() const throw()
{
    return style == MLDial::MultiVertical
        || style == MLDial::MultiHorizontal;
}


float MLDial::getPositionOfValue (const float value)
{
    if (isHorizontal() || isVertical())
    {
        return getLinearDialPos (value);
    }
    else
    {
        jassertfalse; // not a valid call on a MLDial that doesn't work linearly!
        return 0.0f;
    }
}

#pragma mark paint

void MLDial::paint (Graphics& g)
{
	const int width = getWidth();
	const int height = getHeight();
	float currentValue = getFloatProperty("value");

	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();

	if (isOpaque()) 
		myLookAndFeel->drawBackground(g, this);	
	
	// see if lookandfeel's drawNumbers changed
	bool LFDrawNumbers = myLookAndFeel->mDrawNumbers;
	if (LFDrawNumbers != mPrevLFDrawNumbers)
	{
		mParameterLayerNeedsRedraw = true;
		mThumbLayerNeedsRedraw = true;
		mPrevLFDrawNumbers = LFDrawNumbers;
	}
	
	if(0)
	{
		Path P;
		Rectangle<int> br = ( getLocalBounds());	
		br.expand(1, 1);
		P.addRectangle(br);
		g.setColour(Colours::white);	
		g.fillPath(P);
	}
	
	if (style == MLDial::Rotary)
	{
		const float dialPos = (float) valueToProportionOfLength (currentValue);
		drawRotaryDial(g, 0, 0, width, height, dialPos);
		drawRotaryDialOverlay(g, 0, 0, width, height, dialPos);
	}
	else
	{
		float valueMin = getFloatProperty("min_value");
		float valueMax = getFloatProperty("max_value");
		drawLinearDial(g, 0, 0, width, height,
                       getLinearDialPos(currentValue), getLinearDialPos(valueMin), getLinearDialPos(valueMax));
		drawLinearDialOverlay(g, 0, 0, width, height,
                       getLinearDialPos(currentValue), getLinearDialPos(valueMin), getLinearDialPos(valueMax));
	}
	
    if(0)
    {
        Path P;
        const Rectangle<int> & br ( getLocalBounds());	
        P.addRectangle(br);
        g.setColour(Colours::red);	
        g.strokePath(P, PathStrokeType(1.f));
    }
}

void MLDial::repaintAll()
{
	mParameterLayerNeedsRedraw = mThumbLayerNeedsRedraw = mStaticLayerNeedsRedraw = true;
	repaint();
}

void MLDial::drawLinearDial (Graphics& g, int , int , int , int ,
	float dialPos, float minDialPos, float maxDialPos)
{
	const int compWidth = getWidth();
	const int compHeight = getHeight();
	
	Path full, not_full, thumb;	
	MLRect sr(getWidgetLocalBounds());	

	const Colour trackDark (mTrackDarkColor);		
	const Colour fill_normal = (mTrackFillColor.withAlpha (isEnabled() ? 1.f : 0.5f));
	
	MLRect nfr1, nfr2, fr, tr;
	getDialRect (nfr1, MLDial::NoFillRect1, dialPos, minDialPos, maxDialPos);
	getDialRect (nfr2, MLDial::NoFillRect2, dialPos, minDialPos, maxDialPos);
	getDialRect (fr, MLDial::FillRect, dialPos, minDialPos, maxDialPos);
	getDialRect (tr, MLDial::TrackRect, dialPos, minDialPos, maxDialPos);
	
	full.addRectangle(MLToJuceRect(fr));
	
	// parameter layer
	if (mParameterLayerNeedsRedraw)
	{	
		Graphics pg(mParameterImage);	
		mParameterImage.clear(Rectangle<int>(0, 0, compWidth, compHeight), Colours::transparentBlack);
		
		// track unfilled area(s)
		pg.setColour (trackDark);
		pg.fillRect (MLToJuceRectInt(nfr1));
		pg.fillRect (MLToJuceRectInt(nfr2));
        
		// draw fill
		{
			pg.setColour (fill_normal);
			pg.fillPath (full);	
		}
	}
	
	// static layer
	if (mStaticLayerNeedsRedraw)
	{
		Graphics sg(mStaticImage);	
		mStaticImage.clear(Rectangle<int>(0, 0, compWidth, compHeight), Colours::transparentBlack);
		const Colour label_color = (findColour(MLLookAndFeel::labelColor).withAlpha (isEnabled() ? 1.f : 0.5f));	
        
		// detents 
        if (isHorizontal())
        {
			float tX = tr.x();
			float tY = tr.y();
			float tW = tr.getWidth();
			float x1, y1, x2, y2;

			for (unsigned i=0; i<mDetents.size(); ++i)
			{
				float td = valueToProportionOfLength(mDetents[i].mValue); 
				float xx = (tX + (td * tW));	
				Path J;

				// draw tick
				x1 = xx;
				y1 = tY - mMargin;
				x2 = xx;
				y2 = tY - mMargin*(1.0f - mDetents[i].mWidth);
									
				J.startNewSubPath(x1, y1);
				J.lineTo(x2, y2);

				sg.setColour (label_color.withAlpha(1.f));
				sg.strokePath (J, PathStrokeType(mLineThickness*2));
			}
		}
        else
        {
 			float tX = tr.x();
			float tY = tr.y();
			//float tW = tr.getWidth();
			float tH = tr.getHeight();
			float x1, y1, x2, y2;
            
			for (unsigned i=0; i<mDetents.size(); ++i)
			{
				float td = valueToProportionOfLength(mDetents[i].mValue);
				float yy = (tY + (td * tH));
				Path J;
                
				// draw tick
				x1 = tX - mMargin;
				y1 = yy;
				x2 = tX - mMargin*(1.0f - mDetents[i].mWidth);
				y2 = yy;
                
				J.startNewSubPath(x1, y1);
				J.lineTo(x2, y2);
                
				sg.setColour (label_color.withAlpha(1.f));
				sg.strokePath (J, PathStrokeType(mLineThickness*2));
                
			}
        }
	}
	
	// composite images
	//
	if(mParameterImage.isValid())
	{
		g.drawImage (mParameterImage, 0, 0, compWidth, compHeight, 0, 0, compWidth, compHeight, false);
	}
	if(mStaticImage.isValid())
	{
		g.drawImage (mStaticImage, 0, 0, compWidth, compHeight, 0, 0, compWidth, compHeight, false);
	}
	mParameterLayerNeedsRedraw = mStaticLayerNeedsRedraw = false;
}

void MLDial::drawLinearDialOverlay (Graphics& g, int , int , int , int ,
    float dialPos, float minDialPos, float maxDialPos)
{
	const int compWidth = getWidth();
	const int compHeight = getHeight();
	const float cornerSize = 0;
	int thumbAdorn1, thumbAdorn2;
	int glow1 = false, glow2 = false;
	float val1, val2;
	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();

    const Colour thumb_normal = (mFillColor.withAlpha (isEnabled() ? 1.f : 0.5f));
	const Colour outline = findColour(MLLookAndFeel::outlineColor).withAlpha (isEnabled() ? 1.f : 0.5f);
	const Colour textColor = outline;

	MLRect nfr1, nfr2, fr, tr, thumb1, thumb2, tip1, tip2, text1, text2;
	getDialRect (nfr1, MLDial::NoFillRect1, dialPos, minDialPos, maxDialPos);
	getDialRect (nfr2, MLDial::NoFillRect2, dialPos, minDialPos, maxDialPos);
	getDialRect (fr, MLDial::FillRect, dialPos, minDialPos, maxDialPos);
	getDialRect (tr, MLDial::TrackRect, dialPos, minDialPos, maxDialPos);
	getDialRect (thumb1, MLDial::Thumb1Rect, dialPos, minDialPos, maxDialPos);
	getDialRect (thumb2, MLDial::Thumb2Rect, dialPos, minDialPos, maxDialPos);
	getDialRect (tip1, MLDial::Tip1Rect, dialPos, minDialPos, maxDialPos);
	getDialRect (tip2, MLDial::Tip2Rect, dialPos, minDialPos, maxDialPos);
	getDialRect (text1, MLDial::Text1Rect, dialPos, minDialPos, maxDialPos);
	getDialRect (text2, MLDial::Text2Rect, dialPos, minDialPos, maxDialPos);
    
	long doDial1, doDial2;
	const int multi = (isTwoOrThreeValued());
 	if (multi)
	{
		doDial1 = doDial2 = true;
		val1 = getFloatProperty("min_value");
		val2 = getFloatProperty("max_value");
	}
	else
	{
		doDial1 = mTopLeft;
		doDial2 = !doDial1;
		val1 = val2 = getFloatProperty("value");
	}

    if (isHorizontal())
	{
		thumbAdorn1 = eMLAdornBottomLeft | eMLAdornBottom | eMLAdornBottomRight;
		thumbAdorn2 = eMLAdornTopLeft | eMLAdornTop | eMLAdornTopRight;
	}
	else // vertical
	{
		thumbAdorn1 = eMLAdornTopRight | eMLAdornRight | eMLAdornBottomRight;
		thumbAdorn2 = eMLAdornTopLeft | eMLAdornLeft | eMLAdornBottomLeft;
	}

	if(mThumbLayerNeedsRedraw)
	{
		Graphics tg(mThumbImage);
		mThumbImage.clear(Rectangle<int>(0, 0, compWidth, compHeight), Colours::transparentBlack);
		
		// draw track outline
		{
			Path track;
			MLRect tra = tr;
			tra.shrink(0.5f);
			track.addRectangle (MLToJuceRect(tra));
			tg.setColour (outline);
			tg.strokePath (track, PathStrokeType(mLineThickness));
		}
		
		// draw thumbs
		if (doDial1 && mDrawThumb)
		{
			myLookAndFeel->drawMLButtonShape (tg, thumb1, cornerSize,
                                              thumb_normal, outline, mLineThickness, thumbAdorn1, tip1.x(), tip1.y());
		}
		if (doDial2 && mDrawThumb)
		{
			myLookAndFeel->drawMLButtonShape (tg, thumb2, cornerSize,
                                              thumb_normal, outline, mLineThickness, thumbAdorn2, tip2.x(), tip2.y());
		}
	}

    if(mThumbImage.isValid())
	{
		g.drawImage (mThumbImage, 0, 0, compWidth, compHeight, 0, 0, compWidth, compHeight, false);
	}
    
	// finally, draw numbers over composited images
	//
	if(1)//(mParameterLayerNeedsRedraw)
	{
		if (doDial1 && mDrawThumb)
		{
			if (myLookAndFeel->mDrawNumbers && mDoNumber)
			{
				g.setOpacity(isEnabled() ? (glow1 ? 0.8 : 1.) : 0.4f);
				const char* numBuf = myLookAndFeel->formatNumber(val1, mDigits, mPrecision, mDoSign, mValueDisplayMode);
				myLookAndFeel->drawNumber(g, numBuf, text1.x(), text1.y(), text1.getWidth(),
                    text1.getHeight(), textColor, Justification::centred);
			}
		}
		if (doDial2 && mDrawThumb)
		{
			if (myLookAndFeel->mDrawNumbers && mDoNumber)
			{
				g.setOpacity(isEnabled() ? (glow2 ? 0.8 : 1.) : 0.4f);
				const char* numBuf = myLookAndFeel->formatNumber(val2, mDigits, mPrecision, mDoSign, mValueDisplayMode);
				myLookAndFeel->drawNumber(g, numBuf, text2.x(), text2.y(), text2.getWidth(),
                    text2.getHeight(), textColor, Justification::centred);
			}
		}
	}
	mThumbLayerNeedsRedraw = false;
}

#pragma mark rotary dial

void MLDial::drawRotaryDial (Graphics& g, int rx, int ry, int rw, int rh, float dialPos)
{	
 	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
	const MLRect uBounds = getGridBounds();
	const MLRect boundsRect = getWidgetLocalBounds();

	// Colors 
	const Colour trackDark = (mTrackDarkColor.withMultipliedAlpha (isEnabled() ? 1.f : 0.5f));					
	const Colour shadow (findColour(MLLookAndFeel::shadowColor).withAlpha (isEnabled() ? 1.f : 0.5f));
	const Colour outline_color (findColour(MLLookAndFeel::outlineColor).withAlpha (isEnabled() ? 1.f : 0.5f));
	const Colour fill_color (mTrackFillColor.withAlpha (isEnabled() ? 1.0f : 0.5f));
	const Colour indicator_color (mIndicatorColor.withAlpha (isEnabled() ? 1.f : 0.5f));

	// Dimensions
	const float r1 = mDiameter*0.5f;
	const MLPoint center = getDialCenter();
	float cx, cy;
	cx = (int)center.x() + 0.5;
	cy = (int)center.y();
	
    const float iy = cy;	// indicator y
    const float rx1 = cx - r1;
    const float ry1 = cy - r1;
	
	float indicator_thick = mLineThickness*2.f;
	
	bool do_indicator = true;
	bool do_ticks = true;
	if (uBounds.height() <= 0.5f) 
	{
		do_ticks = 0;
	}
	
	float posA, posB;
	float angleA, angleB, angleI, angleM;
	float middlePos = valueToProportionOfLength(0.);
	
	if (mBipolar)
	{
		if (dialPos < middlePos)
		{
			posA = dialPos;
			posB = middlePos;
		}
		else
		{
			posA = middlePos;
			posB = dialPos;
		}
	}
	else
	{
		posA = 0.;
		posB = dialPos;
	}
	
	angleA = rotaryStart + posA * (rotaryEnd - rotaryStart);
	angleB = rotaryStart + posB * (rotaryEnd - rotaryStart);
	angleI = rotaryStart + dialPos * (rotaryEnd - rotaryStart);
	angleM = rotaryStart + middlePos * (rotaryEnd - rotaryStart);
		
	float ttop = 0., tleft = 0.;
	ttop = cy + mMargin*uBounds.height()*0.5f;
	tleft = cx;

 	// parameter layer
	if (mParameterLayerNeedsRedraw)
	{	
		Graphics pg(mParameterImage);	
		mParameterImage.clear(MLToJuceRectInt(boundsRect), Colours::transparentBlack);
		
		// unfilled area(s)
		{	
            Path track;
			pg.setColour (trackDark);
            track.addPieSegment (rx1, ry1, mDiameter, mDiameter, rotaryStart, angleA, 0.);
            track.addPieSegment (rx1, ry1, mDiameter, mDiameter, angleB, rotaryEnd, 0.);
            pg.fillPath (track);
		}
		
		// big fill area
		{	
            Path filledArc;
			pg.setColour (fill_color);
            filledArc.addPieSegment (rx1, ry1 , mDiameter, mDiameter, angleA, angleB, 0.);
            pg.fillPath (filledArc);
		}

		// indicator line
		if (do_indicator)
		{	
			Path indicator;
			indicator.startNewSubPath(0., 0.);
			indicator.lineTo(0., -r1+0.5);
			pg.setColour (indicator_color);
            pg.strokePath (indicator, PathStrokeType(indicator_thick), AffineTransform::rotation(angleI).translated (cx, iy));
		}
		
		// hilight (for patcher connections to tiny dials)
		if ((bool)getFloatProperty("highlight"))
		{
			const float m = mLineThickness*10.;
			const float mh = m / 2.;
            Path track;
			Colour hc = signalToJuceColor(getSignalProperty("highlight_color"));
			pg.setColour (hc);
            track.addArc(rx1-mh, ry1-mh, mDiameter+m, mDiameter+m, 0.f, kMLTwoPi, true);
            pg.strokePath (track, PathStrokeType(m));			
		}
	}
    
	// static layer
	if (mStaticLayerNeedsRedraw)
    {
		const Colour label_color = (findColour(MLLookAndFeel::labelColor).withAlpha (isEnabled() ? 1.f : 0.5f));	
		mStaticImage.clear(MLToJuceRectInt(boundsRect), Colours::transparentBlack);

		Graphics sg(mStaticImage);	
		{	
			// outer shadow
			Path outline;
			float d, opacity;
			for (int i=0; i<mShadowSize; i++)
			{
				outline.clear();			
				outline.addCentredArc(cx, cy, r1 + i + 0.5, r1 + i + 0.5, 0., rotaryStart, rotaryEnd, true);
				d = (float)(mShadowSize - i) / (float)mShadowSize; // 0. - 1.
				opacity = d * d * d * kMLShadowOpacity;
				sg.setColour (shadow.withAlpha(opacity));
				sg.strokePath (outline, PathStrokeType (1.f));	
			}			
		}		
		
		{	
			// track outline
			Path outline;
			outline.addCentredArc(cx, cy, r1, r1, 0., rotaryStart, rotaryEnd, true);			
			sg.setColour (outline_color.withAlpha(0.25f));
			sg.strokePath (outline, PathStrokeType (mLineThickness));
			sg.setColour (outline_color);
			sg.strokePath (outline, PathStrokeType (mLineThickness*2));
		}

		if (do_ticks)
		{
			float angle;
			Path tick;
			tick.startNewSubPath(0, -r1);
			tick.lineTo(0, -r1-mTickSize);
			sg.setColour (outline_color);
			for (int t=0; t<mTicks; t++)
			{
				angle = rotaryStart + (t * (rotaryEnd - rotaryStart) / (mTicks - 1)) ;
				angle += mTicksOffsetAngle;
				sg.strokePath (tick, PathStrokeType(mLineThickness*2), AffineTransform::rotation (angle).translated (cx, cy - 0.5f));
			}
		}

		// draw detents
		if(mDetents.size() > 0)
		{
			float x1, y1, x2, y2;		
			for (unsigned i=0; i<mDetents.size(); ++i)
			{
				float td = valueToProportionOfLength(mDetents[i].mValue); 
				
				// if the detent has a label, it's a line under the text, otherwise a small dot.
				float theta = rotaryStart + (td * (rotaryEnd - rotaryStart));
	
				bool coveringEnd = approxEqual(theta, rotaryStart) || approxEqual(theta, rotaryEnd);
				if (!coveringEnd)
				{
					Path J;

					// draw detent - outer edge lines up with tick
					AffineTransform t1 = AffineTransform::rotation(theta).translated(cx, cy);
					x1 = 0;
					x2 = 0;
					float tW = 0.875f; //  tweak
					y1 = -r1 - (mTickSize*tW);
					y2 = -r1 - (mTickSize*tW)*(1.f - mDetents[i].mWidth);
					t1.transformPoint(x1, y1);
					t1.transformPoint(x2, y2);					
					J.startNewSubPath(x1, y1);
					J.lineTo(x2, y2);
					sg.setColour (label_color);
					sg.strokePath (J, PathStrokeType(mLineThickness*2));
				}
			}
		}		
	}	
	
	// composite images
	if(mParameterImage.isValid())
	{
		g.drawImageTransformed (mParameterImage, AffineTransform::translation(rx, ry), false);
	}
    
	// draw number text over composited images
	if (1)//(mParameterLayerNeedsRedraw)
	{
		if (myLookAndFeel->mDrawNumbers && mDoNumber)
		{
			// draw background under text
			if (!isOpaque())
				myLookAndFeel->drawBackgroundRect(g, this, mRotaryTextRect);
            
			float textSize = mTextSize;
			float op = isEnabled() ? 1.f : 0.4f;
			const char* numBuf = myLookAndFeel->formatNumber(getFloatProperty("value"), mDigits, mPrecision, mDoSign, mValueDisplayMode);
			myLookAndFeel->drawNumber(g, numBuf, tleft, ttop, boundsRect.getWidth() - tleft, textSize,
                                      findColour(MLLookAndFeel::outlineColor).withAlpha(op));
		}
	}
    
	mParameterLayerNeedsRedraw = mStaticLayerNeedsRedraw = false;
}

void MLDial::drawRotaryDialOverlay (Graphics& g, int rx, int ry, int rw, int rh, float dialPos)
{
    if(mStaticImage.isValid())
    {
		g.drawImageTransformed (mStaticImage, AffineTransform::translation(rx, ry), false);
    }
}

#pragma mark -
#pragma mark resize

void MLDial::moved()
{
	mParameterLayerNeedsRedraw = mStaticLayerNeedsRedraw = true;
}

void MLDial::resized()
{
}

void MLDial::sizeChanged()
{
	resized();
}

void MLDial::visibilityChanged()
{
	mParameterLayerNeedsRedraw = mStaticLayerNeedsRedraw = true;
}

#pragma mark -
void MLDial::mouseDown (const MouseEvent& e)
{
    mouseWasHidden = false;
	mParameterLayerNeedsRedraw = true;
	mThumbLayerNeedsRedraw = true;
	
	isMouseDown = true;
	
    if (isEnabled())
    {		
		findDialToDrag(e);	// sets dialToDrag
		dialBeingDragged = dialToDrag;
		
		if (dialBeingDragged != kNoDial)
		{
			beginGesture();
			mLastDragX = e.x;
			mLastDragY = e.y;
			mLastDragTime = Time(e.eventTime.toMilliseconds());
			mFilteredMouseSpeed = 0.;
			mMouseMotionAccum = 0;
			
			if (dialBeingDragged == kMinDial)
				valueWhenLastDragged = getFloatProperty("min_value");
			else if (dialBeingDragged == kMaxDial)
				valueWhenLastDragged = getFloatProperty("max_value");
			else
				valueWhenLastDragged = getFloatProperty("value");

			valueOnMouseDown = valueWhenLastDragged;
			mouseDrag(e);
		}
    }
}

void MLDial::mouseUp (const MouseEvent&)
{
    if (isEnabled())
    {
		mpTimer->scheduleEndGesture(kMinGestureDuration);
    }
}

void MLDial::restoreMouseIfHidden()
{
    if (mouseWasHidden)
    {
        mouseWasHidden = false;
        for (int i = Desktop::getInstance().getNumMouseSources(); --i >= 0;)
            Desktop::getInstance().getMouseSource(i)->enableUnboundedMouseMovement (false);

        Point<int> mousePos;
		mousePos = Desktop::getLastMouseDownPosition();
        Desktop::setMousePosition (mousePos);
    }
}

void MLDial::hideMouse()
{
	for (int i = Desktop::getInstance().getNumMouseSources(); --i >= 0;)
		Desktop::getInstance().getMouseSource(i)->hideCursor(); 
}

void MLDial::modifierKeysChanged (const ModifierKeys& modifiers)
{
    if (isEnabled()
         && style != MLDial::Rotary
         && isVelocityBased == modifiers.isAnyModifierKeyDown())
    {
        restoreMouseIfHidden();
    }
}

void MLDial::mouseDoubleClick (const MouseEvent&)
{
    if (doubleClickToValue
         && isEnabled()
         && mBottomValue <= doubleClickReturnValue
         && mTopValue >= doubleClickReturnValue)
    {
		beginGesture();
		float newValue = constrainValue(doubleClickReturnValue);
		setPropertyImmediate("value", newValue);
		sendAction("change_property", getTargetPropertyName(), getProperty("value"));
		mpTimer->scheduleEndGesture(kMinGestureDuration);
    }
}

float MLDial::getNextValue(float oldVal, int dp, bool doFineAdjust, int stepSize)
{		
	float val = oldVal;
	float r = val;
	int detents = mDetents.size();
	
	if(mFlip)
	{
		dp = -dp;
	}
	
	if ((val < mZeroThreshold) && (dp > 0))
	{
		val = mZeroThreshold;
	}
	if (detents && mSnapToDetents && !doFineAdjust)
	{	
        // more than one detent may be spanned by this event. calculate the number of steps
        // and store the remainder back in mMouseMotionAccum.
        int np = mMouseMotionAccum + dp;
        int steps = np / stepSize;
        mMouseMotionAccum = np - (steps * stepSize);
		
		if (steps != 0)
		{
			mCurrentDetent = nearestDetent(val);				
			mCurrentDetent += steps;
			mCurrentDetent = clamp(mCurrentDetent, 0, detents - 1);
			r = mDetents[mCurrentDetent].mValue;
		}
	} 
	else 
	{
		MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
		int d = myLookAndFeel->getDigitsAfterDecimal(val, mDigits, mPrecision);
		d = clamp(d, 0, 3);
		float minValChange = max(powf(10., -d), interval);		
		
		// for dials without many possible values, slow down mouse movement
		// as the inverse proportion to the number of values.
		int ticks;		
		int values = max(4, (int)(fabs(mTopValue - mBottomValue)/interval));
		const int valuesThresh = 128;
		if(values < valuesThresh)
		{			
			int fineTicksScale = (float)valuesThresh/(float)values;
			if(doFineAdjust)
			{
				fineTicksScale *= 8;
			}
			mFineTicksAccum += dp;
			ticks = mFineTicksAccum/fineTicksScale;
			mFineTicksAccum -= ticks*fineTicksScale;
		}
		else
		{
			ticks = dp;
		}
		
		if(ticks != 0)
		{
			if(doFineAdjust)
			{
				// get minimum visible change as change in position
				float p2 = valueToProportionOfLength (val + minValChange*ticks);
				r = proportionOfLengthToValue (clamp (p2, 0.0f, 1.0f));
			}
			else 
			{
				// move dial 1/100 of length range * drag distance
				float kTickDistance = 0.01f;
				if(mFlip) kTickDistance *= -1.f;
				float p1 = valueToProportionOfLength (val);
				float p2 = p1 + ticks*kTickDistance;
				r = proportionOfLengthToValue (clamp (p2, 0.0f, 1.0f));
				
				// if this motion is too small to change the displayed value, 
				// do the smallest visible change instead
				if(ticks > 0)
				{
					if (r < val + minValChange)
					{
						r = val + minValChange;
					}
				}
				else if(ticks < 0)
				{
					if (r > val - minValChange)
					{
						r = val - minValChange;
					}
				}
			}
		}
	}
	return r;
}

void MLDial::mouseDrag (const MouseEvent& e)
{
	const MLRect uBounds = getGridBounds();
    bool doInitialJump;
    if (style == MLDial::Rotary)
    {
        doInitialJump = uBounds.height() > 0.5f;
    }
    else
    {
        doInitialJump = true;
    }
    
	int detents = mDetents.size();
	
	// TODO can we make shift work if pressed during drag?
	// right now it depends on the host. 
	bool doFineAdjust = e.mods.isShiftDown();

	findDialToDrag(e);
			
    if (isEnabled() && (! menuShown))
    {
		// command click returns to doubleClickValue and exits.
		if ((e.mods.isCommandDown()) && doubleClickToValue)
		{
			if ( isEnabled()
				 && mBottomValue <= doubleClickReturnValue
				 && mTopValue >= doubleClickReturnValue)
			{
				setPropertyImmediate ("value", doubleClickReturnValue);
			}
			return;
		}
		
		// snap to position on first click.
		// first click for Rotary style snaps to position.
		if (doInitialJump && e.mouseWasClicked())
		{
			if (style == MLDial::Rotary)
			{
				int width = trackRect.getWidth();
	//			int height = trackRect.getHeight();
		
				if(width > kMinimumDialSizeForJump)
				{		
					const MLPoint center = getDialCenter();
					int dx = e.x - center.x();
					int dy = e.y - center.y();

					// if far enough from center
					if (dx * dx + dy * dy > 9)
					{
						float angle = atan2 ((float) dx, (float) -dy);
						while (angle < rotaryStart)
							angle += double_Pi * 2.0;

						// if between start and end
						if (angle < rotaryEnd)
						{
							const float proportion = (angle - rotaryStart) / (rotaryEnd - rotaryStart);
							valueWhenLastDragged = proportionOfLengthToValue (clamp (proportion, 0.0f, 1.0f));
						}
					}
				}
			}
			else // linear
			{
				float start = isHorizontal() ? trackRect.left() : trackRect.top();
				float extent = isHorizontal() ? trackRect.width() : trackRect.height();

				if (mOverTrack)
				{
					const int mousePos = (isHorizontal() || style == MLDial::RotaryHorizontalDrag) ? e.x : e.y;
					float scaledMousePos = (mousePos - start) / extent;

					if (isVertical())
					{
						scaledMousePos = 1.0f - scaledMousePos;
					}
					valueWhenLastDragged = proportionOfLengthToValue (clamp (scaledMousePos, 0.0f, 1.0f));
				}		
			}
			if(detents && mSnapToDetents && !doFineAdjust)
			{				
				mCurrentDetent = nearestDetent(valueWhenLastDragged);
				valueWhenLastDragged = mDetents[mCurrentDetent].mValue;
			}
		}
		
		else if (dialBeingDragged != kNoDial) 
		{
			e.source.enableUnboundedMouseMovement (true, false);
            
			int dp = isHorizontal() ? (e.x - mLastDragX) : -(e.y - mLastDragY);
			mLastDragX = e.x;
			mLastDragY = e.y;
			float val = getFloatProperty("value");
			if(dp != 0)
			{
				valueWhenLastDragged = getNextValue(val, dp, doFineAdjust, kDragStepSize);	
			}		
		}		
		sendValueOfDial(dialBeingDragged, valueWhenLastDragged);
		mParameterLayerNeedsRedraw = true;
		mThumbLayerNeedsRedraw = true;
		repaint();
    }
}

void MLDial::mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& wheel)
{
	// filter out zero motions from trackpad
	if ((wheel.deltaX == 0.) && (wheel.deltaY == 0.)) return;

	bool doFineAdjust = e.mods.isShiftDown();
	findDialToDrag(e);

    if (scrollWheelEnabled && isEnabled())
    {
        if (!isMouseButtonDownAnywhere())
        {
			float dpf;
			if (isHorizontal())
			{
				dpf = (wheel.deltaX != 0 ? -wheel.deltaX : wheel.deltaY);
			}
			else 
			{
				dpf = (wheel.deltaY);
			}
            
            if(wheel.isReversed) dpf = -dpf;			
			float val = getFloatProperty("value");
			int dir = sign(dpf);
			int dp = dir*max(1, (int)(fabs(dpf)*16.)); // mouse scale for no detents
						
			float oldVal = valueWhenLastDragged;
			valueWhenLastDragged = getNextValue(val, dp, doFineAdjust, kMouseWheelStepSize);
			
			if(valueWhenLastDragged != oldVal)
			{
				if(!isMouseWheelMoving)
				{
					isMouseWheelMoving = true;
					beginGesture();
				}
				mpTimer->scheduleEndGesture(kMinGestureDuration);
				
				sendValueOfDial(dialToDrag, valueWhenLastDragged);
				mParameterLayerNeedsRedraw = true;
				mThumbLayerNeedsRedraw = true;
				repaint();
			}
        }
    }
    else
    {
        Component::mouseWheelMove (e, wheel);
    }
}

#pragma mark -
#pragma mark ML Stuff

const MLPoint MLDial::getDialCenter() const
{
	return MLPoint(mDialCenter.x(), mDialCenter.y());
}

void MLDial::setTicks (int t)
{
	mTicks = t;
}

void MLDial::setTicksOffsetAngle (float t)
{
	mTicksOffsetAngle = t;
}

// the colors for different MLDial parts are generated algorithmically.
void MLDial::setFillColor (const Colour& color)
{
	// fill for thumb
    mFillColor = color;
	
	// fill for full track	
    mTrackFillColor = color.overlaidWith(Colours::black.withAlpha(0.05f));

	// indicator line
	mIndicatorColor = brightLineColor(color);
	
	// glow (background for rollover) 
	mGlowColor = mFillColor.overlaidWith(mIndicatorColor.withAlpha(0.10f));
	mThumbGlowColor = mFillColor.overlaidWith(mIndicatorColor.withAlpha(0.75f));
	
	mTrackDarkColor = findColour(MLLookAndFeel::darkFillColor);	

	//lookAndFeelChanged();
}

void MLDial::setHilightColor (const Colour& color)
{
	mHilightColor = color;
}

MLDial::WhichDial MLDial::getRectOverPoint(const MouseEvent& e)
{
	int x = e.getMouseDownX();
	int y = e.getMouseDownY();

	return getRectOverPoint(x, y);
}

MLDial::WhichDial MLDial::getRectOverPoint(const int xx, const int yy)
{
	WhichDial which = kNoDial;
	
	// looks correct for all our Juce lines centered on 0.5
	const int x = xx - 1;
	const int y = yy - 1;

	MLRect minRect, maxRect, mainRect, fieldRect;
	float minPos, maxPos, dialPos;
	dialPos = getLinearDialPos (getFloatProperty("value"));
    minPos = getLinearDialPos (getFloatProperty("min_value"));
    maxPos = getLinearDialPos (getFloatProperty("max_value"));

	if (isTwoOrThreeValued())
	{
		// can cache these.
		getDialRect (minRect, Thumb1Rect, dialPos, minPos, maxPos);
		getDialRect (maxRect, Thumb2Rect, dialPos, minPos, maxPos);
	
		if (minRect.contains(x, y))
		{
//	printf("clicked multiMIN\n");
			which = kMinDial;
		}
		else if (maxRect.contains(x, y))
		{
//	printf("clicked multiMAX\n");
			which = kMaxDial;
		}
		else if (trackRect.contains(x, y))
		{
//	printf("clicked multiTRACK\n");
			which = kTrackDial;
		}
	}
	else
	{
		getDialRect (trackRect, TrackRect, dialPos, minPos, maxPos);
		getDialRect (mainRect, mTopLeft ? Thumb1Rect : Thumb2Rect, dialPos, minPos, maxPos);
		getDialRect (fieldRect, mTopLeft ? Thumb1Field : Thumb2Field, dialPos, minPos, maxPos);
		
		if (mainRect.contains(x, y))
		{
            //printf("clicked MAIN\n");
			which = kMainDial;
		}
		else if (trackRect.contains(x, y))
		{
            //printf("clicked TRACK\n");
			which = kTrackDial;
		}
		else if (fieldRect.contains(x, y))
		{
            //printf("clicked FIELD\n");
			which = kMainDial;
		}
	}
	
	return which;
}

// findDialToDrag: usually same as thumb area over point.  But if
// we are in track, find closest thumb to mouse position.
// SIDE EFFECT: sets mInTrack.

void MLDial::findDialToDrag(const MouseEvent& e)
{
	int x = e.getMouseDownX();
	int y = e.getMouseDownY();
	findDialToDrag(x, y);
}

void MLDial::findDialToDrag(const int x, const int y)
{
	float min, max;
	WhichDial thumb;

	if (getDialStyle() == MLDial::Rotary)
	{
		thumb = kMainDial;
	}
	else
	{
		// if over area covered by a MLDial, drag that one. 
		thumb = getRectOverPoint(x, y);
		
		if (thumb == kTrackDial)
		{
			mOverTrack = true;
			if (isTwoOrThreeValued())
			{		
				float tweak = isVertical() ? 0.1 : -0.1;  // for min/max order when equal
				min = getPositionOfValue (getFloatProperty("min_value"));
				max = getPositionOfValue (getFloatProperty("max_value"));
				
				const float mousePos = (float) (isVertical() ? y : x);
	//			printf("in track: min %f, max %f, mouse %f\n", min, max, mousePos);
				const float minPosDistance = fabsf (min + tweak - mousePos);
				const float maxPosDistance = fabsf (max - tweak - mousePos);
			
				if (maxPosDistance <= minPosDistance)
				{
	//			printf ("drag max\n");
					thumb = kMaxDial;
				}
				else
				{
	//			printf ("drag min\n");
					thumb = kMinDial;
				}
			}
			else
			{
				thumb = kMainDial;
			}
		}
		else
		{
			mOverTrack = false;
		}
	}
	
	dialToDrag = thumb;
}

void MLDial::addDetent(const float value, const float width)
{
    mDetents.push_back(MLDialDetent(value, width));
}

void MLDial::snapToDetents(const bool snap)
{
	mSnapToDetents = snap;
}

#pragma mark dial dims

// return whole area for specified area of the dial in bounds sr,
// given positions of dials. 
//
void MLDial::getDialRect (MLRect& ret,
	const MLDial::DialRect whichRect,
	const float dialPos, const float minDialPos, const float maxDialPos) 
{
 	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
    bool smallThumbs = getFloatProperty("small_thumbs");
	bool multi = (isTwoOrThreeValued());
		
	MLRect full, notFull1, notFull2;

	float middlePos = getLinearDialPos(0.);
	float iPos1, iPos2; 
	float fillPos1, fillPos2; 
	float val1, val2;
	bool doDial1, doDial2;
	if (multi)
	{
		doDial1 = doDial2 = true;
		val1 = getFloatProperty("min_value");
		val2 = getFloatProperty("max_value");
		fillPos1 = iPos1 = (minDialPos);		
		fillPos2 = iPos2 = (maxDialPos);		
	}
	else
	{
		doDial1 = mTopLeft;
		doDial2 = !doDial1;
		val1 = val2 = getFloatProperty("value");
		iPos1 = iPos2 = floor(dialPos);		
		if (mBipolar)
		{
			if (dialPos < middlePos)
			{
				fillPos1 = (dialPos);
				fillPos2 = (middlePos);
			}
			else
			{
				fillPos1 = (middlePos);
				fillPos2 = (dialPos);
			}
		}
		else
		{
			fillPos1 = fillPos2 = (dialPos);		
		}
	}
	
	// get sizes of currently displayed numbers
    int numWidth1, numWidth2;
    if(!smallThumbs)
    {
        numWidth1 = mTextSize*myLookAndFeel->getNumberWidth(val1, mDigits, mPrecision, mDoSign);
        numWidth2 = mTextSize*myLookAndFeel->getNumberWidth(val2, mDigits, mPrecision, mDoSign);
        numWidth1 |= 0x1;
        numWidth2 |= 0x1;
    }
    else
    {
        numWidth1 = 0;
        numWidth2 = 0;
    }
	
 	MLRect text1Size(0, 0, numWidth1, mTextHeight);
	MLRect text2Size(0, 0, numWidth2, mTextHeight);

	// get thumb size
    int thumbWidth, thumbHeight;
    bool doNumber = (myLookAndFeel->mDrawNumbers && mDoNumber);
	if (isHorizontal())
	{
        if(smallThumbs)
        {
            int tt = mTrackThickness;
            thumbWidth = tt*2 + 1;
            thumbHeight = tt + tt/2;
        }
        else
        {
            if(doNumber)
            {
                thumbWidth = mMaxNumberWidth + mThumbMargin*2;
                thumbHeight = mTextHeight + mThumbMargin*2;
            }
            else
            {
                thumbWidth = mTrackThickness*2 - 1;
                thumbHeight = mTextHeight + mThumbMargin*2;
            }
        }
    }
    else
    {
        if(smallThumbs)
        {
            int tt = mTrackThickness;
            thumbWidth = tt + tt/2;
            thumbHeight = tt*2 + 1;
        }
        else
        {
            thumbWidth = mMaxNumberWidth + mThumbMargin*2;
            thumbHeight = mTextHeight + mThumbMargin*2 ;
        }
    }
	
	MLRect thumbSize(0, 0, thumbWidth, thumbHeight);

	// get thumb center(s)
	Vec2 thumb1Center, thumb2Center;
	if (isHorizontal())
	{
		thumb1Center = Vec2(iPos1, trackRect.top() - thumbHeight/2 );
		thumb2Center = Vec2(iPos2, trackRect.bottom() + thumbHeight/2 );
        if(smallThumbs)
        {
            thumb1Center += Vec2(0, 0);
            thumb2Center += Vec2(0, 0);
        }
	}
	else
	{
		thumb1Center = Vec2(trackRect.left() - thumbWidth/2 , iPos1);
		thumb2Center = Vec2(trackRect.right() + thumbWidth/2 , iPos2);
        thumb1Center += Vec2(1, 0);
        thumb2Center += Vec2(-1, 0);
	}
    
	// get adornment (which spikes on thumbs)
	int thumbAdorn1, thumbAdorn2;
	if (isHorizontal())
	{
		thumbAdorn1 = eMLAdornBottomLeft | eMLAdornBottom | eMLAdornBottomRight;
		thumbAdorn2 = eMLAdornTopLeft | eMLAdornTop | eMLAdornTopRight;
	}
	else
	{
		thumbAdorn1 = eMLAdornTopRight | eMLAdornRight | eMLAdornBottomRight;	
		thumbAdorn2 = eMLAdornTopLeft | eMLAdornLeft | eMLAdornBottomLeft;
	}

	// get tip
	Vec2 thumb1Tip = thumb1Center;
	Vec2 thumb2Tip = thumb2Center;
	if (isHorizontal())
	{
		thumb1Tip += Vec2(0, mTrackThickness + thumbHeight/2 - 1);
		thumb2Tip -= Vec2(0, mTrackThickness + thumbHeight/2 - 1);
        if(smallThumbs)
        {
            thumb1Tip += Vec2(0, 0);
            thumb2Tip += Vec2(-1, 0);
        }
        else
        {
            if(doNumber)
            {
            }
            else
            {
                thumb1Tip += Vec2(0, 0);
                thumb2Tip += Vec2(-1, 0);
            }
        }
	}
	else
	{
		thumb1Tip += Vec2(mTrackThickness + thumbWidth/2 - 1, 0);
		thumb2Tip -= Vec2(mTrackThickness + thumbWidth/2 - 1, 0);
        if(smallThumbs)
        {
            thumb1Tip += Vec2(-1, 0);
            thumb2Tip += Vec2(-1, 0);
        }
	}
      
	// get fill rects
	full = trackRect;
	notFull1 = trackRect;
	notFull2 = trackRect;
	if (isHorizontal())
	{
		int l = trackRect.left();
		int r = trackRect.right();
		if (multi || mBipolar)
		{		
			notFull1.setWidth(fillPos1 - l);
			full.setLeft(fillPos1);
			full.setWidth(fillPos2 - fillPos1);
			notFull2.setLeft(fillPos2);
			notFull2.setWidth(r - fillPos2);
		}
		else
		{	
			notFull2.setBounds(0, 0, 0, 0);
			int fp = doDial1 ? fillPos1 : fillPos2;
			full.setWidth(fp - l);
			notFull1.setLeft(fp);
			notFull1.setWidth(r - fp);
		}
	} 
	else
	{
		int t = trackRect.top();
		int b = trackRect.bottom();
		if (multi || mBipolar)
		{		
			notFull1.setTop(t); 
			notFull1.setHeight(fillPos1 - t);
			full.setTop(fillPos2);
			full.setHeight(fillPos1 - fillPos2);
			notFull2.setTop(fillPos1); 
			notFull2.setHeight(b - fillPos1);
		}
		else
		{	
			notFull2.setBounds(0, 0, 0, 0);
			int fp = doDial1 ? fillPos1 : fillPos2;
			notFull1.setTop(t); 
			notFull1.setHeight(fp - t); 
			full.setTop(fp);
			full.setHeight(b - fp);
		}
	}

	switch (whichRect)
	{
		case MLDial::TrackRect:
			ret = trackRect;
			break;
		case MLDial::Thumb1Rect:		
			ret = thumbSize.withCenter(thumb1Center);
			break;
		case MLDial::Thumb2Rect:   
			ret = thumbSize.withCenter(thumb2Center);
			break;			
		case MLDial::Thumb1Field:		
			ret = trackRect;
			ret.setToUnionWith(thumbSize.withCenter(thumb1Center));
			break;
		case MLDial::Thumb2Field:   
			ret = trackRect;
			ret.setToUnionWith(thumbSize.withCenter(thumb2Center));
			break;
		case MLDial::Text1Rect:
			ret = text1Size.withCenter(thumb1Center);
			break;
		case MLDial::Text2Rect:
			ret = text2Size.withCenter(thumb2Center);
 			break;
		case MLDial::Tip1Rect:
			ret = thumb1Tip;
			break;
		case MLDial::Tip2Rect:
			ret = thumb2Tip;
			break;
		case MLDial::FillRect:
			ret = full;
			break;
		case MLDial::NoFillRect1:
			ret = notFull1;
			break;
		case MLDial::NoFillRect2:
			ret = notFull2;
			break;
	}
}

// resize this Dial and set the track rect, from which all the other
// parts are calculated
void MLDial::resizeWidget(const MLRect& b, const int u)
{
	Component* pC = getComponent();
	if(pC)
	{
 		MLWidget::resizeWidget(b, u);
     
		MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
		const MLRect uBounds = getGridBounds();
		bool multi = (isTwoOrThreeValued());
        bool smallThumbs = getFloatProperty("small_thumbs");
		
		// adapt vrect to juce rect
		MLRect bb = b;
		float left =  bb.left();
		float top =  bb.top();
		float width =  bb.width();
		float height = bb.height();	
		
		Vec2 bCenter = b.getCenter();		
		mMargin = myLookAndFeel->getSmallMargin() * u;
		mTrackThickness = (int)((float)kMLTrackThickness * u / 48.);
		mLineThickness = u/192.f;
		mTextSize = (float)u*myLookAndFeel->getDialTextSize(*this);
        mMaxNumberWidth = myLookAndFeel->calcMaxNumberWidth(mDigits, mPrecision, mDoSign, mValueDisplayMode)*mTextSize + 2.;
        mMaxNumberWidth &= (~1); // make even
        
        bool isSmall = (uBounds.height() <= 0.5f);
		
		// get component bounds
		//
		Rectangle<int> cBounds;		
		if (style == MLDial::Rotary)
		{
			float minDim = min(width, height);
			
			// get diameter.
			// TODO make this a clearer customization or different class
			// for small knobs with no numbers.
			// 
			int cx, cy;

			if (isSmall) 
			{
				mShadowSize = (int)(kMLShadowThickness*u/48.); // for small dials
				mDiameter = minDim - mShadowSize*2 - 2;
				mTickSize = 0.;
			}
			else
			{
				mShadowSize = (int)(kMLShadowThickness*u/32.);
				mDiameter = minDim * 0.75f;
				mTickSize = mDiameter * 0.1f;
			}
			
			cx = width/2;//mDiameter/2 + max(mShadowSize, (int)mTickSize);
			cy = height/2;//cx;
			mDialCenter = Vec2(cx, cy);

            // adjust size to make space for numbers
            float newLeft = left;
            if(!isSmall)
            {
                if (uBounds.height() <= 0.75f)
                {
                    height = cy*2;
                }
                else
                {
                    // cut off bottom
                    height = (float)cy*1.8f ;
                }
                            
                newLeft = bCenter.x() - cx;
                width = cx*2 + 1; 
                int rightHalf = width - cx;
                if (mMaxNumberWidth > rightHalf)
                {
                    width = cx + mMaxNumberWidth;
                }			
			}
            
			cBounds = Rectangle<int>(newLeft, top, (int)width, (int)height);	
			MLRect tr(newLeft, top, (int)width, (int)height);
			trackRect = tr;
			mRotaryTextRect = MLRect(cx, cy, mMaxNumberWidth, height - cy);
		}
 		else if(smallThumbs)
        // linear with fixed track size, small thumbs, border expands 
        {
            MLRect bb = b;
			mMaxNumberWidth = 0;
			mShadowSize = (int)(kMLShadowThickness*u/32.) & ~0x1;
			mTextHeight = mTrackThickness;
			mThumbMargin = mTrackThickness * 2;//(int)(myLookAndFeel->getSmallMargin()*u*0.75f);
            
            const int multi = (isTwoOrThreeValued());
            bool doDial1, doDial2;
            if (multi)
            {
                doDial1 = doDial2 = true;
            }
            else
            {
                doDial1 = mTopLeft;
                doDial2 = !doDial1;
            }

			// get track rect inside bounds and adjust bounds
            int t = mTrackThickness + 2;
            int nudge = 1;
			if (isHorizontal())
			{
                if(multi)
                {
                    // center track rect
                    // unimplemented
                }
                else if(doDial1)
                {
                    // unimplemented                   
                }
                else if(doDial2)
                {
                    trackRect = MLRect(0, 0, bb.width(), mTrackThickness);
                    bb.stretchWidth(t*2);
                    bb.setHeight(bb.getHeight() + t);
                    bb.translate(Vec2(0, nudge));
                    trackRect.translate(Vec2(t, 1));
                }
			}
			else
			{
                if(multi)
                {
                    // center track rect
                    // unimplemented
                }
                else if(doDial1)
                {
                    trackRect = MLRect(bb.getWidth() - mTrackThickness, 0, mTrackThickness, bb.height());
                    bb.stretchHeight(t*2);
                    bb.setWidth(bb.getWidth() + t);
                    bb.translate(Vec2(-t, 0));
                    trackRect.translate(Vec2(t, 0));
                    bb.translate(Vec2(-nudge, 0));
                    trackRect.translate(Vec2(-1, t));
                }
                else if(doDial2)
                {
                    // unimplemented
                }
			}
            
			cBounds = MLToJuceRectInt(bb);
        }
        else // normal linear, track shrinks to fit
		{
            MLRect bb = b;
			// max widths for dials are too long for horiz thumbs - correct
			// mMaxNumberWidth = ((int)(mMaxNumberWidth) & ~0x1) + 2 ;
			cBounds = MLToJuceRectInt(bb);
			
			mShadowSize = (int)(kMLShadowThickness*u/32.) & ~0x1;			
			mTextHeight = ((long)mTextSize - 1) | 0x1;
			mThumbMargin = (int)(myLookAndFeel->getSmallMargin()*u);
			int padding = mShadowSize + mTrackThickness/2;
			int thumbHeight = mTextHeight + mThumbMargin*2 ;
			Vec2 maxThumbSize(thumbHeight*4, thumbHeight + padding);

			// get track size
			if (isHorizontal())
			{
                trackRect = MLRect(0, 0, bb.width(), mTrackThickness);
                trackRect.stretchWidth(-maxThumbSize.x());
			}
			else
			{
				trackRect = MLRect(0, 0, mTrackThickness, bb.height());
                trackRect.stretchHeight(-maxThumbSize.y());
			}
			
			// get track position relative to bounds rect
			if (multi)
			{
				trackRect.centerInRect(bb.withTopLeft(0, 0));
			}
			else
			{
				if(mTopLeft)
				{
					if (isHorizontal())
					{
						trackRect.setBottom(bb.height() - padding);
					}
					else
					{
						trackRect.setRight(bb.width() - padding);
					}
				}
				else
				{
					if (isHorizontal())
					{
						trackRect.setTop(padding);
					}
					else
					{
						trackRect.setLeft(padding);
					}
				}
			}
		}

        trackRect = trackRect.getIntPart();
		pC->setBounds(cBounds);
		
        // get display scale
        int displayScale = 1;
        
 		// make compositing images
		if ((width > 0) && (height > 0))
		{
			int compWidth = getWidth();
			int compHeight = getHeight();
			mParameterImage = Image(Image::ARGB, compWidth + 1, compHeight + 1, true, SoftwareImageType());
			mParameterImage.clear(Rectangle<int>(0, 0, compWidth, compHeight), Colours::transparentBlack);	
			mThumbImage = Image(Image::ARGB, compWidth + 1, compHeight + 1, true, SoftwareImageType());
			mThumbImage.clear(Rectangle<int>(0, 0, compWidth, compHeight), Colours::transparentBlack);            
			mStaticImage = Image(Image::ARGB, compWidth*displayScale + 1, compHeight*displayScale + 1, true, SoftwareImageType());
			mStaticImage.clear(Rectangle<int>(0, 0, compWidth, compHeight), Colours::transparentBlack);
		}
		
		mParameterLayerNeedsRedraw = mThumbLayerNeedsRedraw = mStaticLayerNeedsRedraw = true;
		resized();
	}
}

void MLDial::beginGesture()
{
	if(mGestureInProgress)
	{
		endGesture();
	}
	
	sendAction("begin_gesture", getTargetPropertyName());
	mGestureInProgress = true;
}

void MLDial::endGesture()
{
	if(mGestureInProgress)
	{
		isMouseDown = false;
		dialBeingDragged = kNoDial;
		isMouseWheelMoving = false;
		restoreMouseIfHidden();
		sendAction("end_gesture", getTargetPropertyName());
		mGestureInProgress = false;
	}
}

#pragma mark MLDial::GestureTimer

MLDial::GestureTimer::GestureTimer(MLDial* pM) :
	mpOwner(pM)
{
}

MLDial::GestureTimer::~GestureTimer()
{
	stopTimer();
}

void MLDial::GestureTimer::timerCallback()
{
	stopTimer();
    mpOwner->endGesture();
}

void MLDial::GestureTimer::scheduleEndGesture(int timeFromNow)
{
	startTimer(timeFromNow);
}


