
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLImageBank.h"

MLImageBank::MLImageBank() : mWidth(0), mHeight(0), mNumImages(0) {};

MLImageBank::~MLImageBank()
{
}

void MLImageBank::buildImages()
{
	mImages.clear();
	if ((mWidth > 0) && (mHeight > 0) && (mNumImages > 0))
	{
		for(unsigned i=0; i<mNumImages; ++i)
		{
            Image newImage(Image::ARGB, mWidth + 1, mHeight + 1, true, NativeImageType());
            newImage.clear(Rectangle<int>(0, 0, mWidth + 1, mHeight + 1), Colours::transparentBlack);
			mImages.push_back(newImage);
		}	
	}
	repaint();
}

void MLImageBank::setImageDims(int w, int h)
{
	mWidth = clamp(w, 8, 16384);
	mHeight = clamp(h, 8, 16384);
	buildImages();
}

void MLImageBank::setImages(unsigned n)
{
	mNumImages = n;
	buildImages();
}

Image& MLImageBank::getImage(unsigned imageIdx)
{
	if (imageIdx < mImages.size())
	{
		return mImages[imageIdx];
	}
	else
	{
		return mNullImage;
	}	
}

// just mark all panels as dirty.
void MLImageBank::repaint()
{
	unsigned panels = mPanels.size();
	for(unsigned i=0; i<panels; ++i)
	{
		Panel& p = mPanels[i];
		p.mPrevIndex = -1;
	}
}

void MLImageBank::paint (Graphics& g)
{
	unsigned panels = mPanels.size();
	for(unsigned i=0; i<panels; ++i)
	{
		Panel& p = mPanels[i];
		if ((p.mIndex >= 0) && (mImages[p.mIndex].isValid()))
		{
			{
				Graphics::ScopedSaveState state(g);
				g.reduceClipRegion(p.mLocation.x(), p.mLocation.y(), mWidth, mHeight);
				g.drawImageAt (mImages[p.mIndex], p.mLocation.x(), p.mLocation.y(), false);
			}
		}
	}
}

void MLImageBank::addPanel(MLPoint p)
{
	mPanels.push_back(Panel(p));
}

void MLImageBank::clearPanels()
{
	mPanels.clear();
}
	
bool MLImageBank::setPanelValue(unsigned pIdx, float v)
{
	bool changed = false;
	if(pIdx < mPanels.size())
	{
		Panel& p = mPanels[pIdx];
		p.mIndex = valueToIndex(v);
		changed = (p.mPrevIndex != p.mIndex);
		p.mPrevIndex = p.mIndex;
	}
	return changed;
}

void MLImageBank::getPanelRect(unsigned pIdx, MLRect& r)
{
	if(pIdx < mPanels.size())
	{
		Panel& p = mPanels[pIdx];
		r = MLRect(p.mLocation.x(), p.mLocation.y(), mWidth, mHeight);
	}
}

// convert input value on [0., 1.] to image index.
int MLImageBank::valueToIndex(float v)
{
	int idx = -1;
	unsigned n = mImages.size();
	if(n)
	{
		idx = v*n;
		if (idx > (int)n-1) idx = n-1;
		if (idx < 0) idx = 0;
	}
	return idx;
}
