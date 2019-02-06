
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_PAGE_VIEW_H__
#define __ML_PAGE_VIEW_H__

#include "JuceHeader.h"

#include <vector>
#include <map>

//#include "MLPluginBinaryData.h"
#include "MLUIBinaryData.h"
#include "MLButton.h"
#include "MLDrawableButton.h"
#include "MLTextButton.h"
#include "MLMultiSlider.h"
#include "MLLookAndFeel.h"
#include "MLMultiButton.h"
#include "MLEnvelope.h"
#include "MLDefaultFileLocations.h"
#include "MLJuceFilesMac.h"
#include "MLAppView.h"

#pragma mark page view

class MLPageView : 
	public MLAppView,
	public ChangeListener
{
public:
    MLPageView(MLWidget::Listener* pResp, MLReporter* pRep);
	~MLPageView();

	void resized();

	void setParent(MLAppView* pParent);
	MLAppView* addPage();
	MLAppView* addPage(MLAppView* newPage);
	void goToPage (int destPage, bool animate = true, Component* prevButton = 0, Component* nextButton = 0);
	int getCurrentPage(){ return mCurrPage; }
	int getNumPages(){ return mPages.size(); }

	void changeListenerCallback (ChangeBroadcaster*);
	
private:	
	
	// TODO use Widget container instead of this vector.  means rewriting internals when you have a spare hour.
	std::vector<MLAppView*> mPages;

	MLAppView* mpParent;
	int mCurrPage;
	ComponentAnimator mAnimator;
	
};



#endif // __ML_PAGE_VIEW_H__