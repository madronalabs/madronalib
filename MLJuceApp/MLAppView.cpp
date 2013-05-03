
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLAppView.h"

const Colour defaultColor = Colours::grey;

MLAppView::MLAppView(MLResponder* pResp, MLReporter* pRep) :
	mDoAnimations(false),
	mpResponder(pResp),
	mpReporter(pRep)
{
	MLWidget::setComponent(this);
	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
	LookAndFeel::setDefaultLookAndFeel (myLookAndFeel);		

	setOpaque(false);
//	setOpaque(myLookAndFeel->getDefaultOpacity());
//	setBufferedToImage(myLookAndFeel->getDefaultBufferMode());
//	setPaintingIsUnclipped(myLookAndFeel->getDefaultUnclippedMode());
	
	setInterceptsMouseClicks (false, true);
}

MLAppView::~MLAppView()
{
	setAnimationsActive(false);
	deleteAllChildren();
}

void MLAppView::addParamView(MLSymbol p, MLWidget* w, MLSymbol attr)
{
	if(p && w && attr)
		getReporter()->addParamViewToMap(p, w, attr);
}

void MLAppView::addWidgetToView(MLWidget* pW, const MLRect& r, MLSymbol name = MLSymbol())
{
	addWidget(pW, name);
	pW->setGridBounds(r);
	//pW->setWidgetContainer(this);
	addAndMakeVisible(pW->getComponent());
}

// --------------------------------------------------------------------------------
#pragma mark component add utility methods
//

MLDial* MLAppView::addDial(const char * displayName, const MLRect & r, const MLSymbol paramName, 
	const Colour& color, const float sizeMultiplier)
{
	MLDial* dial = new MLDial;
	dial->setParamName(paramName);
	dial->setListener(getResponder());	
	
	dial->setSizeMultiplier(sizeMultiplier);
	dial->setDialStyle (MLDial::Rotary);
	dial->setFillColor(color); 		

	addWidgetToView(dial, r, paramName);
	
	addParamView(paramName, dial, MLSymbol("value"));
	
	if (strcmp(displayName, ""))
	{
		addLabelAbove(dial, displayName);
	}
	return dial;
}

MLMultiSlider* MLAppView::addMultiSlider(const char * displayName, const MLRect & r, const MLSymbol paramName, 
	int numSliders, const Colour& color)
{
	MLMultiSlider* slider = new MLMultiSlider;
	slider->setNumSliders(numSliders);
	slider->setParamName(paramName);
	slider->setListener(getResponder());	
	
	slider->setFillColor(color); 		

	addWidgetToView(slider, r, paramName);
	
	for(int i=0; i<numSliders; ++i)
	{
		addParamView(paramName.withFinalNumber(i), slider, MLSymbol("value").withFinalNumber(i));
	}
	
	if (strcmp(displayName, ""))
	{
		addLabelAbove(slider, displayName);
	}
	return slider;
}

MLMultiButton* MLAppView::addMultiButton(const char * displayName, const MLRect & r, const MLSymbol paramName, 
	int n, const Colour& color)
{
	MLMultiButton* b = new MLMultiButton;
	b->setNumButtons(n);
	b->setParamName(paramName);
	b->setListener(getResponder());	
	
	b->setFillColor(color); 		

	addWidgetToView(b, r, paramName);
	
	for(int i=0; i<n; ++i)
	{
		addParamView(paramName.withFinalNumber(i), b, MLSymbol("value").withFinalNumber(i));
	}
	
	if (strcmp(displayName, ""))
	{
		addLabelAbove(b, displayName);
	}
	return b;
}

MLButton* MLAppView::addToggleButton(const char* displayName, const MLRect & r, const MLSymbol paramName, 
	const Colour& color, const float sizeMultiplier)
{	
	MLButton* button = new MLToggleButton;
	button->setSizeMultiplier(sizeMultiplier);
	button->setParamName(paramName);

	button->setListener(getResponder());	

	button->setFillColor(color); 

	addWidgetToView(button, r, paramName);
	
	addParamView(paramName, button, MLSymbol("value"));
	
	if (strcmp(displayName, ""))
	{
		addLabelAbove(button, displayName, sizeMultiplier);
	}

	return button;
}

MLPanel* MLAppView::addPanel(const MLRect & r, const Colour& color)
{
	MLPanel* b = new MLPanel;
	
	b->setBackgroundColor(color);

	addWidgetToView(b, r);

	return b;
}

MLDebugDisplay* MLAppView::addDebugDisplay(const MLRect & r)
{
	MLDebugDisplay* b = new MLDebugDisplay();
	addWidgetToView(b, r);
	return b;
}

MLDrawableButton* MLAppView::addDrawableButton(const MLRect & r, const char * name, 
	const Colour& color, const Drawable* normalImg)
{
	MLDrawableButton* b = new MLDrawableButton;
	b->setParamName(name);
	
	b->setListener(getResponder());	
	b->setClickingTogglesState(false);
	
	b->setButtonStyle(MLDrawableButton::ImageOnButtonBackground);
	b->setBackgroundColours(color, color);
	b->setImage(normalImg);

	addWidgetToView(b, r, name);

	return b;
}

MLDrawableButton* MLAppView::addRawImageButton(const MLRect & r, const char * name, 
	const Colour& color, const Drawable* normalImg)
{
	MLDrawableButton* b = new MLDrawableButton;
	b->setParamName(name);
	
	b->setListener(getResponder());	
	b->setClickingTogglesState(false);
	
	b->setButtonStyle(MLDrawableButton::ImageFitted);
	b->setBackgroundColours(color, color);
	b->setImage(normalImg);
	
	addWidgetToView(b, r, name);

	return b;
}

