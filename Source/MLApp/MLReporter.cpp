
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLReporter.h"

// --------------------------------------------------------------------------------
#pragma mark param viewing 

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
	mpWidget->setPropertyImmediate(mAttr, p);
}

// --------------------------------------------------------------------------------
#pragma mark MLReporter 
	
MLReporter::MLReporter(MLPropertySet* m) :
	MLPropertyListener(m)
{
}

MLReporter::~MLReporter()
{
}

// add a view.
// when property p changes, property attr of Widget w will be set to the new property's value.
//
void MLReporter::addPropertyViewToMap(MLSymbol p, MLWidget* w, MLSymbol attr)
{
	mPropertyViewsMap[p].push_back(MLPropertyViewPtr(new MLPropertyView(w, attr))); 
}

void MLReporter::doPropertyChangeAction(MLSymbol prop, const MLProperty& newVal)
{
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
			v.view(newVal);
		}
	}
}
