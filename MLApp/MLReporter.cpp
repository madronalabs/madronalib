
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLReporter.h"

// --------------------------------------------------------------------------------
#pragma mark param viewing 

MLParamView::MLParamView(MLWidget* w, MLSymbol a) :
	mpWidget(w),
	mAttr(a)
{

}

MLParamView::~MLParamView()
{
}
	
void MLParamView::view(const MLModelParam& p) const
{
	switch(p.getType())
	{
		case MLModelParam::kUndefinedParam:
			break;
		case MLModelParam::kFloatParam:
			mpWidget->setAttribute(mAttr, p.getFloatValue());		
			break;
		case MLModelParam::kStringParam:
			mpWidget->setStringAttribute(mAttr, p.getStringValue());		
			break;
		case MLModelParam::kSignalParam:
			// unused
			break;
	}
}

// --------------------------------------------------------------------------------
#pragma mark MLReporter 
	
MLReporter::MLReporter(MLModel* m) :
	MLModelListener(m)
{
	mpModel->addParamListener(this); 
}

MLReporter::~MLReporter()
{
}

// ----------------------------------------------------------------
// parameter viewing

// add a parameter view. 
// when param p changes, attribute attr of Widget w will be set to the param's value.
//
void MLReporter::addParamViewToMap(MLSymbol p, MLWidget* w, MLSymbol attr)
{
	mParamViewsMap[p].push_back(MLParamViewPtr(new MLParamView(w, attr))); 
}

void MLReporter::doParamChangeAction(MLSymbol param, const MLModelParam& , const MLModelParam& newVal)
{
	// debug() << "MLReporter::doParamChangeAction: " << param << " from " << oldVal << " to " << newVal << "\n";	
	// do we have viewers for this parameter?
	MLParamViewListMap::iterator look = mParamViewsMap.find(param);
	if (look != mParamViewsMap.end())
	{		
		// run viewers
		MLParamViewList viewers = look->second;
		for(MLParamViewList::iterator vit = viewers.begin(); vit != viewers.end(); vit++)
		{
			MLParamViewPtr pv = (*vit);
			const MLParamView& v = (*pv);
			v.view(newVal);
		}
	}
}
