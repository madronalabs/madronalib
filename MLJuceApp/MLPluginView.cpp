
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLPluginView.h"

MLPluginView::MLPluginView (MLPluginProcessor* const ownerProcessor, MLPluginController* pR) :
	MLAppView(pR, pR),
	mpProcessor(ownerProcessor),
	mpController(pR)
{
	MLWidget::setComponent(this);
	setOpaque (false);
}


MLPluginView::~MLPluginView()
{	
	deleteAllChildren();
}

// --------------------------------------------------------------------------------
#pragma mark component setup 
//

void MLPluginView::addSignalView(MLSymbol p, MLWidget* w, MLSymbol attr, int size)
{
	if(p && w && attr)
		mpController->addSignalViewToMap(p, w, attr, size);
}

MLPluginView* MLPluginView::addSubView(const MLRect & r, const MLSymbol name)
{
	MLPluginView* b = new MLPluginView(getProcessor(), mpController);
	addWidgetToView(b, r, name);
	return b;
}

MLDial* MLPluginView::addDial(const char * displayName, const MLRect & r, 
	const MLSymbol paramName, const Colour& color)
{
	MLDial* dial = MLAppView::addDial(displayName, r, paramName, color);
	
	// setup dial attrs from filter parameter
	MLPluginProcessor* const filter = getProcessor();
	int idx = filter->getParameterIndex(paramName);
	if (idx >= 0)
	{
		MLPublishedParamPtr p = filter->getParameterPtr(idx);
		if (p)
		{
			dial->setRange(p->getRangeLo(), p->getRangeHi(), p->getInterval(), p->getZeroThresh(), p->getWarpMode()); 
			dial->setDoubleClickReturnValue(true, p->getDefault());
		}
	}
	else
	{
		debug() << "MLPluginView::addDial: parameter " << paramName << " not found!\n";
	}
	
	return dial;
}

MLScopeDial* MLPluginView::addScopeDial(const char * displayName, const MLRect & r, 
	const MLSymbol paramName, const Colour& color, const MLSymbol signalName)
{
//	MLDial* dial = MLAppView::addDial(displayName, r, paramName, color);
	MLScopeDial* dial = new MLScopeDial;
	dial->setParamName(paramName);
	dial->setListener(getResponder());	
	dial->setDialStyle (MLScopeDial::Rotary);
	dial->setFillColor(color); 		
	addWidgetToView(dial, r, paramName);
	addParamView(paramName, dial, MLSymbol("value"));
	
	if (strcmp(displayName, ""))
	{
		addLabelAbove(dial, displayName);
	}
		
	// setup dial attrs from filter parameter
	MLPluginProcessor* const filter = getProcessor();
	int idx = filter->getParameterIndex(paramName);
	if (idx >= 0)
	{
		MLPublishedParamPtr p = filter->getParameterPtr(idx);
		if (p)
		{
			dial->setRange(p->getRangeLo(), p->getRangeHi(), p->getInterval(), p->getZeroThresh(), p->getWarpMode()); 
			dial->setDoubleClickReturnValue(true, p->getDefault());
		}
	}
	else
	{
		debug() << "MLPluginView::addDial: parameter " << paramName << " not found!\n";
	}
	
	if(signalName)
	{
		int bufferSize = 128;
		addSignalView(signalName, dial, MLSymbol("amplitude"), bufferSize);
	}
	
	return dial;
}

MLMultiSlider* MLPluginView::addMultiSlider(const char * displayName, const MLRect & r, const MLSymbol paramName, int numSliders, const Colour& color)
{
	MLMultiSlider* dial = MLAppView::addMultiSlider(displayName, r, paramName, numSliders, color);
	
	// setup dial attrs from filter parameter
	MLPluginProcessor* const filter = getProcessor();
	if(filter) 
	{
		int paramIdx = filter->getParameterIndex(paramName.withFinalNumber(0));
		if (paramIdx >= 0)
		{
			MLPublishedParamPtr p = filter->getParameterPtr(paramIdx);
			if (p)
			{
				dial->setRange(p->getRangeLo(), p->getRangeHi(), p->getInterval()); 
			}
		}
		else
		{
			debug() << "MLPluginView::addMultiSlider: parameter " << paramName << " not found!\n";
		}
	}
	return dial;
}

MLMultiButton* MLPluginView::addMultiButton(const char * displayName, const MLRect & r, const MLSymbol paramName, int numButtons, const Colour& color)
{
	MLMultiButton* b = MLAppView::addMultiButton(displayName, r, paramName, numButtons, color);
	
	// setup dial attrs from filter parameter
	MLPluginProcessor* const filter = getProcessor();
	if(filter) 
	{
		int paramIdx = filter->getParameterIndex(paramName.withFinalNumber(0));
		if (paramIdx >= 0)
		{
			MLPublishedParamPtr p = filter->getParameterPtr(paramIdx);
			if (p)
			{
			//	b->setRange(p->getRangeLo(), p->getRangeHi(), p->getInterval()); 
			}
		}
		else
		{
			debug() << "MLPluginView::addMultiButton: parameter " << paramName << " not found!\n";
		}		
	}
	return b;
}

