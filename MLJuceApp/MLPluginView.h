
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_PLUGINVIEW_H__
#define __ML_PLUGINVIEW_H__

#include "MLPluginController.h"
#include "MLAppView.h"
#include "MLPluginProcessor.h"
#include "MLAppView.h"
#include "MLPageView.h"
#include "MLResponder.h"
#include "MLDrawing.h"

#include "MLStepDisplay.h"
#include "MLScopeDial.h"
#include "MLScope.h"

// view for looking at changes of plugins. 
//
class MLPluginView : 
	public MLAppView
{
public:
	MLPluginView(MLPluginProcessor* const pProcessor, MLPluginController* pR);
    ~MLPluginView();
	
//	void paint(Graphics& g);

	void setGridUnits(double gx, double gy);
	void setContent(MLAppView* contentView);

	MLPluginProcessor* getProcessor() const { return mpProcessor; } // TO GO

	void addSignalView(MLSymbol p, MLWidget* w, MLSymbol attr, int size);

	// component setup 
	//
	MLPluginView* addSubView(const MLRect & r, const MLSymbol name);

	MLDial* addDial(const char * displayName, const MLRect & r, const MLSymbol paramName, 
		const Colour& color = defaultColor);	
	MLScopeDial* addScopeDial(const char * displayName, const MLRect & r, 
		const MLSymbol paramName, const Colour& color, const MLSymbol signalName = MLSymbol());
	MLMultiSlider* addMultiSlider(const char * displayName, const MLRect & r, const MLSymbol paramName, int n, 
		const Colour& color = defaultColor);
	MLMultiButton* addMultiButton(const char * displayName, const MLRect & r, const MLSymbol paramName, int n, 
		const Colour& color = defaultColor);
	MLButton* addToggleButton(const char * displayName, const MLRect & r, const char * name, 
		const Colour& color = defaultColor, const float sizeMultiplier = 1.0f);

	MLDial* addPatcherOutputDial(const MLSymbol paramName, const MLRect & r, bool bipolar, 
		MLPatcher* pPatcher, int outIndex);

	MLDial* addMultDial(const MLRect & r, const MLSymbol paramName, const Colour& color);
	MLStepDisplay* addStepDisplay(const MLRect & r, const MLSymbol signalName, int steps, const Colour& color);
	MLScope* addScope(const MLRect & r, const MLSymbol signalName, const Colour& color);

	MLEnvelope* addEnvelope(const MLRect & r, const MLSymbol paramName);

	MLOutlet* addOutlet(const char * displayName, const MLRect & r, const char * signalName, MLPatcher* pPatcher, int idx,
		const Colour& color = defaultColor, const float sizeMultiplier = 1.0f);

protected:		
	MLPluginProcessor* mpProcessor;

private:
    // (prevent copy constructor and operator= being generated..)
    MLPluginView (const MLPluginView&);
    const MLPluginView& operator= (const MLPluginView&);

	MLPluginController* mpController;
	
	int mWrapperFormat;
	
    OpenGLContext mOpenGLContext;
	
};

// --------------------------------------------------------------------------------
// component setter functions
//
void setSliderValue(Component* comp, float val, int selector);
void setMultiSliderValue(Component* comp, float val, int selector);
void setMultiButtonValue(Component* comp, float val, int selector);
void setEnvelopeValue(Component* comp, float val, int selector);
void setButtonToggleState(Component* comp, float val, int selector);




#endif // __ML_PLUGINVIEW_H__