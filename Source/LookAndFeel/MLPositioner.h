
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_POSITIONER__
#define __ML_POSITIONER__

#include "MLUI.h"
#include "MLDebug.h"

class MLPositioner
{
public:
	MLPositioner();
	MLPositioner(const MLRect& bounds);
	~MLPositioner();
	
 	typedef enum 
	{
		kHorizontal = 0,
		kVertical = 1,
		kRectangle = 2
	}	Geometry;

	typedef enum 
	{
		kOdd = 1 << 0,
		kSquare = 1 << 1,
		kUseElementAspectRatio = 1 << 2,
		kOnePixelOverlap = 1 << 3
	}	SizeFlags;

	void setBounds(const MLRect& r) { mBounds = r; layout(); }
	void setElements(const int e) { mElements = e; layout(); }
	void setGeometry(const Geometry g) { mGeometry = g; layout(); }
	void setSizeFlags(const int f) { mSizeFlags = (SizeFlags)f; layout(); }
	void setMargin(const float m) { mMarginFraction = m; layout(); }
	void setElementAspectRatio(const float m) { mElementAspectRatio = m; layout(); }
	
	Vec2 getElementPositionWithMargin(int elementIdx); 
	Vec2 getElementPosition(int elementIdx); 
	Vec2 getElementSizeWithMargin();
	Vec2 getElementSize(); 
	Vec2 getCenter(); 
	MLRect getElementBoundsWithMargin(int elementIdx); 
	MLRect getElementBounds(int elementIdx); 
	MLRect getBounds(); 
	MLRect getLocalBounds(); 
	MLRect getLocalOutline();
	//MLRect getElementRangeBounds(int startIdx, int endIdx); 
	
	int getElementUnderPoint(const Vec2& p);

private:
	void layout();

	MLRect mBounds;
 	int mElements;
	Geometry mGeometry;
	SizeFlags mSizeFlags;
	float mMarginFraction;
	float mElementAspectRatio;
	
	Vec2 mTopLeft;
	Vec2 mCenter;
	Vec2 mElementsMargin;
	Vec2 mElementWithMarginSize;
	Vec2 mElementMarginSize;
	Vec2 mElementSize;
	Vec2 mElementStep;
	Vec2 mElemsXY;
};

#endif // __ML_POSITIONER__

