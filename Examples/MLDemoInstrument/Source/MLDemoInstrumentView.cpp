
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLDemoInstrumentView.h"

// --------------------------------------------------------------------------------
#pragma mark ExampleHeaderView

ExampleHeaderView::ExampleHeaderView(MLPluginProcessor* const , MLResponder* pResp, MLReporter* pRep) :
	MLAppView(pResp, pRep)
{
	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
	setInterceptsMouseClicks (false, true);
	const Colour c = findColour(MLLookAndFeel::backgroundColor);
	const Colour buttonColor = findColour(TextButton::buttonColourId);

	// masthead
	MLLabel* pLC = addLabel("", MLRect(0.25, 0.125, 3.0, 0.75));	
	pLC->setDrawable(myLookAndFeel->getPicture("masthead"));	
	pLC->setResizeToText(false);
	
	// preset menu
	MLMenuButton* mB = addMenuButton("", MLRect(4, 0.2, 4, 0.6), "preset", buttonColor);
	mB->setConnectedEdges(Button::ConnectedOnRight | Button::ConnectedOnLeft);
	mB->setMenuTextStyle(false); 
	mB->setButtonText("---"); 
	addParamView("preset_name", mB, MLSymbol("value"));
	
	// prev / next buttons
	mpPrevButton = addRawImageButton(MLRect(3.5, 0.25, 0.5, 0.5), "prev", buttonColor, myLookAndFeel->getPicture("arrowleft"));
	mpNextButton = addRawImageButton(MLRect(8, 0.25, 0.5, 0.5), "next", buttonColor, myLookAndFeel->getPicture("arrowright"));
}


ExampleHeaderView::~ExampleHeaderView()
{
}

void ExampleHeaderView::paint(Graphics& )
{
//	g.fillAll(Colours::blue); // TEST
}

// --------------------------------------------------------------------------------
#pragma mark MLDemoInstrumentView

MLDemoInstrumentView::MLDemoInstrumentView(MLPluginProcessor* const pProcessor, MLPluginController* pC) :
	MLPluginView(pProcessor, pC),
	mpHeader(0),
	mpPrevButton(0),
	mpNextButton(0),
	mpPages(0)
{			
	setWidgetName("example_view");
	
	// setup application's look and feel 
	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
	LookAndFeel::setDefaultLookAndFeel (myLookAndFeel);	
	setupColors();
	const Colour c1 = findColour(MLLookAndFeel::backgroundColor);
		
	// local component colors
	static const Colour dial1 = Colour::fromHSV(0.40f, 0.50f, 0.80f, 1.f);
	static const Colour dial2 = Colour::fromHSV(0.58f, 0.50f, 0.99f, 1.f);
	
	// component sizes
	MLRect dialRect(0, 0, 1., 1.);
	MLRect dialRectBig(0, 0, 1.2, 1.2);
	MLRect dialRectBigger(0, 0, 1.7, 1.7);

	// add resources for drawing
	myLookAndFeel->addPicture ("masthead", MLDemoInstrumentBinaryData::masthead_svg, MLDemoInstrumentBinaryData::masthead_svgSize);
	myLookAndFeel->addPicture ("arrowleft", MLDemoInstrumentBinaryData::arrowleft_svg, MLDemoInstrumentBinaryData::arrowleft_svgSize);
	myLookAndFeel->addPicture ("arrowright", MLDemoInstrumentBinaryData::arrowright_svg, MLDemoInstrumentBinaryData::arrowright_svgSize);

	// setup grid size and add subviews.
	// int u = myLookAndFeel->getGridUnitSize(); 
	myLookAndFeel->setGlobalTextScale(1.15f);
	const float titleSize = 1.20f;

	mpHeader = new ExampleHeaderView(pProcessor, getResponder(), getReporter());
	addWidgetToView(mpHeader, MLRect(0, 0, kExampleViewUnitsX, 1), "header");
	
	addLabel("OSCILLATOR", MLRect(0, 2, 6, 0.5), titleSize, eMLTitle)
		->setColour(MLLabel::textColourId, findColour(MLLookAndFeel::darkLabelColor));

	addDial("noise", dialRectBig.withCenter(2, 4), "osc_noise", dial1);
	addDial("pitch", dialRectBigger.withCenter(4, 4), "osc_pitch", dial1);
	
	addLabel("OUTPUT", MLRect(6, 2, 6, 0.5), titleSize, eMLTitle)
		->setColour(MLLabel::textColourId, findColour(MLLookAndFeel::darkLabelColor));

	// pan slider
	addDial("pan", MLRect(7, 3.0, 4, 1), "output_pan", dial2)
		->setDialStyle (MLDial::LinearHorizontal);
	
	// pan slider
	addDial("reverb", dialRectBig.withCenter(9.0, 5.0), "output_reverb", dial2);
	
	// debug
	dumpWidgets();
}

