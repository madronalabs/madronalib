
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/


#include "MLDemoInstrumentEditor.h"


// ----------------------------------------------------------------
// entry point for MLPluginEditor to make one of this subclass.
// not the most beautiful code, but this is a stopoff on the way to
// reading in a UI description .
//
// this code essentially does what the application setup would do 
// in a non-plugin app, except that the Model already exists in 
// the MLPluginProcessor.  So we attach to it instead of creating a Model.
//
MLPluginEditor* CreateMLPluginEditor (MLPluginProcessor* const pProcessor, const MLRect& bounds, bool num, bool anim)
{
	MLPluginEditor* r = 0;	
	MLDemoInstrumentEditor* pEd = new MLDemoInstrumentEditor(pProcessor);
	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
	
	if (pEd)
	{
		pEd->initialize(pProcessor);
		r = pEd;
	}

	if(MLRect() != bounds)
	{
		// set up saved size.  Position should be set by host. 
		pEd->setSize(bounds.getWidth(), bounds.getHeight());
	}
	else 
	{
		// set up default size
		int minGrid = 48;
		pEd->setSize(kExampleViewUnitsX * minGrid, kExampleViewUnitsY * minGrid);
	}
	
	pProcessor->setModelParam("patch_num", num);
	myLookAndFeel->setDrawNumbers(num);
	pProcessor->setModelParam("patch_anim", anim);
	myLookAndFeel->setAnimate(anim);

	return r;
}

// ----------------------------------------------------------------
#pragma mark MLDemoInstrumentEditor

MLDemoInstrumentEditor::MLDemoInstrumentEditor (MLPluginProcessor* const ownerProcessor) :
	MLPluginEditor(ownerProcessor),
	mpProcessor(ownerProcessor),
	mpView(0),
	mpController(0)
{	
  
}

MLDemoInstrumentEditor::~MLDemoInstrumentEditor()
{
	// turn off any param change notifications from Processor
	mpProcessor->removeParamListener(mpController); 
	
	if(mpController) delete mpController;
	deleteAllChildren();
}

void MLDemoInstrumentEditor::initialize (MLPluginProcessor* )
{

#if DEBUG
	debug().setActive(true);
#else
	debug().setActive(false);
#endif

	mpController = new MLDemoInstrumentController(mpProcessor);
	mpView = new MLDemoInstrumentView(mpProcessor, mpController);
	mpBorder = new MLAppBorder();
	mpBorder->makeResizer(this);
	mpBorder->setContent(mpView);
	mpBorder->setGridUnits(kExampleViewUnitsX, kExampleViewUnitsY);
		
	mpController->setView(mpView);
	mpController->initialize();
	mpController->updateAllParams();
	
    addAndMakeVisible(mpBorder);
    
    debug() << "CREATING MLDemoInstrumentEditor\n";
}

void MLDemoInstrumentEditor::resized()
{
    debug() << "MLDemoInstrumentEditor:: RESIZED to " << getWidth() << ", " << getHeight() << "\n";
	
    mpBorder->setSize(getWidth(), getHeight());
}
