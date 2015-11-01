
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLAppView.h"

const Colour defaultColor = Colours::grey;

MLAppView::MLAppView(MLWidget::Listener* pResp, MLReporter* pRep) :
	mpResponder(pResp),
	mpReporter(pRep),
	mInitialized(false)
{
	MLWidget::setComponent(this);
	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
	LookAndFeel::setDefaultLookAndFeel (myLookAndFeel);		
	setOpaque(false);
	setInterceptsMouseClicks (false, true);
}

MLAppView::~MLAppView()
{
	deleteAllChildren();
}

void MLAppView::initialize()
{
	mInitialized = true;
}

void MLAppView::doPropertyChangeAction(MLSymbol p, const MLProperty & newVal)
{
	int propertyType = newVal.getType();
	switch(propertyType)
	{
		case MLProperty::kFloatProperty:
		{
		}
		break;
		case MLProperty::kStringProperty:
		{
		}
		break;
		case MLProperty::kSignalProperty:
		{
			const MLSignal& sig = newVal.getSignalValue();
			if(p == MLSymbol("view_bounds"))
			{
				setWindowBounds(sig);
			}
		}
		break;
		default:
		break;
	}
}

void MLAppView::addPropertyView(MLSymbol modelProp, MLWidget* widget, MLSymbol widgetProp)
{
	if(modelProp && widget && widgetProp)
		mpReporter->addPropertyViewToMap(modelProp, widget, widgetProp);
}

void MLAppView::addWidgetToView(MLWidget* pW, const MLRect& r, MLSymbol name = MLSymbol())
{
	addWidget(pW, name);
	pW->setGridBounds(r);
	pW->addListener(mpResponder);
	addAndMakeVisible(pW->getComponent());
}

#pragma mark component add utility methods
//

MLDial* MLAppView::addDial(const char * displayName, const MLRect & r, const MLSymbol p,
	const Colour& color, const float sizeMultiplier)
{
	MLDial* dial = new MLDial;
	dial->setTargetPropertyName(p);
	dial->setSizeMultiplier(sizeMultiplier);
	dial->setDialStyle (MLDial::Rotary);
	dial->setFillColor(color); 		

	addWidgetToView(dial, r, p);
	
	addPropertyView(p, dial, MLSymbol("value"));
	
	if (strcmp(displayName, ""))
	{
		addLabelAbove(dial, displayName);		
	}
	return dial;
}

MLMultiSlider* MLAppView::addMultiSlider(const char * displayName, const MLRect & r, const MLSymbol propName, 
	int numSliders, const Colour& color)
{
	MLMultiSlider* slider = new MLMultiSlider;
	slider->setNumSliders(numSliders);
	slider->setTargetPropertyName(propName);
	slider->setFillColor(color);
	addWidgetToView(slider, r, propName);
	
	for(int i=0; i<numSliders; ++i)
	{
		addPropertyView(propName.withFinalNumber(i), slider, MLSymbol("value").withFinalNumber(i));
	}
	
	if (strcmp(displayName, ""))
	{
		addLabelAbove(slider, displayName);
	}
	return slider;
}

MLMultiButton* MLAppView::addMultiButton(const char * displayName, const MLRect & r, const MLSymbol propName, 
	int n, const Colour& color)
{
	MLMultiButton* b = new MLMultiButton;
	b->setNumButtons(n);
	b->setTargetPropertyName(propName);
	b->setFillColor(color);
	addWidgetToView(b, r, propName);
	
	for(int i=0; i<n; ++i)
	{
		addPropertyView(propName.withFinalNumber(i), b, MLSymbol("value").withFinalNumber(i));
	}
	
	if (strcmp(displayName, ""))
	{
		addLabelAbove(b, displayName);
	}
	return b;
}

MLButton* MLAppView::addToggleButton(const char* displayName, const MLRect & r, const MLSymbol propName,
                                     const Colour& color, const float sizeMultiplier)
{
	MLButton* button = new MLToggleButton;
	button->setSizeMultiplier(sizeMultiplier);
	button->setTargetPropertyName(propName);
	button->setFillColor(color);
	addWidgetToView(button, r, propName);
	addPropertyView(propName, button, MLSymbol("value"));
	
	if (strcmp(displayName, ""))
	{
		addLabelAbove(button, displayName, sizeMultiplier);
	}
    
	return button;
}

MLButton* MLAppView::addTriToggleButton(const char* displayName, const MLRect & r, const MLSymbol propName,
                                     const Colour& color, const float sizeMultiplier)
{
	MLButton* button = new MLTriToggleButton;
	button->setSizeMultiplier(sizeMultiplier);
	button->setTargetPropertyName(propName);
	button->setFillColor(color);
	addWidgetToView(button, r, propName);
	addPropertyView(propName, button, MLSymbol("value"));
	
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
	b->setProperty("toggle", false);
	b->setImage(normalImg);
	addWidgetToView(b, r, name);
	return b;
}

MLTextButton* MLAppView::addTextButton(const char * displayName, const MLRect & r, const char * name, const Colour& color)
{	
	MLTextButton* b = new MLTextButton();
	b->setTargetPropertyName(name);
	b->setProperty("toggle", false);
	b->setFillColor(color);
	b->setPropertyImmediate("text", displayName);
	addWidgetToView(b, r, name);
	return b;
}

MLMenuButton* MLAppView::addMenuButton(const char * displayName, const MLRect & r, const char * menuName, const Colour& color)
{	
	MLMenuButton* b = new MLMenuButton();
	b->setTargetPropertyName(menuName);
	b->setFillColor(color);
	b->setProperty("text", "---");
	addWidgetToView(b, r, menuName);
	addPropertyView(menuName, b, MLSymbol("text"));
	
	if (strcmp(displayName, ""))
	{
		addLabelAbove(b, displayName);
	}
	return b;
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
		
		// quantize label top to 1/8 unit grid
		float gridSize = 0.125;
		float top = rr.top();
		top = (roundf(top/gridSize))*gridSize;
		rr.setTop(top);
		
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

void MLAppView::resized()
{
	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
	int u = myLookAndFeel->getGridUnitSize(); 

	for(auto it = mWidgets.begin(); it != mWidgets.end(); ++it)
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
	
	for(auto it = mWidgets.begin(); it != mWidgets.end(); ++it)
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

void MLAppView::setViewBoundsProperty()
{
	if(!mInitialized) return;
	
	MLSignal bounds(4);
	ComponentPeer* p = getPeer();
	if(!p) return;	
	Rectangle<int> peerBounds = p->getBounds();	
	bounds[0] = peerBounds.getX();
	bounds[1] = peerBounds.getY();
	bounds[2] = peerBounds.getWidth();
	bounds[3] = peerBounds.getHeight();

	// set property for saving, avoiding loop
	setPropertyImmediateExcludingListener("view_bounds", bounds, this);
}

void MLAppView::setWindowBounds(const MLSignal& bounds)
{
	ComponentPeer* p = getPeer();
	if(!p) return;
	bool fullScreen = 0;
	p->setBounds(Rectangle<int>(bounds[0], bounds[1], bounds[2], bounds[3]), fullScreen);
}

void MLAppView::windowMoved()
{
	setViewBoundsProperty();
}

void MLAppView::windowResized()
{
	setViewBoundsProperty();
}

