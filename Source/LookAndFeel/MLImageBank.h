
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_IMAGE_BANK__
#define __ML_IMAGE_BANK__

#include "MLUI.h"
#include <vector>

class MLImageBank
{
public:
	MLImageBank();
    ~MLImageBank();
	
	void setImageDims(int w, int h);
	void setImages(unsigned n);
	Image& getImage(unsigned imageIdx);
	
	void repaint ();				// mark everything as dirty
	void paint (Graphics& g);		// paint only the panels that need updating
	void addPanel(MLPoint p);
	void clearPanels();
	
	// set a panel's value, return true if the value is changed.
	bool setPanelValue(unsigned p, float v);

	void getPanelRect(unsigned p, MLRect& r);

private:
	class Panel
	{
	public:
		Panel(MLPoint p) : mValue(0.), mIndex(0), mPrevIndex(-1), mLocation(p){};
		~Panel() {};
		float mValue;
		int mIndex;
		int mPrevIndex;
		MLPoint mLocation;
	};
	
	int valueToIndex(float v);
	void buildImages();
	
	std::vector<Image> mImages;
	std::vector<Panel> mPanels;
	Image mNullImage;

	unsigned mWidth;
	unsigned mHeight;
	unsigned mNumImages;
};


#endif // __ML_IMAGE_BANK__

