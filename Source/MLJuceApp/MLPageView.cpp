
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLPageView.h"

MLPageView::MLPageView(MLWidget::Listener* pResp, MLReporter* pRep) :
	MLAppView(pResp, pRep),
    mpParent(0),
	mCurrPage(-1)
{
	setOpaque (false);
	MLWidget::setComponent(this);
	setInterceptsMouseClicks (false, true);
	mAnimator.addChangeListener (this);
}

MLPageView::~MLPageView()
{
	int size = mPages.size();
	for(int i=0; i<size; ++i)
	{		
		delete mPages[i];
	}
}

void MLPageView::setParent(MLAppView* pParent)
{
	mpParent = pParent;
}

MLAppView* MLPageView::addPage()
{
	MLAppView* newPage = new MLAppView(mpResponder, mpReporter);
    int pageNum = mPages.size();
    String pageStr = String("page") + String(pageNum);
    newPage->setName(pageStr);
    
    // debug() << "ADDING page " << pageStr << "\n";
	mPages.push_back(newPage);
	addChildComponent(newPage);
	newPage->setBounds(0, 0, getWidth(), getHeight());	
	addWidgetToView(newPage, MLRect(0, 0, getWidth(), getHeight()), MLSymbol("page").withFinalNumber(pageNum));
	return newPage;
}

MLAppView* MLPageView::addPage(MLAppView* newPage)
{
	mPages.push_back(newPage);
	addChildComponent(newPage);
	newPage->setBounds(0, 0, getWidth(), getHeight());	

	int pageNum = mWidgets.size();
	addWidgetToView(newPage, MLRect(0, 0, getWidth(), getHeight()), MLSymbol("page").withFinalNumber(pageNum));	
	return newPage;
}

void MLPageView::resized()
{
	int w = getWidth();
	int h = getHeight();
	Rectangle<int> myBounds(0, 0, w, h);
	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
	int u = myLookAndFeel->getGridUnitSize(); 
	int margin = u;

	int numPages = getNumPages();
	for(int i=0; i<numPages; ++i)
	{
		mPages[i]->setVisible(i == mCurrPage);
		mPages[i]->setBounds(myBounds.translated((w + margin)*(i - mCurrPage), 0));
		mPages[i]->repaint();
	}
}

void MLPageView::goToPage (int destPage, bool animate, Component* prevButton, Component* nextButton)
{
	int duration = 500;
	float targetAlpha;
	
	//with this on, animations fail sometimes. with this off, they fail in a different way,always.
	bool proxy = true;
	
	int w = getWidth();
	int h = getHeight();
	Rectangle<int> localBounds(0, 0, w, h);
	
    int pages = mPages.size();
	if(!pages) return;
    if(mCurrPage == destPage) return;

	MLLookAndFeel* myLookAndFeel = MLLookAndFeel::getInstance();
	int u = myLookAndFeel->getGridUnitSize(); 
	
	// margin between pages prevents invisible components from overlapping
	// those onscreen
	int margin = u;
	int newPage = clamp(destPage, 0, (int)mPages.size() - 1);

	animate = true;
	if ((animate) && (newPage != mCurrPage) && (mCurrPage >= 0))
	{
		// line up all pages from new through current offscreen
		// and make visible all in line up
		
		int start = min(mCurrPage, newPage);
		int end = max(mCurrPage, newPage);
		for(int i=start; i <= end; ++i)
		{
			mPages[i]->setBounds(localBounds.translated((w + margin)*(i - mCurrPage), 0));
			mPages[i]->setVisible(true);
		}
				
		// scroll past all to new page
		for(int i=start; i <= end; ++i)
		{
			if (i == newPage)
			{
				targetAlpha = 1.f;
			}
			else if (i == mCurrPage)
			{
				targetAlpha = 0.f;
			}
			else 
			{
				// allow intermediate pages to show? we don't really do these
				// transitions anyway.  
				targetAlpha = 1.f;
			}
            
            // NOTE: this can cause a problem stopping a running thread in CachedImage::stop if called at just the wrong time-- TODO investigate!
			mAnimator.animateComponent (mPages[i], localBounds.translated((w + margin)*(i - newPage), 0),
				targetAlpha, duration, proxy, 4.0, 0.25);
		}
		
		// animate buttons 
		if (prevButton)
		{
			targetAlpha = (newPage > 0) ? 1.f : 0.f;
			mAnimator.animateComponent (prevButton, prevButton->getBounds(), 
				targetAlpha, duration, proxy, 1.0, 1.0);
//	debug() << "prev alpha: " << targetAlpha << "\n";
		}

		// animate buttons 
		if (nextButton)
		{
			int last = mPages.size() - 1;
			targetAlpha = (newPage < last) ? 1.f : 0.f;
			mAnimator.animateComponent (nextButton, nextButton->getBounds(),
				targetAlpha, duration, proxy, 1.0, 1.0);
				
//	debug() << "next alpha: " << targetAlpha << "\n";
		}
	}
    else
    {
        for(int p=0; p<pages; ++p)
        {
            if(p == newPage)
            {
                mPages[p]->setBounds(localBounds);
                mPages[p]->setVisible(true);
            }
            else
            {
                mPages[p]->setBounds(localBounds.translated((w + margin), 0));
                mPages[p]->setVisible(false);
            }
        }
    }

	mCurrPage = newPage;
}

void MLPageView::changeListenerCallback (ChangeBroadcaster* pSender)
{
	if (pSender == &mAnimator)
	{
		if(mAnimator.isAnimating(mPages[mCurrPage]))
		{
			// animation start
			// turn off openGL timers
//debug() << "anim off\n"		;
		}
		else
		{
//debug() << "anim on\n"	;		
			// animation end
			// make invisible all but new page
			int numPages = getNumPages();
			for(int i=0; i<numPages; ++i)
			{
				mPages[i]->setVisible(i == mCurrPage);
			}		
		}
	}
}

