
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLDrawing.h"

MLDrawing::Operation::Operation(eOperationType typeParam, float a0, float a1, float a2, float a3)
{
	type = typeParam;
	args[0] = a0;
	args[1] = a1;
	args[2] = a2;
	args[3] = a3;
}

MLDrawing::Operation::Operation(eOperationType typeParam, int a0, int a1, int a2, int a3)
{
	type = typeParam;
	args[0] = a0;
	args[1] = a1;
	args[2] = a2;
	args[3] = a3;
}

MLDrawing::Operation::~Operation()
{
}


//

MLDrawing::MLDrawing()
{
	MLWidget::setComponent(this);
	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
	setOpaque(myLookAndFeel->getDefaultOpacity());
	setBufferedToImage(myLookAndFeel->getDefaultBufferMode());
	setPaintingIsUnclipped(myLookAndFeel->getDefaultUnclippedMode());

	// setup component
	setRepaintsOnMouseActivity (false);
	setInterceptsMouseClicks(false, false);
	
	// set default drawing state
	mLineThickness = 1.0f;
	mLineColor = ( myLookAndFeel->findColour(MLLookAndFeel::darkLabelColor));
	Colour bg = myLookAndFeel->findColour(MLLookAndFeel::backgroundColor);
	
	mLightColor =  myLookAndFeel->findColour(MLLookAndFeel::labelColor);
	mDarkColor =  myLookAndFeel->findColour(MLLookAndFeel::darkLabelColor);
}

MLDrawing::~MLDrawing()
{

}

int MLDrawing::addPoint(const Vec2& p)
{
	mGridPoints.push_back(p);
	mTransformedPoints.push_back(p);
	assert(mGridPoints.size() == mTransformedPoints.size());
	return(mGridPoints.size() - 1);
}

void MLDrawing::addOperation(eOperationType op)
{
	mOperations.push_back(MLDrawing::Operation(op, 0, 0, 0, 0));
}

void MLDrawing::addOperation(eOperationType op, float arg1, float arg2, float arg3, float arg4)
{
	mOperations.push_back(MLDrawing::Operation(op, arg1, arg2, arg3, arg4));
}

void MLDrawing::addOperation(eOperationType op, int arg1, int arg2, int arg3, int arg4)
{
	mOperations.push_back(MLDrawing::Operation(op, arg1, arg2, arg3, arg4));
}

void MLDrawing::dumpOperations()
{
	debug() << "MLDrawing " << getWidgetName() << ": \n";
	int c = 0;
	for(std::vector<MLDrawing::Operation>::iterator it = mOperations.begin(); it != mOperations.end(); it++)
	{
		const MLDrawing::Operation& op = *it;
		debug() << "    op" << c++ << ": " << op.type << "\n";
		debug() << "        args: ";
		for (int i=0; i<4; ++i)
		{
			debug() << op.args[i] << " ";
		}
		debug() << "\n";

	}
}

void MLDrawing::drawArrowhead(Graphics& g, const Vec2& p1, const Vec2& p2, float scale)
{
	Vec2 p1c = p1.getIntPart() + Vec2(0.5f, 0.5f);
	Vec2 p2c = p2.getIntPart() + Vec2(0.5f, 0.5f);
	float angle = atan2(-(p2c.x() - p1c.x()), (p2c.y() - p1c.y()));
	Path head;
	head.startNewSubPath(0, 0);
	head.lineTo(-2.25*scale, -5.5*scale);
	head.lineTo(2.25*scale, -5.5*scale);
	head.closeSubPath();
	
	g.fillPath(head, AffineTransform::rotation(angle).translated (p2c.x(), p2c.y()));
}

void MLDrawing::paint(Graphics& g)
{
	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
	float fu = myLookAndFeel->getGridUnitSize();

	if (isOpaque())
		myLookAndFeel->drawBackground(g, this);
	
	// defaults
	float ft = 1.0f * fu / 64.;
	float arrowScale = fu / 48.;
	mLineColor = mDarkColor;
	g.setColour(mLineColor);
//	int w = getWidth();
//	int h = getHeight();
	
	/*
	// TEST TODO auto border / background?
	Path tbounds;
	const MLRect boundsRect ( getLocalBounds());	
	tbounds.addRectangle(boundsRect);
	g.setColour(Colours::blue.withAlpha(0.5f));	
	g.fillPath(tbounds);
	g.setColour(Colours::red);	
	g.strokePath(tbounds, PathStrokeType(1.0f));
	*/
	
//debug() << "painting MLDrawing " << getWidgetName() << "\n";
//int c = 0;	

	for(auto op : mOperations)
	{
		Path p;
		Vec2 p1, p2;
		//const MLDrawing::Operation& op = *it;

//debug() << "    painting op " << c++ << "\n";	
		for(int i=0; i<4; ++i)
		{
			assert(within((int)op.args[i], 0, (int)mTransformedPoints.size()));
		}
		
		switch(op.type)
		{
			case drawLine:				
				p1 = mTransformedPoints[(int)op.args[0]];
				p2 = mTransformedPoints[(int)op.args[1]];				
				p.startNewSubPath(MLToJucePoint(correctPoint(p1)));
				p.lineTo(MLToJucePoint(correctPoint(p2)));
				g.strokePath(p, PathStrokeType(ft));
				break;
			case drawLineArrowStart:
			case drawLineArrowEnd:
				p1 = mTransformedPoints[(int)op.args[0]];
				p2 = mTransformedPoints[(int)op.args[1]];				
				p.startNewSubPath(MLToJucePoint(correctPoint(p1)));
				p.lineTo(MLToJucePoint(correctPoint(p2)));
				g.strokePath(p, PathStrokeType(ft));
				drawArrowhead(g, p1, p2, arrowScale);
				break;
			case drawLineArrowBoth:
			break;
			case setLineThickness:
				mLineThickness = op.args[0];
				ft = mLineThickness * fu / 64.;
			break;
			case setLightColor:
				g.setColour(mLightColor);
			break;
			case setDarkColor:
				g.setColour(mDarkColor);
			break;
		}
	}
}

void MLDrawing::setPixelOffset(const Vec2& f) 
{ 
//	debug() << "BEFORE: ";
//	dumpOperations();
	mPixelOffset = f; 
//	debug() << "AFTER: ";
//	dumpOperations();
}
	
void MLDrawing::resizeWidget(const MLRect& b, const int u)
{
	// adapt vrect to juce rect
	Component* pC = getComponent();
	if(pC)
	{
		MLRect bb = b + mPixelOffset;
		if (bb.height() == 0) bb.setHeight(1);
		if (bb.width() == 0) bb.setWidth(1);
	
		// adapt vrect to juce rect
		Rectangle<int> c(bb.left(), bb.top(), bb.width(), bb.height());
		
		pC->setBounds(c);
	}
	
//debug() << "RESIZING DRAWING " << getWidgetName() << ":\n";
	// iterate over points
	int size = mGridPoints.size();
	for(int i = 0; i<size; ++i)
	{
		mTransformedPoints[i] = mGridPoints[i]*u;
//debug() << "pt. " << i << ":" << mTransformedPoints[i] <<  " ";
	}
//debug() << "\n";
}
 
