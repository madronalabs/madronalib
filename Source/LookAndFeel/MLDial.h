
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// Portions of this software originate from JUCE, 
// copyright 2004-2013 by Raw Material Software ltd.
// JUCE is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#ifndef __ML_DIAL_HEADER__
#define __ML_DIAL_HEADER__

#include "MLUI.h"
#include "MLWidget.h"
#include "MLProc.h"
#include "MLParameter.h"
#include "MLProcContainer.h"
#include "MLLabel.h"

class MLDialDetent
{
public:
	MLDialDetent(const float v, const float w) : mValue(v), mWidth(w) {}
	~MLDialDetent(){}	
	float mValue;
	float mWidth;
};

class MLDial  : 
	public Component,
	public MLWidget
{
friend class MLLookAndFeel;
public:
    MLDial ();
    ~MLDial();
	
	class Listener 
	{
	public:
		virtual ~Listener() {}
		virtual void dialDragStarted (MLDial* dial) = 0;
		virtual void dialValueChanged (MLDial* dial) = 0;
		virtual void dialDragEnded (MLDial* dial) = 0;
	};
	
	void setListener (MLDial::Listener* const l);
	void setAttribute(MLSymbol attr, float val);
	
	enum DialStyle
    {
        LinearHorizontal,       /**< A traditional horizontal dial. */
        LinearVertical,         /**< A traditional vertical dial. */
        LinearBar,              /**< A horizontal bar dial with the text label drawn on top of it. */
        Rotary,                 /**< A rotary control that you move by dragging the mouse in a circular motion, like a knob.
                                     @see setRotaryParameters */
        RotaryHorizontalDrag,   /**< A rotary control that you move by dragging the mouse left-to-right.
                                     @see setRotaryParameters */
        RotaryVerticalDrag,     /**< A rotary control that you move by dragging the mouse up-and-down.
                                     @see setRotaryParameters */
		TwoValueHorizontal,     /**< A horizontal dial that has two thumbs instead of one, so it can show a minimum and maximum value.
                                     @see setMinValue, setMaxValue */
        TwoValueVertical,       /**< A vertical dial that has two thumbs instead of one, so it can show a minimum and maximum value.
                                     @see setMinValue, setMaxValue */

        ThreeValueHorizontal,   /**< A horizontal dial that has three thumbs instead of one, so it can show a minimum and maximum
                                     value, with the current value being somewhere between them.
                                     @see setMinValue, setMaxValue */
        ThreeValueVertical,     /**< A vertical dial that has three thumbs instead of one, so it can show a minimum and maximum
                                     value, with the current value being somewhere between them.
                                     @see setMinValue, setMaxValue */
        MultiHorizontal,  
        MultiVertical   
	};
 
    enum WhichValue
	{
        mainValue,		
        minValue,   
        maxValue
	};
	
    enum WhichDial
	{
        kMainDial,		
        kMinDial,   
        kMaxDial,
		kTrackDial,
		kNoDial
	};
	
    enum DialRect
	{
        Thumb1Rect,		
        Thumb2Rect,   
        Thumb1Field,		
        Thumb2Field,   
		TrackRect,
		Text1Rect,
		Text2Rect,
		Tip1Rect,  // just Vec2 tip for thumb
		Tip2Rect,
		FillRect,
		NoFillRect1,
		NoFillRect2
	};
	
    enum WhichCorner
	{
        BottomLeft,		
        BottomRight,   
        TopLeft,
		TopRight,
	};
	
	
	WhichDial getDialBeingDragged() {return dialBeingDragged;}
	WhichDial getDialToDrag() {return dialToDrag;}
	bool isOverTrack() {return mOverTrack;}

    void setDialStyle (const MLDial::DialStyle newStyle);
    MLDial::DialStyle getDialStyle() const throw() { return style; }

	// set corner for text area.
	void setRotaryDialCorner(WhichCorner k);

    void setRotaryParameters (const float startAngleRadians,
                              const float endAngleRadians,
                              const bool stopAtEnd);

    void setMouseDragSensitivity (const int distanceForFullScaleDrag);

    void setVelocityBasedMode (const bool isVelocityBased) throw();

    void setVelocityModeParameters (const float sensitivity = 1.0,
                                    const int threshold = 1,
                                    const float offset = 0.0,
                                    const bool userCanPressKeyToSwapMode = true) throw();


    float getValue() const throw();
	void setRange (const float newMinimum,
				const float newMaximum,
				const float newInterval = 0.,
				const float zeroThresh = -1000000000.,
				JucePluginParamWarpMode warpMode = kJucePluginParam_Linear);

	void setDefault (const float newDefault);
    float getMaximum() const throw()                                       { return maximum; }
    float getMinimum() const throw()                                       { return minimum; }
    float getInterval() const throw()                                      { return interval; }
    float getMinValue() const throw();
    float getMaxValue() const throw();

	void setDoubleClickReturnValue (const bool isDoubleClickEnabled,
                                    const float valueToSetOnDoubleClick) throw();

	float getDoubleClickReturnValue (bool& isEnabled) const throw();
    void setChangeNotificationOnlyOnRelease (const bool onlyNotifyOnRelease) throw();

    void setDialSnapsToMousePosition (const bool shouldSnapToMouse) throw();

	void setScrollWheelEnabled (const bool enabled) throw();

	WhichDial getThumbBeingDragged() const throw()        { return dialBeingDragged; }

    virtual void valueChanged();
    virtual int valueChanged (float) { jassertfalse; return 0; }
	virtual void findDialToDrag(const int x, const int y);
	virtual void findDialToDrag(const MouseEvent& e);
    virtual float getValueFromText (const String& text);
    virtual const String getTextFromValue (float value);
    void setTextValueSuffix (const String& suffix);

    virtual float proportionOfLengthToValue (float proportion);

    virtual float valueToProportionOfLength (float value) const;

    float getPositionOfValue (const float value);

    virtual float snapValue (float attemptedValue, const bool userIsDragging);
	unsigned nearestDetent (float attemptedValue) const;

    bool isHorizontal() const throw();
    bool isVertical() const throw();
    bool isTwoOrThreeValued() const throw();
	bool isTwoValued() const throw();
	bool isMultiValued() const throw();
	
    enum ColourIds
    {
        fillColor			= 0x1001200,  
        trackFillColor		= 0x1001201,                                                        
        indicatorColor		= 0x1001202,  
        glowColor			= 0x1001203,  
        thumbGlowColor		= 0x1001204, 
        trackLightColor		= 0x1001205,  
        trackDarkColor		= 0x1001206,  
    };
	
#pragma mark -
	
	const MLPoint getDialCenter() const;
	Vec2 mDialCenter;
	
	void setTicks (int t);
	void setTicksOffsetAngle (float t);

	void setFillColor (const Colour& color);
	void setTopLeft(const bool n) {mTopLeft = n;}
	void setDoNumber(const bool n) {mDoNumber = n;}
	void setBipolar(const bool n) {mBipolar = n;}

	void addDetent(const float value, const float width = 0.5f);
	void snapToDetents(const bool snap);
	
	void setTicks(const unsigned t) {mTicks = t;}
	void setDrawThumb(const bool d) {mDrawThumb = d;}
	
	void setWarpMode(const JucePluginParamWarpMode b);
	
	void setValueDisplayMode(MLValueDisplayMode m) {mValueDisplayMode = m;}
	void setHilight (bool h);
	void setHilightColor (const Colour& color);
	
	// get whole area for specified thumb of the dial.
	void getDialRect (MLRect& r, 
		const MLDial::DialRect whichRect,
		const float dialPos, const float minDialPos, const float maxDialPos) ;

	WhichDial getRectOverPoint(const MouseEvent& e);
	WhichDial getRectOverPoint(const int x, const int y);		
	
	
	void sizeChanged();
	void visibilityChanged();
	
	float getLabelVerticalOffset() { return 0.875f; }

protected:
	float getValueOfDial(WhichDial s);
	void sendValueOfDial(WhichDial s, float val);
	void setValueOfDial(WhichDial s, float val, bool sendUpdate = false, bool sync = false);
    void setSelectedValue (float newValue, int valSelector);

	
    void repaintAll ();
	virtual void paint (Graphics& g);
	void drawLinearDial (Graphics& g, int rx, int ry, int rw, int rh,
        float dialPos, float minDialPos, float maxDialPos);
	void drawLinearDialOverlay (Graphics& g, int rx, int ry, int rw, int rh,
        float dialPos, float minDialPos, float maxDialPos);
	void drawRotaryDial (Graphics& g, int rx, int ry, int rw, int rh, float dialPos);
	void drawRotaryDialOverlay (Graphics& g, int rx, int ry, int rw, int rh, float dialPos);
    
    void moved();
    virtual void resized();
    
    void mouseDown (const MouseEvent& e);
    void mouseUp (const MouseEvent& e);

	bool collectMouseMotion(int dp, int dt);
	float getNextValue(float oldVal, int dp, bool doFineAdjust, int stepSize);

    void mouseDrag (const MouseEvent& e);		
    void mouseDoubleClick (const MouseEvent& e);
	void mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& wheel);

	void hideMouse();
    void modifierKeysChanged (const ModifierKeys& modifiers);
    void lookAndFeelChanged();
    void enablementChanged();
    void colourChanged();
    void sendDragStart();
	void sendDragEnd();
	inline void endDrag() { dialBeingDragged = kNoDial; }

    float constrainedValue (float value) const throw();

    WhichDial dialBeingDragged;
	WhichDial dialToDrag;

	void resizeWidget(const MLRect& b, const int u);
		
    MLDial (const MLDial&); // no copy
    const MLDial& operator= (const MLDial&);

    float getLinearDialPos (const float value);
    void restoreMouseIfHidden();
    bool incDecDragDirectionIsHorizontal() const throw();
	
    float currentValue, valueMin, valueMax;
    float minimum, maximum, interval, doubleClickReturnValue;
    float valueWhenLastDragged, valueOnMouseDown;
    float rotaryStart, rotaryEnd;
    int numDecimalPlaces;
	bool mOverTrack;
	
	Time mLastDragTime, mLastWheelTime;
	int mLastDragX, mLastDragY;
	float mFilteredMouseSpeed;
	int mMouseMotionAccum;

    int pixelsForFullDragExtent;
    MLDial::DialStyle style;
	MLValueDisplayMode mValueDisplayMode;

    MLRect trackRect;
    String textSuffix;

    bool doubleClickToValue : 1;
    bool isVelocityBased : 1, userKeyOverridesVelocity : 1, rotaryStop : 1;
    bool incDecButtonsSideBySide : 1, sendChangeOnlyOnRelease : 1, popupDisplayEnabled : 1;
    bool menuShown : 1, mouseWasHidden : 1;
    bool scrollWheelEnabled : 1, snapsToMousePos : 1;

	// colors
	Colour			mFillColor;
	Colour			mGlowColor;
	Colour			mThumbGlowColor;
	Colour			mIndicatorColor;
	Colour			mTrackFillColor;
	Colour			mTrackDarkColor;
	Colour			mHilightColor;
	
	// geometry and behavior
	JucePluginParamWarpMode mWarpMode;
	float			mZeroThreshold;
	bool			mTopLeft;
	bool			mDrawThumb;
	
	MLRect mRotaryTextRect;

	bool mDoSign;
	bool mDoNumber;
	int	mDigits, mPrecision;
	bool mBipolar;
	
	float mTextSize;
	int mMaxNumberWidth;

	int mTrackThickness; // for track of linear dials. 	
	float mLineThickness;
	int mTicks;
	float mTicksOffsetAngle;
	int mDiameter;
	float mMargin;
	float mTickSize;
	int mShadowSize;
	
	int mTextHeight;
	int mThumbMargin;
		
	std::vector<MLDialDetent> mDetents;	
	bool mSnapToDetents;
	int mCurrentDetent;
	bool mPrevLFDrawNumbers;
		
	// redraw flags
	bool mParameterLayerNeedsRedraw;
	bool mStaticLayerNeedsRedraw;
	bool mThumbLayerNeedsRedraw;

	// image layers
	Image mParameterImage;
	Image mStaticImage;
	Image mThumbImage;
	
	MLDial::Listener* mpListener;
};

#endif // __ML_DIAL_HEADER__