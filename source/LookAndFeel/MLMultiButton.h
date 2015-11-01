
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_MULTI_BUTTON_HEADER__
#define __ML_MULTI_BUTTON_HEADER__

#include "MLUI.h"
#include "MLButton.h"
#include "MLMultiSlider.h"
#include "MLProc.h"
#include "MLPositioner.h"
#include <vector>

class MLMultiButton :
	public Component,
	public MLWidget
{
public:
	MLMultiButton ();
	~MLMultiButton();
	
	void doPropertyChangeAction(MLSymbol property, const MLProperty& val);

    enum ColourIds
    {
        buttonOnColourId                = 0x1018000,  
		buttonOffColourId               = 0x1018001,
        textColourId                    = 0x1018002
    };
		
	void setNumButtons(int n);
	unsigned getNumButtons();
	void setFillColor(const Colour& c);	
	void paint (Graphics& g);
	
	//
	void mouseDown (const MouseEvent& e);
	void mouseUp (const MouseEvent&);
	void modifierKeysChanged (const ModifierKeys& modifiers);
//	void mouseMove (const MouseEvent& e);
//	void mouseExit (const MouseEvent& e);
	void mouseDrag (const MouseEvent& e);
	//

	void setSelectedValue (float newValue, int selector);

	void setGeometry(const MLPositioner::Geometry g) { mGeometry = g; }
	void setSizeFlags(const int f) { mSizeFlags = (MLPositioner::SizeFlags)f; }
	void setMargin(const float m) { mMarginFraction = m; }

	void resizeWidget(const MLRect& b, const int );

private:
	int getButtonUnderPoint(const Point<int>& p);
	int getButtonUnderMouse();

	int mNumButtons;
	int mButtonUnderMouse;
	int mCurrDragButton;
	float mCurrDragValue;
	bool mMovedInDrag;
	
	MLPositioner mPos;
	MLPositioner::Geometry mGeometry;
	MLPositioner::SizeFlags mSizeFlags;
	float mMarginFraction;
	float mLineThickness;
};

#endif // __ML_MULTI_BUTTON_HEADER__