MLTextButton* MLAppView::addTextButton(const char * displayName, const MLRect & r, const char * name, const Colour& color)
{	
	MLTextButton* button = new MLTextButton;
	button->setParamName(name);
	button->setListener(getResponder());	
	button->setClickingTogglesState(false);
	button->setFillColor(color); 
	button->setButtonText(displayName);	
	addWidgetToView(button, r, name);
	return button;
}

MLMenuButton* MLAppView::addMenuButton(const char * displayName, const MLRect & r, const char * menuName, const Colour& color)
{	
	MLMenuButton* button = new MLMenuButton();
	button->setParamName(menuName);
	button->setListener(getResponder());		
	button->setFillColor(color); 
	button->setButtonText("---"); 
	addWidgetToView(button, r, menuName);	
	addParamView(menuName, button, MLSymbol("text"));
	
	if (strcmp(displayName, ""))
	{
		addLabelAbove(button, displayName);
	}
	return button;
}

MLGraph* MLAppView::addGraph(const char * name, const Colour& color)
{
	MLGraph* graph = new MLGraph;
	graph->setName(name);
	graph->setColor(color); 
	addAndMakeVisible(graph);
	return graph;
}

MLLabel* MLAppView::addLabel(const char* displayName, const MLRect & r, const float sizeMultiplier, int font)
{
	MLLabel* label = new MLLabel(displayName);
	if (label)
	{
		MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
		
		if (strcmp(displayName, ""))
		{
			Font f = myLookAndFeel->getFont(font);
			label->setFont(f);		
			label->setSizeMultiplier(sizeMultiplier);
			label->setJustification(Justification::centred);
		}
		
		label->setResizeToText(true);
		addWidgetToView(label, r, "");
	}
//debug() << "added label " << displayName << ", rect" << r << "\n";
	return label;
}

MLLabel* MLAppView::addLabelAbove(MLWidget* c, const char* displayName, const float sizeMultiplier, int font, Vec2 offset)
{
	MLLabel* label = new MLLabel(displayName);
	if (label)
	{
		MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
		float labelHeight = myLookAndFeel->getLabelHeight()*sizeMultiplier;
		
		//float m = myLookAndFeel->getMargin();
		label->setResizeToText(true);
		
		MLRect r (c->getGridBounds());
		r.setHeight(labelHeight);
		r.stretchWidthTo(1.);
		
		Font f = myLookAndFeel->getFont(font);
		label->setFont(f);		
		label->setSizeMultiplier(sizeMultiplier);
		label->setJustification(Justification::centred);		
	
		MLRect rr = r.translated(Vec2(0, -labelHeight) + offset);
		addWidgetToView(label, rr);

	}
	return label;
}


MLDrawing* MLAppView::addDrawing(const MLRect & r)
{
	MLDrawing* drawing = new MLDrawing;
	addWidgetToView(drawing, r);
	return drawing;
}

MLProgressBar* MLAppView::addProgressBar(const MLRect & r)
{
	MLProgressBar* pb = new MLProgressBar;
	addWidgetToView(pb, r);
	return pb;
}

void MLAppView::setAnimationsActive(bool animState)
{
//debug() << "animations: " << animState << "\n";
	mDoAnimations = animState;
}

// --------------------------------------------------------------------------------
#pragma mark resize
//

void MLAppView::resized()
{
	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
	int u = myLookAndFeel->getGridUnitSize(); 

	std::map<MLSymbol, MLWidget*>::iterator it;

	for(it = mWidgets.begin(); it != mWidgets.end(); ++it)
	{
		MLWidget* w = (*it).second;
		MLRect r = w->getGridBounds();
		MLRect scaled = r*u;
		if (!w->wantsResizeLast())
		{
			w->setWidgetGridUnitSize(u);
			w->resizeWidget(r*u, u);
		}
	}
	
	for(it = mWidgets.begin(); it != mWidgets.end(); ++it)
	{
		MLWidget* w = (*it).second;
		MLRect r = w->getGridBounds();
		MLRect scaled = r*u;
		if (w->wantsResizeLast())
		{
			w->setWidgetGridUnitSize(u);
			w->resizeWidget(r*u, u);
		}
	}
}

void MLAppView::setPeerBounds(int x, int y, int w, int h)
{
	int minDim = 200;
	ComponentPeer* p = getPeer();
	if(!p) return;
	Desktop& d = Desktop::getInstance();
	Rectangle<int> r = d.getDisplays().getTotalBounds(true);
	
	// 
	const int kMenuBarHeight = 20;
	r.setTop(r.getY() + kMenuBarHeight);
	
			
	Rectangle<int> b(x, y, w, h);
	Rectangle<int> c = r.getIntersection(b);	
	if((c.getWidth() >= minDim) && (c.getHeight() >= minDim))
	{
debug() << "	MLAppView::setPeerBounds: " << x << " " << y << " " << w << " " << h << "\n";
		p->setBounds(x, y, w, h, false);
	}
	else
	{
		// make new onscreen bounds rect
		//const Desktop::Displays::Display& main = d.getDisplays().getMainDisplay();
		//Rectangle<int>mainRect = main.userArea;		
		Rectangle<int> onscreenBounds = b.constrainedWithin(r);
		p->setBounds(onscreenBounds.getX(), onscreenBounds.getY(), 
			onscreenBounds.getWidth(), onscreenBounds.getHeight(), false);
	}
}

