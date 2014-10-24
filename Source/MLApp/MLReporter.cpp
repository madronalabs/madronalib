
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
	// widget properties must be set immediate because they have no timers to propagate changes.
	// so we use the ReporterTimer to ensure that changes come from the message thread.
	mpWidget->setPropertyImmediate(mAttr, p);
}

// MLReporter
	
MLReporter::MLReporter(MLPropertySet* m) :
	MLPropertyListener(m)
{
	int size = 1 << 10;
	mSymbolData.resize(size);
	PaUtil_InitializeRingBuffer( &mSymbolRing, sizeof(MLSymbol), size, &(mSymbolData[0]) );
	mpTimer = std::tr1::shared_ptr<ReporterTimer>(new ReporterTimer(this));
	mpTimer->startTimer(33);
}

MLReporter::~MLReporter()
{
}

void MLReporter::doPropertyChangeAction(MLSymbol prop, const MLProperty& newVal)
{
	// enqueue property name
	int written = PaUtil_WriteRingBuffer( &mSymbolRing, &prop, 1 );
#if DEBUG
	if(written < 1)
	{
		debug() << "MLReporter::doPropertyChangeAction: ring buffer full! \n";
	}
#endif
}

// add a view.
// when property p changes, property attr of Widget w will be set to the new property's value.
//
void MLReporter::addPropertyViewToMap(MLSymbol p, MLWidget* w, MLSymbol attr)
{
	mPropertyViewsMap[p].push_back(MLPropertyViewPtr(new MLPropertyView(w, attr))); 
}

void MLReporter::viewProperties()
{
	while(PaUtil_GetRingBufferReadAvailable(&mSymbolRing) > 0)
	{
		// dequeue property name
		MLSymbol prop;
		PaUtil_ReadRingBuffer( &mSymbolRing, &prop, 1 );

		// do we have viewers for this property?
		MLPropertyViewListMap::iterator look = mPropertyViewsMap.find(prop);
		if (look != mPropertyViewsMap.end())
		{
			// run viewers
			MLPropertyViewList viewers = look->second;
			for(MLPropertyViewList::iterator vit = viewers.begin(); vit != viewers.end(); vit++)
			{
				MLPropertyViewPtr pv = (*vit);
				const MLPropertyView& v = (*pv);
				v.view(mpPropertyOwner->getProperty(prop));
			}
		}
	}
}

// MLModel::ModelTimer

MLReporter::ReporterTimer::ReporterTimer(MLReporter* pR) :
	mpOwner(pR)
{
}

MLReporter::ReporterTimer::~ReporterTimer()
{
	stopTimer();
}

void MLReporter::ReporterTimer::timerCallback()
{
     mpOwner->viewProperties();
}






