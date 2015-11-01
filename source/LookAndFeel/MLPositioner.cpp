
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLPositioner.h"

MLPositioner::MLPositioner(const MLRect& b) :
	mBounds(b),
	mElements(1),
	mGeometry(kHorizontal),
	mSizeFlags(kOdd),
	mMarginFraction(0.),
	mElementAspectRatio(1.)
{
	layout(); 
}

MLPositioner::MLPositioner() :
	mBounds(),
	mElements(1),
	mGeometry(kHorizontal),
	mSizeFlags(kOdd),
	mMarginFraction(0.),
	mElementAspectRatio(1.)
{
	layout(); 
}

MLPositioner::~MLPositioner()
{
}

void MLPositioner::layout()
{
	// tweak overall bounds to center on a pixel
	if (mSizeFlags & kOdd)
	{
		mBounds[2] -= !((int)mBounds[2] & 1);
		mBounds[3] -= !((int)mBounds[3] & 1);
	}
	
	mTopLeft = Vec2 (mBounds[0], mBounds[1]);
	Vec2 boundsSize (mBounds[2], mBounds[3]);
	switch (mGeometry)
	{
		default:
		case kHorizontal:
			mElemsXY = Vec2(mElements, 1);
		break;
		case kVertical:
			mElemsXY = Vec2(1, mElements);
		break;
		case kRectangle:
			// TODO fit elements into bounds depending on w/h ratio
			// and element aspect ratio 
			mElemsXY = Vec2(mElements / 2, 2);
		break;
	}
	
	mCenter = getPixelCenter(mBounds);
	Vec2 floatWithMarginSize((boundsSize - Vec2(1, 1)) / mElemsXY);
	if (mSizeFlags & kOnePixelOverlap)
	{
		mElementWithMarginSize = floatWithMarginSize.getIntPart() + Vec2(1, 1); 
	}
	else
	{
		mElementWithMarginSize = floatWithMarginSize.getIntPart();
	}
	
	// make square if needed
	//
	if (mSizeFlags & kSquare)
	{
		mElementWithMarginSize[0] = mElementWithMarginSize[1] = min(mElementWithMarginSize[0], mElementWithMarginSize[1]);
	}
	
	mElementMarginSize = (mElementWithMarginSize*Vec2(mMarginFraction)).getIntPart(); 
	mElementSize = mElementWithMarginSize - mElementMarginSize;

	if (mSizeFlags & kOdd)
	{
		mElementSize[0] -= !((int)mElementSize[0] & 1);
		mElementSize[1] -= !((int)mElementSize[1] & 1);
	}
	
	if (mSizeFlags & kOnePixelOverlap)
	{
		mElementStep = mElementWithMarginSize - Vec2(1, 1); 	
	}
	else
	{
		mElementStep = mElementWithMarginSize; 	
	}
	
	// get margin bounding all elements 
	mElementsMargin = ((boundsSize - mElementStep*mElemsXY) / 2).getIntPart();	
}

// get local topLeft of an element surrounded by its margin. 
//
Vec2 MLPositioner::getElementPositionWithMargin(int elementIdx)
{
	Vec2 stepsXY;
	switch (mGeometry)
	{
		case kHorizontal:
			stepsXY = Vec2(elementIdx, 0);
		break;
		case kVertical:
			stepsXY = Vec2(0, elementIdx);
		break;
		case kRectangle:
		default:
			int y = elementIdx/mElemsXY[0];
			int x = elementIdx - y*mElemsXY[0];
			stepsXY = Vec2(x, y);
		break;
	}
	
	return correctPoint (mElementsMargin + stepsXY*mElementStep);
}

// get local topLeft of an element within its margin. 
//
Vec2 MLPositioner::getElementPosition(int elementIdx)
{
	return correctPoint(getElementPositionWithMargin(elementIdx) + mElementMarginSize/Vec2(2.f));
}

// get local bounds of an element and its margin. 
//
MLRect MLPositioner::getElementBoundsWithMargin(int elementIdx)
{
	Vec2 e = getElementPositionWithMargin(elementIdx);
	
	return correctRect(MLRect(e, e + mElementWithMarginSize));
}

// get local bounds of an element. 
//
MLRect MLPositioner::getElementBounds(int elementIdx)
{
	Vec2 e = getElementPosition(elementIdx);
	
	return correctRect(MLRect(e, e + mElementSize));
}

// get overall bounds in parent.
//
MLRect MLPositioner::getBounds()
{
	return mBounds;
}

MLRect MLPositioner::getLocalBounds()
{
	return mBounds - mTopLeft;
}

// get smallest rect outlining all elements. For now this is different than getLocalBounds, because
// we might want to allow components to draw into the margin within getLocalBounds. 
//  
MLRect MLPositioner::getLocalOutline()
{
//	MLRect out = mBounds - mTopLeft;
//	out.expand(-mElementsMargin*2);
//	return correctRect(out);

	// TEMP works for horiz only
	return MLRect(getElementBounds(0).getTopLeft(), getElementBounds(mElements - 1).getBottomRight());
}

Vec2 MLPositioner::getElementSize()
{
	return mElementSize;
}

Vec2 MLPositioner::getElementSizeWithMargin()
{
	return mElementSize + mElementMarginSize;
}

Vec2 MLPositioner::getCenter()
{
	return mCenter;
}

int MLPositioner::getElementUnderPoint(const Vec2& p)
{
	int idx = -1;
	Vec2 pLocal = p - mTopLeft;
	for(int i=0; i<mElements; ++i)
	{
		if (getElementBounds(i).contains(p)) 
		{
			idx = i;
			break;
		}
	}
	return idx;
}

