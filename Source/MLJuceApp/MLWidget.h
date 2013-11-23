
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_WIDGET_H__
#define __ML_WIDGET_H__

#include "MLUI.h"
#include "MLVector.h"
#include "MLSymbol.h"
#include "MLSignal.h"

class MLWidgetContainer;

// adapter for private UI stuff to Juce UI stuff.  as we use less of JUCE
// this can incorporate our own component class.
//
class MLWidget
{
friend class MLWidgetContainer;
friend class MLAppView;
public:
	MLWidget();
	virtual ~MLWidget();

	virtual bool isWidgetContainer(void) { return false; }
	virtual MLWidget* getWidget(MLSymbol ) { return nullptr; }
	
    // attributes
	virtual void setAttribute(MLSymbol attr, float val);
	virtual void setStringAttribute(MLSymbol attr, const std::string& val);
	virtual void setSignalAttribute(MLSymbol attr, const MLSignal& val);
    virtual void setColorAttribute(MLSymbol attr, juce::Colour val);

	float getAttribute(MLSymbol attr) const;
	const std::string& getStringAttribute(MLSymbol attr) const;
    const MLSignal& getSignalAttribute(MLSymbol attr) const;
    juce::Colour getColorAttribute(MLSymbol attr) const;

	// A signal viewer, not required. This is called repeatedly to view 
	// a dynamic Signal, as opposed to a signal Parameter.
	virtual void viewSignal(MLSymbol, const MLSignal&, int) {}

	// in order to function, a widget's Component must get set!
	//
	void setComponent(Component* pC) { pComponent = pC; }
	Component* getComponent() const { return pComponent; }

	void setGridBounds(const MLRect& p);
	const MLRect& getGridBounds() const;
	
	void setWidgetBounds(const MLRect& p);
	MLRect getWidgetBounds();
	MLRect getWidgetLocalBounds();
	MLRect getWidgetWindowBounds();
    
    double getBackingLayerScale() const;    
    int getBackingLayerWidth() const { return (int)(getBackingLayerScale() * pComponent->getWidth()); }
    int getBackingLayerHeight() const  { return (int)(getBackingLayerScale() * pComponent->getHeight()); }

	int getWidgetGridUnitSize(void) const { return mGridUnitSize; }
	void setSizeMultiplier(float f) { mSize = f; }
	float getSizeMultiplier() { return mSize; }
	virtual float getLabelVerticalOffset() { return 1.f; }

	// here we set the component bounds from the grid-based bounds. 
	// can be overridden so that widgets can calculate their own margins.
	// also gives the widget an opportunity to resize its internals.
	// labels resize their fonts here for example. 
	virtual void resizeWidget(const MLRect& b, const int unitSize);
	bool wantsResizeLast() { return mWantsResizeLast; }
	void setWantsResizeLast(bool t) { mWantsResizeLast = t; }

	void setLabelOffset(const Vec2& p) { mLabelOffset = p; }
	const Vec2& getLabelOffset() { return mLabelOffset; }
	
	const MLSymbol& getWidgetName() { return mName; }

	void setWidgetVisible(bool v);
	
	void enterPaint(); // for debugging
	
protected:
	void setWidgetName(const MLSymbol& n) { mName = n; }
	void setWidgetGridUnitSize(const int w) { mGridUnitSize = w; }
	

private:
	MLSymbol mName;
	
	// this is the size of drawn widget parts compared to the usual size.
	// normally 1.0, set to 0.75 or similar for smaller labels, buttons, etc. 
	float mSize;
	
	// bounds and unit size on the grid system.
	MLRect mGridBounds;
	int mGridUnitSize;
	
	// offset for an external label if there is one
	Vec2 mLabelOffset;
	
	// JUCE component we are using.  Needs to be set up in ctor of every subclass!
	Component* pComponent;
	
	bool mWantsResizeLast;
	
	std::map<MLSymbol, float> mAttributes;
	std::map<MLSymbol, std::string> mStringAttributes;
	std::map<MLSymbol, MLSignal> mSignalAttributes;
};

#endif // __ML_WIDGET_H__