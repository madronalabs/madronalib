
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_BUTTON_HEADER__
#define __ML_BUTTON_HEADER__

#include "MLUI.h"
#include "MLWidget.h"
#include "MLSymbol.h"
#include "MLLabel.h"

class MLButton : 
	public Component,
	public MLWidget
{
friend class MLLookAndFeel;
public:
	MLButton (const String& label);
	MLButton ();
	
    enum ColourIds
    {
        buttonOnColourId                = 0x1009000,  
		buttonOffColourId               = 0x1009001,
        textColourId                    = 0x1009002
    };
	
	void paint (Graphics& g);
	
	// set all colors calculated from fill color
	virtual void setFillColor(const Colour c);

	void mouseDown (const MouseEvent& e);
	void mouseUp (const MouseEvent& e);
	void mouseDrag (const MouseEvent&);
	
    virtual void clicked();
	void resizeWidget(const MLRect& b, const int u);
	void setLineThickness(float f);
	void setToggleValues(float, float);

	// MLPropertyListener
	virtual void doPropertyChangeAction(MLSymbol property, const MLProperty& newVal);
	
protected:
	bool mDoRollover;
	Image mImage;
	float mMargin;
	MLPoint mImageOffset;
	MLPoint mLabelOffset;
	
	float mOffValue;
	float mOnValue;
    float mLineThickness;

	bool mOver;
	bool mDown;
	bool mToggleState;
	bool mTriggerOnMouseDown;
	
private:

};


#endif // __ML_BUTTON_HEADER__