MLButton* MLPluginView::addToggleButton(const char * displayName, const MLRect & r, const char * paramName, 
		const Colour& color, const float sizeMultiplier)
{
	MLButton* b = MLAppView::addToggleButton(displayName, r, paramName, color, sizeMultiplier);

	// setup button attrs from filter parameter
	MLPluginProcessor* const filter = getProcessor();
	int idx = filter->getParameterIndex(paramName);
	if (idx >= 0)
	{
		MLPublishedParamPtr p = filter->getParameterPtr(idx);
		if (p)
		{
			b->setRange(p->getRangeLo(), p->getRangeHi()); 
		}
	}
	else
	{
		debug() << "MLPluginView::addToggleButton: parameter " << paramName << " not found!\n";
	}
	
	return b;
}

MLDial* MLPluginView::addPatcherOutputDial(const MLSymbol paramName, const MLRect & r, bool bipolar, 
	MLPatcher* pPatcher, int outIndex)
{
	MLDial* dial = MLAppView::addDial("", r, paramName);

	if (bipolar)
	{
		dial->setRange(-1., 1., 0.01);
		dial->setBipolar(true);
	}
	else
	{
		dial->setRange(0., 1., 0.01);
		dial->setBipolar(false);
	}
	
	dial->setDialStyle (MLDial::Rotary);
	dial->setFillColor(findColour(MLLookAndFeel::defaultDialFillColor));
	dial->setRotaryParameters ((kMLPi * 1.f),(kMLPi * 3.0f), true);
	dial->setDoNumber(false);
	dial->setTicks(0);
	dial->setDoubleClickReturnValue (true, 0.0);
	dial->setOpaque(false); // allow patcher border to show through
	
	// setup dial attrs from filter parameter
	MLPluginProcessor* const filter = getProcessor();
	int idx = filter->getParameterIndex(paramName);
	if (idx >= 0)
	{
		MLPublishedParamPtr p = filter->getParameterPtr(idx);
		if (p)
		{
			dial->setRange(p->getRangeLo(), p->getRangeHi(), p->getInterval(), p->getZeroThresh(), p->getWarpMode()); 
			dial->setDoubleClickReturnValue(true, p->getDefault());
		}
	}
	else
	{
		debug() << "MLPluginView::addPatcherOutputDial: parameter " << paramName << " not found!\n";
	}
	
	// add to patcher as output target 
	if (pPatcher)
	{
		pPatcher->addOutput(dial, outIndex);
	}
		
	return dial;
}

MLDial* MLPluginView::addMultDial(const MLRect & r, const MLSymbol paramName, const Colour& color)
{
	MLDial* dial = MLAppView::addDial("", r, paramName, color);
	dial->setRange(0., 1., 0.01);
	dial->setBipolar(false);	
	dial->setDialStyle (MLDial::Rotary);
	dial->setRotaryParameters ((kMLPi * 1.f),(kMLPi * 3.0f), true);
	dial->setDoNumber(false);
	dial->setTicks(0);
	dial->setDoubleClickReturnValue (true, 0.0);
	dial->setOpaque(false);
	
	return dial;
}

MLStepDisplay* MLPluginView::addStepDisplay(const MLRect & r, const MLSymbol signalName, int steps, const Colour& color)
{
	MLStepDisplay * pDisp = new MLStepDisplay();
	pDisp->setNumSteps(steps);
	pDisp->setFillColor (color);

	addWidgetToView(pDisp, r, signalName); 
	
	int bufferSize = kMLEngineMaxVoices;
	addSignalView(signalName, pDisp, MLSymbol("step"), bufferSize);
	
	return(pDisp);
}

MLScope* MLPluginView::addScope(const MLRect & r, const MLSymbol signalName, const Colour& color)
{
	MLScope * pScope = new MLScope();
	pScope->setColour(MLScope::fgColorId, color);

	addWidgetToView(pScope, r, signalName); 
	
	int bufferSize = 256;
	addSignalView(signalName, pScope, MLSymbol("amplitude"), bufferSize);
	
	return(pScope);
}

MLEnvelope* MLPluginView::addEnvelope(const MLRect & r, const MLSymbol paramName)
{
	MLEnvelope * pE = new MLEnvelope();
		
	const std::string paramStr = paramName.getString();
	addParamView(MLSymbol(paramStr + "_delay"), pE, MLSymbol("delay"));
	addParamView(MLSymbol(paramStr + "_attack"), pE, MLSymbol("attack"));
	addParamView(MLSymbol(paramStr + "_decay"), pE, MLSymbol("decay"));
	addParamView(MLSymbol(paramStr + "_sustain"), pE, MLSymbol("sustain"));
	addParamView(MLSymbol(paramStr + "_release"), pE, MLSymbol("release"));
	addParamView(MLSymbol(paramStr + "_repeat"), pE, MLSymbol("repeat"));
	
	addWidgetToView(pE, r, paramName);
	return(pE);
}


MLOutlet* MLPluginView::addOutlet(const char * displayName, const MLRect & r, const char * signalName, 
	MLPatcher* pPatcher, int idx, const Colour& color, const float sizeMultiplier)
{
	MLOutlet * pOutlet = new MLOutlet;
	pOutlet->setFillColour (color);	

	MLSymbol widgetName = signalName;
	addWidgetToView(pOutlet, r, widgetName);
	
	int bufferSize = kMLEngineMaxVoices;
	addSignalView(signalName, pOutlet, MLSymbol("amplitude"), bufferSize);

	// add patcher as event listener to outlet
	if (pPatcher)
	{
		pOutlet->setListener(pPatcher);
		pPatcher->addInput(pOutlet, idx);
	}

	// make label
	if (strcmp(displayName, ""))
	{
		addLabelAbove(pOutlet, displayName, sizeMultiplier);
	}
	
	return(pOutlet);
}

