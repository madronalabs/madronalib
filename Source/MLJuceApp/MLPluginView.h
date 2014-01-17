
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

	void addSignalView(MLSymbol p, MLWidget* w, MLSymbol attr, int size, int priority = 0);

	// component setup 
	//
	MLPluginView* addSubView(const MLRect & r, const MLSymbol name);

	MLDial* addDial(const char * displayName, const MLRect & r, const MLSymbol paramName, 
		const Colour& color = defaultColor);	
	MLMultiSlider* addMultiSlider(const char * displayName, const MLRect & r, const MLSymbol paramName, int n, 
		const Colour& color = defaultColor);
	MLMultiButton* addMultiButton(const char * displayName, const MLRect & r, const MLSymbol paramName, int n, 
		const Colour& color = defaultColor);
	MLButton* addToggleButton(const char * displayName, const MLRect & r, const char * name,
                              const Colour& color = defaultColor, const float sizeMultiplier = 1.0f);
	MLButton* addTriToggleButton(const char * displayName, const MLRect & r, const char * name,
                              const Colour& color = defaultColor, const float sizeMultiplier = 1.0f);

    MLDrawableButton* addDrawableButton(const MLRect & r, const char * name, const Drawable* img, const Colour& color = defaultColor);
	MLDial* addMultDial(const MLRect & r, const MLSymbol paramName, const Colour& color);

	MLEnvelope* addEnvelope(const MLRect & r, const MLSymbol paramName);

protected:		
	MLPluginProcessor* mpProcessor;
	MLPluginController* mpController;

private:
    // (prevent copy constructor and operator= being generated..)
    MLPluginView (const MLPluginView&);
    const MLPluginView& operator= (const MLPluginView&);

	
	int mWrapperFormat;
	
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