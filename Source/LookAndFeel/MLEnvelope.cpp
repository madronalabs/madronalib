
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLEnvelope.h"

static const float kMinAttack = 0.0001;
static const float kMinDecay = 0.0001;

MLEnvelope::MLEnvelope ()
{
	MLWidget::setComponent(this);
	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
	setOpaque(myLookAndFeel->getDefaultOpacity());
	setBufferedToImage(myLookAndFeel->getDefaultBufferMode());
	setPaintingIsUnclipped(myLookAndFeel->getDefaultUnclippedMode());

	mDARMode = false;
 	setColour (foregroundColor, findColour(MLLookAndFeel::labelColor));
    setRepaintsOnMouseActivity (false);
}

MLEnvelope::~MLEnvelope()
{
}

void MLEnvelope::doPropertyChangeAction(MLSymbol property, const MLProperty& val)
{
	repaint();
}

void MLEnvelope::lookAndFeelChanged()
{
	setInterceptsMouseClicks(false, false);
}

void MLEnvelope::colourChanged()
{
    lookAndFeelChanged();
}

void MLEnvelope::paint (Graphics& g)
{	
	float mDelay = getFloatProperty("delay");
	float mAttack = getFloatProperty("attack");
	float mSustain = getFloatProperty("sustain");
	float mDecay = getFloatProperty("decay");
	float mRelease = getFloatProperty("release");
	float r = getFloatProperty("repeat");
	float mRepeat = (r > 0.f) ? (1.f / (r + 0.0001f)) : 0.f;

	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
	if (isOpaque()) 
		myLookAndFeel->drawBackground(g, this);	
	
	float margin = myLookAndFeel->getSmallMargin() * myLookAndFeel->getGridUnitSize();
	int w = getWidth()-1;
	int h = getHeight()-1;
	
	float totalTime;
	float startX, delX, attX, decX, susX, relX, repX, endX;
	float attTime, decTime, relTime, susTime;
	float leadIn = 0.01;
	float leadOut = 0.01;
	float susHeight;
	float epsilon = 0.0005;
	float susLevel = mDARMode ? 1.0f : mSustain;
	attTime = mAttack + kMinAttack;
	decTime = mDecay * (1.f - susLevel) + kMinDecay;
	relTime = mRelease * susLevel + kMinDecay;
			
	// get times
	if (mDARMode)
	{
		susTime = (mSustain > 0.5) ? 1.f : 0.f; // is hold on?
		decTime = 0.;
		totalTime = leadIn + mDelay + attTime + susTime + relTime + leadIn + leadOut;
	}
	else
	{
		susTime = 1.f;
		totalTime = leadIn + attTime + decTime + susTime + relTime + leadIn + leadOut;
	}
	
	if (mRepeat > epsilon)
	{
		totalTime = max(totalTime, (float)(leadIn + mDelay + mRepeat + leadOut));
	}
	
	totalTime = max(totalTime, 0.01f);
	
	// get dims
	MLRange vRange(UnityRange);
	MLRange wRange(0.f, totalTime);
	vRange.convertTo(MLRange(h - margin, margin));
	wRange.convertTo(MLRange(margin, w - margin*2));
	float t = 0.;
	startX = wRange(0.);

	delX = wRange(t += mDelay);
	attX = wRange(t += attTime);
	decX = wRange(t += decTime);
	susX = wRange(t += susTime);
	relX = wRange(t += relTime);
	endX = wRange(totalTime);
	repX = wRange(leadIn + mDelay + mRepeat);
	susHeight = vRange(susLevel);
		
	// draw env shape
	Path envPath;
	envPath.startNewSubPath(startX,  floor(h - margin) + 0.5);
	envPath.lineTo(delX,  floor(h - margin) + 0.5);
	envPath.quadraticTo(delX,  floor(margin) + 0.5, attX, floor(margin) + 0.5); // up
	envPath.quadraticTo(attX,  floor(susHeight) + 0.5, decX,  floor(susHeight) + 0.5);  // down
	envPath.quadraticTo(decX,  floor(susHeight) + 0.5, susX,  floor(susHeight) + 0.5); // across
	envPath.quadraticTo(susX,  floor(h - margin) + 0.5, relX,  floor(h - margin) + 0.5); // down
	envPath.quadraticTo(relX,  floor(h - margin) + 0.5, endX,  floor(h - margin) + 0.5); // across
	g.setColour(findColour(MLLookAndFeel::outlineColor));
	g.strokePath(envPath, PathStrokeType (mOutlineThickness));	
	g.setColour(findColour(MLLookAndFeel::outlineColor).withAlpha(0.125f));
	g.fillPath(envPath);
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

	// draw repeat bracket
	if (mRepeat > epsilon)
	{		
		float thick = margin / 2;
		float high = margin * 1.5;
		envPath.clear();
		envPath.startNewSubPath(floor(delX - thick/2) , floor(h - margin - high));
		envPath.lineTo(floor(delX - thick/2) , floor(h - margin));
		envPath.lineTo(floor( repX + thick/2) , floor(h - margin));
		envPath.lineTo(floor( repX + thick/2) , floor(h - margin - high));
		g.strokePath(envPath, PathStrokeType (mOutlineThickness * 4));	

	}

//debug() << "DEL " << mDelay << " ATT " << attTime << " DEC " << decTime << " SUS " << susTime << " REL " << relTime << " REP " << mRepeat << "\n";
//debug() << "repeatX: " << repX << " delayX: " << delX << "\n";

	/*
	// TEST
	Path tbounds;
	const Rectangle<int> & boundsRect ( getLocalBounds());	
	tbounds.addRectangle(boundsRect);
	g.setColour(Colours::red);	
	g.strokePath(tbounds, PathStrokeType(0.5f));
	*/
}

void MLEnvelope::visibilityChanged()
{

}

void MLEnvelope::resizeWidget(const MLRect& b, const int u)
{
	MLWidget::resizeWidget(b, u);
	mOutlineThickness = u/96.f;
}
