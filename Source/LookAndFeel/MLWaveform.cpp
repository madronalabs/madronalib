
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLWaveform.h"

MLWaveform::MLWaveform () :
    mpSignal(0)
{
	MLWidget::setComponent(this);
	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
	setOpaque(myLookAndFeel->getDefaultOpacity());
	setBufferedToImage(myLookAndFeel->getDefaultBufferMode());
	setPaintingIsUnclipped(myLookAndFeel->getDefaultUnclippedMode());

 	setColour (foregroundColor, findColour(MLLookAndFeel::labelColor));
    setRepaintsOnMouseActivity (false);
}

MLWaveform::~MLWaveform()
{
}

void MLWaveform::lookAndFeelChanged()
{
	setInterceptsMouseClicks(false, false);
}

void MLWaveform::colourChanged()
{
    lookAndFeelChanged();
}

void MLWaveform::setSignalToView(MLSignal* sig)
{
    mpSignal = sig;
    
    // TODO precalculate waveform data here
}

void MLWaveform::setAttribute(MLSymbol attr, float val)
{
	MLWidget::setAttribute(attr, val);
	
    // debug() << "MLWaveform " << getWidgetName() << ":" << attr << " = " << val << "\n";
    
	repaint();
}

void MLWaveform::paint (Graphics& g)
{
	enterPaint();
    
	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
	if (isOpaque())
		myLookAndFeel->drawBackground(g, this);
	
	float margin = myLookAndFeel->getSmallMargin() * myLookAndFeel->getGridUnitSize();
	int width = getWidth();
	int height = getHeight();
    int hh = height/2;
    int yMax = hh - margin;
    
	// draw shape
	Path envPath;
    
    if (mpSignal)
    {
        MLRange posRange(0, width, 0, mpSignal->getSize());
        MLRange ampRange1(0, 1., hh, hh - yMax);
        MLRange ampRange2(0, 1., hh, hh + yMax);
        
        envPath.startNewSubPath(posRange(0), ampRange1(0));
        
        for(int i=0; i<width; i += 2)
        {
            //float pos = posRange(i);
            //float amp = ampRange1((*mpSignal)[i]);
            
            envPath.lineTo(posRange(i), ampRange1((*mpSignal)[i]));
        }
        
        g.setColour(findColour(MLLookAndFeel::outlineColor));
        g.strokePath(envPath, PathStrokeType (mOutlineThickness));
        g.setColour(findColour(MLLookAndFeel::outlineColor).withAlpha(0.125f));
        g.fillPath(envPath);
        
        /*
        // lines down
        envPath.clear();
        envPath.startNewSubPath(attX,  floor(margin) + 0.5);
        envPath.lineTo(attX,  floor(h - margin) + 0.5);
        envPath.startNewSubPath(decX,  floor(susHeight) + 0.5);
        envPath.lineTo(decX,  floor(h - margin) + 0.5);
        envPath.startNewSubPath(susX,  floor(susHeight) + 0.5);
        envPath.lineTo(susX,   floor(h - margin) + 0.5);
        g.setColour(findColour(MLLookAndFeel::outlineColor));
        g.strokePath(envPath, PathStrokeType (mOutlineThickness / 2));
        */
    }
                     
    // TEST
    Path tbounds;
    const Rectangle<int> & boundsRect ( getLocalBounds());
    tbounds.addRectangle(boundsRect);
    g.setColour(findColour(MLLookAndFeel::outlineColor).withAlpha(0.125f));
    g.fillPath(tbounds);
    g.setColour(findColour(MLLookAndFeel::outlineColor));
    g.strokePath(tbounds, PathStrokeType (mOutlineThickness));
}

void MLWaveform::visibilityChanged()
{
    
}



// resize this Dial and set the track rect, from which all the other
// parts are calculated
void MLWaveform::resizeWidget(const MLRect& b, const int u)
{
	MLWidget::resizeWidget(b, u);
	mOutlineThickness = u/96.f;
}