MLDemoInstrumentView::~MLDemoInstrumentView()
{
	debug() << "DELETING Example View\n";
	// save view selection state on window close
	
}

void MLDemoInstrumentView::setAttribute(MLSymbol attr, float val)
{
	MLWidget::setAttribute(attr, val);
	// debug() << "MLDemoInstrumentView " << getWidgetName() << ": setAttribute " << attr << " = " << val << "\n";
	// no attributes of this view to set. 
}

void MLDemoInstrumentView::setupColors()
{
	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
	myLookAndFeel->setGradientMode(1);
	
	myLookAndFeel->setColour(MLLookAndFeel::backgroundColor2, Colour::fromHSV(0.41f, 0.30f, 0.60f, 1.f));	// top and bottom
	myLookAndFeel->setColour(MLLookAndFeel::backgroundColor, Colour::fromHSV(0.41f, 0.20f, 0.65f, 1.f));	// middle	
	myLookAndFeel->setColour(MLLookAndFeel::backgroundColor, Colour::fromHSV(0.41f, 0.20f, 0.75f, 1.f));

	// dial tracks and button backgrounds
	myLookAndFeel->setColour(MLLookAndFeel::darkFillColor, Colour::fromHSV(0.41f, 0.41f, 0.50f, 1.f)); // empty things	
	myLookAndFeel->setColour(MLLookAndFeel::defaultFillColor, Colour::fromHSV(0.41f, 0.34f, 0.75f, 1.f));	// little dials fill
			
	myLookAndFeel->setColour(MLLookAndFeel::markColor, Colour::fromHSV(0.41f, 0.95f, 0.20f, 1.f)); // section headers and lines
	myLookAndFeel->setColour(MLLookAndFeel::outlineColor, Colour::fromHSV(0.41f, 0.45f, 0.20f, 1.f)); // outlines of controls

	myLookAndFeel->setColour(MLLookAndFeel::labelColor, Colour::fromHSV(0.41f, 0.45f, 0.25f, 1.f)); // text labels	
	myLookAndFeel->setColour(MLLookAndFeel::darkLabelColor, Colour::fromHSV(0.41f, 0.99f, 0.10f, 1.f)); // text headers
		
	myLookAndFeel->setColour(MLLookAndFeel::shadowColor, Colour::fromHSV(0.41f, 0.10f, 0.00f, 1.f));	
	myLookAndFeel->setColour(MLLookAndFeel::highlightColor, Colour::fromHSV(0.41f, 0.10f, 0.70f, 1.f));
	
	myLookAndFeel->setColour(MLLookAndFeel::radioOffColor, Colour(0xffc0c0bc));
	myLookAndFeel->setColour(MLLookAndFeel::radioOnColor, Colour::fromHSV(0.41f, 0.40f, 0.90f, 1.f));
	myLookAndFeel->setColour(MLLookAndFeel::buttonOffColor, Colour(0xeeeeeeee));
	myLookAndFeel->setColour(MLLookAndFeel::buttonOnColor, Colour(0xcc666666));
}



