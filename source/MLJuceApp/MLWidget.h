
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#include "MLUI.h"
#include "MLVectorDeprecated.h"
#include "MLSymbol.h"
#include "MLSignal.h"
#include "MLPropertySet.h"
#include "JuceHeader.h"

class MLWidgetContainer;

// adapter for private UI stuff to Juce UI stuff.  as we use less of JUCE
// this can incorporate our own component class.
//
class MLWidget :
	public MLPropertySet,
	public MLPropertyListener,
	public juce::OpenGLRenderer
{
friend class MLWidgetContainer;
friend class MLAppView;
public:
	class Listener
	{
		public:
		virtual ~Listener() {}
		virtual void handleWidgetAction (MLWidget*, ml::Symbol action, ml::Symbol target, const MLProperty& val) = 0;
	};

	MLWidget(MLWidget* pC);
	virtual ~MLWidget();
	
	// MLPropertyListener methods.
	// a Widget's local properties must be set in Immediate mode. There is no timer to propagate changes.
	virtual void doPropertyChangeAction(ml::Symbol param, const MLProperty& newVal) override;
	
	// in order to function, a Widget's Component must get set.
	void setComponent(Component* pC) { pComponent = pC; }
	Component* getComponent() const { return pComponent; }
	
	// add a Listener to our list.
	void addListener (MLWidget::Listener* const p);
	
	// send an action to all of our listeners.
	void sendAction(ml::Symbol m, ml::Symbol target, const MLProperty& val = MLPropertySet::nullProperty);

	// return true if this Widget contains other Widgets. Used to search recursively for Widgets.
	virtual bool isWidgetContainer(void) { return false; }
	
	// recursive search for a Widget contained within this one.
	virtual MLWidget* getWidget(ml::Symbol name) { return nullptr; }
	
	// A signal viewer, not required. This is called repeatedly to view a Signal.
	virtual void viewSignal(ml::Symbol, const MLSignal&, int frames, int voices) {}

	// TODO widgets should not own GL contexts
	void setupGL();
    
    // OpenGLRenderer methods to use if we have one
    virtual void newOpenGLContextCreated() override {}
    virtual void openGLContextClosing() override {}
    virtual void renderOpenGL() override {}
    
	void setGridBounds(const MLRect& p);
	const MLRect& getGridBounds() const;
	
	void setWidgetBounds(const MLRect& p);
	
	// bounds in immediately enclosing component
	MLRect getWidgetBounds();
	
	// bounds relative to widget itself, so topleft will be (0,0)
	MLRect getWidgetLocalBounds();
	
	// bounds of widget relative to top level window
	MLRect getWidgetBoundsInWindow();
	
	// bounds of top level window
	MLRect getTopLevelWindowBounds();
    
    float getRenderingScale() const;
    int getBackingLayerWidth() const;
    int getBackingLayerHeight() const;
	
	// TODO remove, in Context only
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
	
	const ml::Symbol& getWidgetName() { return mName; }

	void setWidgetVisible(bool v);
	void setWidgetEnabled(bool v);
    
	ml::Symbol getTargetPropertyName() { return mTargetPropertyName; }
	void setTargetPropertyName(ml::Symbol p) { mTargetPropertyName = p; }
	
	// this can't return a widgetContainer because that type is incomplete.
	// this is a good argument for keeping all the container stuff external (non-intrusive).
	// that and reusing it.
	MLWidget* getContainer() const { return mpContainer; }

	// TEMP working around widget / view hierarchy problems
	void setContainer(MLWidget* c) { mpContainer = c; }
	void setWidgetName(const ml::Symbol& n) { mName = n; }

protected:
	void setWidgetGridUnitSize(const int w) { mGridUnitSize = w; }

	std::vector<MLWidget::Listener*> mpListeners;
	
	// JUCE GL context, if we have one. Owned.
	juce::OpenGLContext* mpGLContext;

private:
	// JUCE component we are using. Not owned. Needs to be set up in ctor of every subclass!
	Component* pComponent;
	
	// must point to enclosing context.
	// TODO this should not be a Widget but a widget container type!
	MLWidget* mpContainer;
	
	ml::Symbol mName;
	
	// name of the target property of Listeners we would like to affect.
	// if a Widget has multiple parts, like a Multislider, this property name can get
	// a numerical or symbolic suffix to indicate what part was changed.
	ml::Symbol mTargetPropertyName;
	
	// this is the size of drawn widget parts compared to the usual size.
	// normally 1.0, set to 0.75 or similar for smaller labels, buttons, etc. 
	float mSize;
	
	// bounds and unit size on the grid system.
	MLRect mGridBounds;
	int mGridUnitSize;
	
	// offset for an external label if there is one
	Vec2 mLabelOffset;
	
	bool mWantsResizeLast;
};
