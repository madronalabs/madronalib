
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_MULTI_SLIDER_HEADER__
#define __ML_MULTI_SLIDER_HEADER__

#include "MLUI.h"
#include "MLDial.h"
#include "MLProc.h"
#include "MLPositioner.h"
#include <vector>

class MLMultiSlider : 
	public Component,
	public MLWidget
{
public:
	MLMultiSlider ();
	~MLMultiSlider();
		
	void doPropertyChangeAction(MLSymbol property, const MLProperty& val);
	
    enum ColourIds
    {
        fillColor						= 0x1017000,
        indicatorColor					= 0x1017001,
        glowColor						= 0x1017002,
        trackFullLightColor				= 0x1017003,
        trackFullDarkColor				= 0x1017004,
        trackEmptyDarkColor				= 0x1017006
    };

	void setNumSliders(int n);
	int getNumSliders() const;
	void setRange(float a, float b, float c);	

	void setFillColor(const Colour& c);
	const MLRect getActiveRect () const;
	int getSliderWidth() const;
	void paint (Graphics& g);
	//
	float constrainedValue (float value) const throw();
	float snapValue (float attemptedValue, const bool);
	float valueToProportionOfLength(float v);
	float proportionOfLengthToValue(float l);
	//
	void mouseDown (const MouseEvent& e);
	void mouseUp (const MouseEvent&);
	void modifierKeysChanged (const ModifierKeys& modifiers);
	void mouseMove (const MouseEvent& e);
	void mouseExit (const MouseEvent& e);
	void mouseDrag (const MouseEvent& e);
    void mouseWheelMove (const MouseEvent& event, const MouseWheelDetails& wheel);
	//
	void setWave(int w);
	void sendSliderAction (float newValue, int selector);
 
	int getCurrentDragSlider() {return mCurrDragSlider;}

	void resizeWidget(const MLRect& b, const int );
	
	void beginGesture();
	void endGesture();
	
protected:
	void triggerChangeMessage (const bool synchronous);
    void handleAsyncUpdate();
 	
private:
	int getSliderUnderPoint(const Vec2& p);
	int getSliderUnderMouse();

	int mNumSliders;
	MLRange mRange; 
	float mInterval;
	float mZeroThreshold;
	int mSliderUnderMouse;
	int mCurrDragSlider;
	float mCurrDragValue;
	bool mDoRollover;
	Vec2 mMousePos;

	bool mVertical;
	
	MLPositioner mPos;
	MLPositioner::Geometry mGeometry;
	MLPositioner::SizeFlags mSizeFlags;
	float mMarginFraction;
	
	bool isMouseWheelMoving;
	bool mGestureInProgress;
	
	// TODO write a Timer class. juce::Timer is the only reason Juce is needed here. temporary.
	class GestureTimer : public juce::Timer
	{
	public:
		GestureTimer(MLMultiSlider*);
		~GestureTimer();
		void timerCallback();
	private:
		MLMultiSlider* mpOwner;
	};
	std::unique_ptr<GestureTimer> mpTimer;
};

#endif // __ML_MULTI_SLIDER_HEADER__

