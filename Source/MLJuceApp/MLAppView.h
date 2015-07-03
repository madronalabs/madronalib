
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_APP_VIEW_H__
#define __ML_APP_VIEW_H__

#include "JuceHeader.h"
#include "MLUIBinaryData.h"
#include "MLReporter.h"
#include "MLWidgetContainer.h"

#include "MLButton.h"
#include "MLPanel.h"
#include "MLDrawing.h"
#include "MLDrawableButton.h"
#include "MLTextButton.h"
#include "MLMenuButton.h"
#include "MLToggleButton.h"
#include "MLTriToggleButton.h"
#include "MLMultiButton.h"
#include "MLMultiSlider.h"
#include "MLLookAndFeel.h"
#include "MLEnvelope.h"
#include "MLProgressBar.h"
#include "MLDebugDisplay.h"
#include "MLDefaultFileLocations.h"
#include "MLJuceFilesMac.h"
#include "MLVector.h"
#include "MLSymbol.h"

extern const Colour defaultColor;

// maintains a View Component with grid dimensions set by parent. 
// provides handy UI component creation functions for grid.
// 
class MLAppView : 
	public Component,
	public MLWidgetContainer
{
public:
	MLAppView(MLWidget::Listener* pResp, MLReporter* pRep);
    ~MLAppView();

	void initialize();
	
	// MLWidget::MLPropertyListener
	void doPropertyChangeAction(MLSymbol p, const MLProperty & newVal);

	virtual bool isWidgetContainer(void) { return true; }

	// using our Reporter, setup view for Model Property p as attr of widget.
	void addPropertyView(MLSymbol p, MLWidget* w, MLSymbol attr);

	// add the widget and add our Responder as a listener. The Responder can then do things in HandleWidgetAction().
	void addWidgetToView(MLWidget* pW, const MLRect& r, MLSymbol name);
	
	void addSignalView(MLSymbol p, MLWidget* w, MLSymbol attr, int size = kMLSignalViewBufferSize);	

	virtual MLDial* addDial(const char * displayName, const MLRect & r, const MLSymbol propName, 
		const Colour& color = defaultColor, const float sizeMultiplier = 1.0f);	
	virtual MLMultiSlider* addMultiSlider(const char * displayName, const MLRect & r, const MLSymbol propName, 
		int n, const Colour& color);
	virtual MLMultiButton* addMultiButton(const char * displayName, const MLRect & r, const MLSymbol propName, 
		int n, const Colour& color);
	virtual MLButton* addToggleButton(const char* displayName, const MLRect & r, const MLSymbol name,
                                      const Colour& color = defaultColor, const float sizeMultiplier = 1.0f);
	virtual MLButton* addTriToggleButton(const char* displayName, const MLRect & r, const MLSymbol name,
                                      const Colour& color = defaultColor, const float sizeMultiplier = 1.0f);

	MLPanel* addPanel(const MLRect & r, const Colour& color = defaultColor);
	MLDebugDisplay* addDebugDisplay(const MLRect & r);
		
	MLDrawableButton* addRawImageButton(const MLRect & r, const char * name, 
		const Colour& color, const Drawable* normal);
		
	MLTextButton* addTextButton(const char * displayName, const MLRect & r, const char * name,
		const Colour& color = defaultColor);
		
	MLMenuButton* addMenuButton(const char * displayName, const MLRect & r, const char * name,
		const Colour& color = defaultColor);
		
	MLLabel* addLabel(const char* displayName, const MLRect & r, 
		const float sizeMultiplier = 1.0f, const int font = eMLCaption);
		
	MLLabel* addLabelAbove(MLWidget* c, const char* displayName, 
		const float sizeMultiplier = 1.0f, const int font = eMLCaption, Vec2 offset = Vec2());

	MLDrawing* addDrawing(const MLRect & r);
	MLProgressBar* addProgressBar(const MLRect & r);

	void resized();
	void setViewBoundsProperty();
	void setWindowBounds(const MLSignal& bounds);

	// sent by the AppWindow 
	void windowMoved();
	void windowResized();
	
protected:
	bool mInitialized;
	float mGridUnitSize;	
	MLWidget::Listener* mpResponder;
	MLReporter* mpReporter;
};

#endif // __ML_APP_VIEW_H__