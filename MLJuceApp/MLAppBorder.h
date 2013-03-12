
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_JUCE_APP_BORDER__
#define __ML_JUCE_APP_BORDER__

#include "MLWidget.h"
#include "MLAppView.h"


class MLBoundsConstrainer  : public ComponentBoundsConstrainer
{
public:
    MLBoundsConstrainer();
	~MLBoundsConstrainer();

	void setTitleBarHeight(int t) { mTitleBarHeight = t; }
	
    //==============================================================================
    /** This callback changes the given co-ordinates to impose whatever the current
        constraints are set to be.

        @param bounds               the target position that should be examined and adjusted
        @param previousBounds       the component's current size
        @param limits               the region in which the component can be positioned
        @param isStretchingTop      whether the top edge of the component is being resized
        @param isStretchingLeft     whether the left edge of the component is being resized
        @param isStretchingBottom   whether the bottom edge of the component is being resized
        @param isStretchingRight    whether the right edge of the component is being resized
    */
	/*
    void checkBounds (MLRect& bounds,
		const MLRect& previousBounds,
		const MLRect& limits,
		bool isStretchingTop,
		bool isStretchingLeft,
		bool isStretchingBottom,
		bool isStretchingRight);
	*/
		
private:
		int mTitleBarHeight;
};

// --------------------------------------------------------------------------------
#pragma mark MLAppBorder
// a component for resizing other components nicely
//
class MLAppBorder : 
	public Component
{
public:
    MLAppBorder();
    ~MLAppBorder();

	void addMainView(MLAppView* pC);
	void makeResizer(Component* targetComp);
	void paint (Graphics& g);
    void centerMainViewInWindow();
    void resized();

	void setGridUnits(double w, double h);
	int getGridUnitsX() { return mGridUnitsX; }
	int getGridUnitsY() { return mGridUnitsY; }

	void setContent(MLAppView* contentView);

private:

    // (prevent copy constructor and operator= being generated..)
    MLAppBorder (const MLAppBorder&);
    const MLAppBorder& operator= (const MLAppBorder&);
	
	MLAppView *pMainView;
	
	// grid-based view resizing things. 
	float mGridUnitsX;
	float mGridUnitsY;	
	ResizableCornerComponent* mpResizer; 	 	
    MLBoundsConstrainer myConstrainer;	

};

#endif // __ML_JUCE_APP_BORDER__