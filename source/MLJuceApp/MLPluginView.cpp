
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


#pragma mark component setup 

// add a view to our controller’s map that the controller will service periodically.
// p: name of signal to view
// w: widget to do the viewing
// attr: attr of widget to send the signal to 
// size: size of signals to collect
// priority: at present either 1 for show always, or 0 for normal priority
//
void MLPluginView::addSignalView(MLSymbol p, MLWidget* w, MLSymbol attr, int size, int priority)
{
	if(p && w && attr)
		mpController->addSignalViewToMap(p, w, attr, size, priority);
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
	
	// setup dial properties based on the filter parameter
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

MLMultiSlider* MLPluginView::addMultiSlider(const char * displayName, const MLRect & r, const MLSymbol paramName, int numSliders, const Colour& color)
{
	MLMultiSlider* dial = MLAppView::addMultiSlider(displayName, r, paramName, numSliders, color);
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
	MLPluginProcessor* const filter = getProcessor();
	int idx = filter->getParameterIndex(paramName);
	if (idx >= 0)
	{
		MLPublishedParamPtr p = filter->getParameterPtr(idx);
		if (p)
		{
			b->setToggleValues(p->getRangeLo(), p->getRangeHi());
		}
	}
	else
	{
		debug() << "MLPluginView::addToggleButton: parameter " << paramName << " not found!\n";
	}
	
	return b;
}

MLButton* MLPluginView::addTriToggleButton(const char * displayName, const MLRect & r, const char * paramName,
                                        const Colour& color, const float sizeMultiplier)
{
	MLButton* b = MLAppView::addTriToggleButton(displayName, r, paramName, color, sizeMultiplier);
	return b;
}

MLDial* MLPluginView::addMultDial(const MLRect & r, const MLSymbol paramName, const Colour& color)
{
	MLDial* dial = addDial("", r, paramName, color);
	dial->setRange(0., 1., 0.01);
	dial->setBipolar(false);	
	dial->setDialStyle (MLDial::Rotary);
	dial->setRotaryParameters ((kMLPi * 1.f),(kMLPi * 3.0f), true);
	dial->setDoNumber(false);
	dial->setTicks(0);
	// dial->setDoubleClickReturnValue (true, 0.0);
	//dial->setOpaque(false);
	return dial;
}

MLEnvelope* MLPluginView::addEnvelope(const MLRect & r, const MLSymbol paramName)
{
	MLEnvelope * pE = new MLEnvelope();
    
	const std::string paramStr = paramName.getString();
	addPropertyView(MLSymbol(paramStr + "_delay"), pE, MLSymbol("delay"));
	addPropertyView(MLSymbol(paramStr + "_attack"), pE, MLSymbol("attack"));
	addPropertyView(MLSymbol(paramStr + "_decay"), pE, MLSymbol("decay"));
	addPropertyView(MLSymbol(paramStr + "_sustain"), pE, MLSymbol("sustain"));
	addPropertyView(MLSymbol(paramStr + "_release"), pE, MLSymbol("release"));
	addPropertyView(MLSymbol(paramStr + "_repeat"), pE, MLSymbol("repeat"));
	
	addWidgetToView(pE, r, paramName);
	return(pE);
}


