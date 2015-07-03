
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLReporter.h"

// property viewing

MLPropertyView::MLPropertyView(MLWidget* w, MLSymbol a) :
	mpWidget(w),
	mAttr(a)
{
}

MLPropertyView::~MLPropertyView()
{
}

void MLPropertyView::view(const MLProperty& p) const
{
	// widget properties must be set immediately because they have no timers to propagate changes.
	// We use the ReporterTimer to ensure that changes come from the message thread.
	mpWidget->setPropertyImmediate(mAttr, p);
}

// MLReporter::PropertyListener

void MLReporter::PropertyListener::doPropertyChangeAction(MLSymbol property, const MLProperty& newVal)
{
	// note that property names will collide across different PropertyListeners!
	mpOwnerReporter->enqueuePropertyChange(property, newVal);
}

// MLReporter::ReporterTimer

MLReporter::ReporterTimer::ReporterTimer(MLReporter* pR) :
mpOwnerReporter(pR)
{
	startTimer(33);
}

MLReporter::ReporterTimer::~ReporterTimer()
{
	stopTimer();
}

void MLReporter::ReporterTimer::timerCallback()
{
	mpOwnerReporter->viewProperties();
}

// MLReporter
	
MLReporter::MLReporter()
{
	int size = 1 << 10;
	mChangeData.resize(size);
	PaUtil_InitializeRingBuffer( &mChangeQueue, sizeof(MLSymbol), size, &(mChangeData[0]) );
	mpTimer = std::unique_ptr<ReporterTimer>(new ReporterTimer(this));
}

MLReporter::~MLReporter()
{
}

void MLReporter::enqueuePropertyChange(MLSymbol prop, const MLProperty& newVal)
{
	// enqueue change
#if DEBUG
	int written = PaUtil_WriteRingBuffer( &mChangeQueue, &prop, 1 );
	if(written < 1)
	{
		debug() << "MLReporter::doPropertyChangeAction: ring buffer full! \n";
	}
#else
	PaUtil_WriteRingBuffer( &mChangeQueue, &prop, 1 );
#endif
	// store changed value
	mCurrentProperties.setProperty(prop, newVal);
}

void MLReporter::listenTo(MLPropertySet* p)
{
	pListeners.push_back(MLPropertyListenerPtr(new MLReporter::PropertyListener(this, p)));
}

void MLReporter::fetchChangedProperties()
{
	for(int i=0; i<pListeners.size(); ++i)
	{
		pListeners[i]->updateChangedProperties();
	}
}

void MLReporter::fetchAllProperties()
{
	for(int i=0; i<pListeners.size(); ++i)
	{
		pListeners[i]->updateAllProperties();
	}
}

// add a view. This means that:
// when the Model's Property p changes, Property widgetProp of Widget w will be set to the new property's value.
//
void MLReporter::addPropertyViewToMap(MLSymbol modelProp, MLWidget* w, MLSymbol widgetProp)
{
	mPropertyViewsMap[modelProp].push_back(MLPropertyViewPtr(new MLPropertyView(w, widgetProp))); 
}

void MLReporter::viewProperties()
{
	while(PaUtil_GetRingBufferReadAvailable(&mChangeQueue) > 0)
	{
		// dequeue name of changed property
		MLSymbol propName;
		PaUtil_ReadRingBuffer( &mChangeQueue, &propName, 1 );
		
		// do we have viewers for this property?
		MLPropertyViewListMap::iterator look = mPropertyViewsMap.find(propName);
		if (look != mPropertyViewsMap.end())
		{
			// run viewers
			MLPropertyViewList viewers = look->second;
			for(MLPropertyViewList::iterator vit = viewers.begin(); vit != viewers.end(); vit++)
			{
				MLPropertyViewPtr pv = (*vit);
				const MLPropertyView& v = (*pv);
				v.view(mCurrentProperties.getProperty(propName));
			}
		}
	}
}




