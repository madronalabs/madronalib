
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
    // debug() << "DELETING " << getWidgetName() << "\n";
	setAnimationsActive(false);
	deleteAllChildren();
}

void MLAppView::addParamView(MLSymbol p, MLWidget* w, MLSymbol attr)
{
	if(p && w && attr)
		getReporter()->addPropertyViewToMap(p, w, attr);
}

void MLAppView::addWidgetToView(MLWidget* pW, const MLRect& r, MLSymbol name = MLSymbol())
{
	addWidget(pW, name);
	pW->setGridBounds(r);
	//pW->setWidgetContainer(this);
	pW->addListener(getResponder());
	addAndMakeVisible(pW->getComponent());
}

// --------------------------------------------------------------------------------
#pragma mark component add utility methods
//

MLDial* MLAppView::addDial(const char * displayName, const MLRect & r, const MLSymbol p,
	const Colour& color, const float sizeMultiplier)
{
	MLDial* dial = new MLDial;
	dial->setTargetPropertyName(p);
//	dial->setListener(getResponder()); // MLTEST
	
	dial->setSizeMultiplier(sizeMultiplier);
	dial->setDialStyle (MLDial::Rotary);
	dial->setFillColor(color); 		

	addWidgetToView(dial, r, p);
	
	addParamView(p, dial, MLSymbol("value"));
	
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
	slider->setTargetPropertyName(paramName);
// MLTEST	slider->setListener(getResponder());
	
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
	b->setTargetPropertyName(paramName);
// MLTEST 	b->setListener(getResponder());
	
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
	button->setTargetPropertyName(paramName);
// MLTEST 	button->setListener(getResponder());
	button->setFillColor(color);
	addWidgetToView(button, r, paramName);
	addParamView(paramName, button, MLSymbol("value"));
	
	if (strcmp(displayName, ""))
	{
		addLabelAbove(button, displayName, sizeMultiplier);
	}
    
	return button;
}

MLButton* MLAppView::addTriToggleButton(const char* displayName, const MLRect & r, const MLSymbol paramName,
                                     const Colour& color, const float sizeMultiplier)
{
	MLButton* button = new MLTriToggleButton;
	button->setSizeMultiplier(sizeMultiplier);
	button->setTargetPropertyName(paramName);
// MLTEST 	button->setListener(getResponder());
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

MLDrawableButton* MLAppView::addRawImageButton(const MLRect & r, const char * name, 
	const Colour& color, const Drawable* normalImg)
{
	MLDrawableButton* b = new MLDrawableButton;
	b->setTargetPropertyName(name);
// MLTEST 	b->setListener(getResponder());
	b->setProperty("toggle", false);
	b->setImage(normalImg);
	addWidgetToView(b, r, name);
	return b;
}

MLTextButton* MLAppView::addTextButton(const char * displayName, const MLRect & r, const char * name, const Colour& color)
{	
	MLTextButton* b = new MLTextButton;
	b->setTargetPropertyName(name);
// MLTEST 	button->setListener(getResponder());
	b->setProperty("toggle", false);
	b->setFillColor(color);
	b->setProperty("text", displayName);
	addWidgetToView(b, r, name);
	return b;
}

MLMenuButton* MLAppView::addMenuButton(const char * displayName, const MLRect & r, const char * menuName, const Colour& color)
{	
	MLMenuButton* b = new MLMenuButton();
	b->setTargetPropertyName(menuName);
// MLTEST 	button->setListener(getResponder());		
	b->setFillColor(color);
	b->setProperty("text", "---");
	addWidgetToView(b, r, menuName);
	addParamView(menuName, b, MLSymbol("text"));
	
	if (strcmp(displayName, ""))
	{
		addLabelAbove(b, displayName);
	}
	return b;
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
	
		MLRect rr = r.translated(Vec2(0, -labelHeight*c->getLabelVerticalOffset()) + offset);
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

MLWaveform* MLAppView::addWaveform(const MLRect & r, const MLSymbol paramName)
{
	MLWaveform* w = new MLWaveform;
	addWidgetToView(w, r, paramName);
	addParamView(paramName, w, MLSymbol("wave"));
	return w;
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
		MLRect scaled = (r*u).getIntPart();
		if (!w->wantsResizeLast())
		{
			w->setWidgetGridUnitSize(u);
			w->resizeWidget(scaled, u);
		}
	}
	
	for(it = mWidgets.begin(); it != mWidgets.end(); ++it)
	{
		MLWidget* w = (*it).second;
		MLRect r = w->getGridBounds();
		MLRect scaled = (r*u).getIntPart();
		if (w->wantsResizeLast())
		{
			w->setWidgetGridUnitSize(u);
			w->resizeWidget(scaled, u);
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
	
	const int kMenuBarHeight = 20;
	r.setTop(r.getY() + kMenuBarHeight);
	
	Rectangle<int> b(x, y, w, h);
	Rectangle<int> c = r.getIntersection(b);	
	if((c.getWidth() >= minDim) && (c.getHeight() >= minDim))
	{
        // debug() << "	MLAppView::setPeerBounds: " << x << " " << y << " " << w << " " << h << "\n";
		p->setBounds(Rectangle<int>(x, y, w, h), false);
	}
	else
	{
		// make new onscreen bounds rect
		//const Desktop::Displays::Display& main = d.getDisplays().getMainDisplay();
		//Rectangle<int>mainRect = main.userArea;		
		Rectangle<int> onscreenBounds = b.constrainedWithin(r);
		p->setBounds(onscreenBounds, false);
	}
}

