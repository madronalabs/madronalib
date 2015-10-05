
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_JUCE_APP_BORDER__
#define __ML_JUCE_APP_BORDER__

#include "MLWidget.h"
#include "MLAppView.h"
#include "MLBoundsConstrainer.h"

#pragma mark MLAppBorder

// a component for holding a view and resizing other components nicely.
//
class MLAppBorder : 
	public Component,
	public MLWidget
{
public:
    MLAppBorder();
    ~MLAppBorder();

	void addMainView(MLAppView* pC);
	void makeResizer(Component* targetComp);
	void paint (Graphics& g);
    void centerMainViewInWindow();
    void resized();

	void setGridUnits(int w, int h);
	void setContent(MLAppView* contentView);
	void setZoomable(bool z);

private:

    // (prevent copy constructor and operator= being generated..)
    MLAppBorder (const MLAppBorder&);
    const MLAppBorder& operator= (const MLAppBorder&);
	
	MLAppView *pMainView;
	ResizableCornerComponent* mpResizer; 
	MLBoundsConstrainer myConstrainer;	 	
	
	// grid-based view resizing things. 
	int mGridUnitsX;
	int mGridUnitsY;	
	
	MLRect mBorderRect;
	
	bool mZoomable;
	
};

#endif // __ML_JUCE_APP_BORDER__