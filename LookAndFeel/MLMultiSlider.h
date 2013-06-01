
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
	public MLWidget,
	public MLParamSetter,
    protected AsyncUpdater
{
public:
	MLMultiSlider ();
	~MLMultiSlider();
		
	class Listener 
	{
	public:
		virtual ~Listener() {}
		virtual void multiSliderValueChanged (MLMultiSlider* dial, int idx) = 0;
	};

    void setListener (MLMultiSlider::Listener* const listener) throw();

	void setAttribute(MLSymbol attr, float val);
	
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
	unsigned getNumSliders();
	void setRange(float a, float b, float c);	

	void setFillColor(const Colour& c);
	const MLRect getActiveRect () const;
	unsigned getSliderWidth() const;
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
//	void setIndexedValue (unsigned index, float newValue, const bool sendUpdateMessage, const bool sendMessageSynchronously);
	float getValue(unsigned index);
	void setWave(unsigned w);
	void setSelectedValue (float newValue, int selector, const bool sendUpdateMessage = true, const bool sendMessageSynchronously = false);
 
	int getCurrentDragSlider() {return mCurrDragSlider;}

	void resizeWidget(const MLRect& b, const int );
	
protected:
	void triggerChangeMessage (const bool synchronous);
    void handleAsyncUpdate();
 	
private:
	int getSliderUnderPoint(const Vec2& p);
	int getSliderUnderMouse();

	MLMultiSlider::Listener* mpListener;
	
	std::vector<float> mSliderValues;
	std::vector<bool> mValueNeedsUpdate;
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
};

#endif // __ML_MULTI_SLIDER_HEADER__

