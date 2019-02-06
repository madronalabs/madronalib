
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLReporter.h"

// property viewing

MLPropertyView::MLPropertyView(MLWidget* w, ml::Symbol a) :
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

void MLReporter::PropertyListener::doPropertyChangeAction(ml::Symbol property, const MLProperty& newVal)
{
	// note that property names will collide across different PropertyListeners!
	mpOwnerReporter->enqueuePropertyChange(property, newVal);
}

// MLReporter
	
MLReporter::MLReporter()
{
	int size = 1 << 10;
	mChangeQueue = std::unique_ptr< Queue<Symbol> >(new Queue<Symbol>(size));
	mTimer.start([&](){ viewProperties(); }, milliseconds(33));
}

MLReporter::~MLReporter()
{
}

void MLReporter::enqueuePropertyChange(ml::Symbol prop, const MLProperty& newVal)
{
	// enqueue change
#if DEBUG
	if(!mChangeQueue->push(prop))
	{
		// std::cout << "MLReporter::doPropertyChangeAction: ring buffer full! \n"; // TODO something
	}
#else
	mChangeQueue->push(prop);
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
void MLReporter::addPropertyViewToMap(ml::Symbol modelProp, MLWidget* w, ml::Symbol widgetProp)
{
	mPropertyViewsMap[modelProp].push_back(MLPropertyViewPtr(new MLPropertyView(w, widgetProp))); 
}

void MLReporter::viewProperties()
{
	while(Symbol propName = mChangeQueue->pop())
	{
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




