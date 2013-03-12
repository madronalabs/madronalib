
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
			mImages.push_back(Image(Image::ARGB, mWidth, mHeight, true, SoftwareImageType()));
		}	
	}
	repaint();
}


void MLImageBank::setDims(int w, int h)
{
	mWidth = w;
	mHeight = h;
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
		if(p.mPrevIndex != p.mIndex)
		{
			if ((p.mIndex >= 0) && (mImages[p.mIndex].isValid()))
			{
				g.drawImage (mImages[p.mIndex],
					p.mLocation.x(), p.mLocation.y(), mWidth, mHeight,
					0, 0, mWidth, mHeight,
					false);
				p.mPrevIndex = p.mIndex;
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
	}
	return changed;
}

bool MLImageBank::panelIndexChanged(unsigned pIdx)
{
	bool r = false;
	if(pIdx < mPanels.size())
	{
		Panel& p = mPanels[pIdx];
		r = (p.mPrevIndex != p.mIndex);
	}
	return r;
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


