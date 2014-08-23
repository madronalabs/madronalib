
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_WIDGET_H__
#define __ML_WIDGET_H__

#include "MLUI.h"
#include "MLVector.h"
#include "MLSymbol.h"
#include "MLSignal.h"
#include "MLProperty.h"

class MLWidgetContainer;

// adapter for private UI stuff to Juce UI stuff.  as we use less of JUCE
// this can incorporate our own component class.
//
class MLWidget :
    public OpenGLRenderer, // WAT
	public MLPropertySet,
	public MLPropertyListener
{
friend class MLWidgetContainer;
friend class MLPropertyView;
friend class MLAppView;
public:
	MLWidget();
	virtual ~MLWidget();
	
	class Listener
	{
		public:
		virtual ~Listener() {}
		virtual void handleWidgetAction (MLWidget*, MLSymbol action, MLSymbol target, const MLProperty& val) = 0;
	};

	// in order to function, a Widget's Component must get set!
	void setComponent(Component* pC) { pComponent = pC; }
	Component* getComponent() const { return pComponent; }
	
	// add a Listener to our list.
	void addListener (MLWidget::Listener* const p);
	
	// send an action to all of our listeners.
	void sendAction(MLSymbol m, MLSymbol target, const MLProperty& val = MLPropertySet::nullProperty);

	// return true if this Widget contains other Widgets. Used to search recursively for Widgets.
	virtual bool isWidgetContainer(void) { return false; }
	
	// recursive search for a Widget contained within this one.
	virtual MLWidget* getWidget(MLSymbol name) { return nullptr; }
	
	// A signal viewer, not required. This is called repeatedly to view a Signal.
	virtual void viewSignal(MLSymbol, const MLSignal&, int frames, int voices) {}

    void setupGL(Component* pC);
    OpenGLContext* getGLContext() { return pGLContext; }
    
    // OpenGLRenderer methods to use if we have one
    virtual void newOpenGLContextCreated() {}
    virtual void openGLContextClosing() {}
    virtual void renderOpenGL() {}
    
	void setGridBounds(const MLRect& p);
	const MLRect& getGridBounds() const;
	
	void setWidgetBounds(const MLRect& p);
	MLRect getWidgetBounds();
	MLRect getWidgetLocalBounds();
	MLRect getWidgetWindowBounds();
    
    float getRenderingScale() const;
    int getBackingLayerWidth() const;
    int getBackingLayerHeight() const;
    
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
    
	MLSymbol getTargetPropertyName() { return mTargetPropertyName; }
	void setTargetPropertyName(MLSymbol p) {  mTargetPropertyName = p; }
	
	// MLPropertyListener methods
	virtual void doPropertyChangeAction(MLSymbol param, const MLProperty& newVal) {}
	
protected:
	void setWidgetName(const MLSymbol& n) { mName = n; }
	void setWidgetGridUnitSize(const int w) { mGridUnitSize = w; }

	std::list<MLWidget::Listener*> mpListeners;

private:
	MLSymbol mName;
	MLSymbol mTargetPropertyName;
	
	// this is the size of drawn widget parts compared to the usual size.
	// normally 1.0, set to 0.75 or similar for smaller labels, buttons, etc. 
	float mSize;
	
	// bounds and unit size on the grid system.
	MLRect mGridBounds;
	int mGridUnitSize;
	
	// offset for an external label if there is one
	Vec2 mLabelOffset;
	
	// JUCE component we are using. Not owned. Needs to be set up in ctor of every subclass!
	Component* pComponent;
    
    // JUCE GL context, if we have one. Owned.
    OpenGLContext* pGLContext;
	
	bool mWantsResizeLast;
};

#endif // __ML_WIDGET_H__