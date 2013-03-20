
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __EXAMPLEVIEW_H__
#define __EXAMPLEVIEW_H__

#include "JuceHeader.h"

#include "MLExampleBinaryData.h"

#include "MLPluginProcessor.h"
#include "MLResponder.h"
#include "MLReporter.h"
#include "MLButton.h"
#include "MLDrawableButton.h"
#include "MLTextButton.h"
#include "MLInputProtocols.h"
#include "MLMultiButton.h"
#include "MLMultiSlider.h"
#include "MLEnvelope.h"
#include "MLGraph.h"
#include "MLAppView.h"
#include "MLPluginView.h"
#include "MLPageView.h"

typedef std::tr1::shared_ptr<Drawable> DrawablePtr;

const int kExampleViewUnitsX = 12;
const int kExampleViewUnitsY = 7; 

// --------------------------------------------------------------------------------
#pragma mark header view

class ExampleHeaderView : 
	public MLAppView
{
public:
    ExampleHeaderView(MLPluginProcessor* const pProcessor, MLResponder* pResp, MLReporter* pRep);
    ~ExampleHeaderView();
	void paint(Graphics& g);

private:
	MLDrawableButton* mpPrevButton;
	MLDrawableButton* mpNextButton;
};


#pragma mark main view

class MLExampleView : 
	public MLPluginView,
	public MLResponder
{
public:
	const Colour bg1;
	const Colour bg2;

    MLExampleView(MLPluginProcessor* const pProcessor, MLPluginController* pC);
    ~MLExampleView();
	
	void setAttribute(MLSymbol attr, float val);
	void adaptSeqRateUI(bool t);
	void adaptInputProtocolUI(int p);
	void setupColors();
	void timerCallback();

private:
    // (prevent copy constructor and operator= being generated..)
    MLExampleView (const MLExampleView&);
    const MLExampleView& operator= (const MLExampleView&);
	
	void goToPage (int page);

	// components
	ExampleHeaderView* mpHeader;	
	MLDrawableButton* mpPrevButton;
	MLDrawableButton* mpNextButton;
	MLPageView* mpPages;

	Rectangle<int> mPageRect;
	
	// icons
	DrawablePtr mPrevArrow;
	DrawablePtr mNextArrow;
};


#endif // __EXAMPLEVIEW_H__