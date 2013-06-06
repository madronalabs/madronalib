
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
	mpController->setView(0);
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

